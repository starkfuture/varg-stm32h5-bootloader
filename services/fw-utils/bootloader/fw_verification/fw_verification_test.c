#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <bits/getopt_core.h>

#include "tinycrypt/ecc_dsa.h"
#include "tinycrypt/sha256.h"
#include "tinycrypt/constants.h"
#include "fw.h"
// Firmware header structure

// Structure to manage firmware processing
typedef struct {
    uint32_t bytes_processed;   // Number of bytes processed
    struct tc_sha256_state_struct sha_ctx;  // Cumulative SHA-256 context
    const fw_header_t *header;        // Verified firmware header
} fw_t;

void printf_buffer(const uint8_t* data, size_t size, FILE* fp, uint8_t align) {
    for (int i = 0; i < size; i++) {
        if (i % align == 0) {  // Add a new line after every 12 bytes for readability
            if (i!=0) {
                fprintf(fp,"\n");
            }
            fprintf(fp,"    ");
        }
        fprintf(fp,"0x%02x", data[i]);
        if (i != size - 1) {
            fprintf(fp,", ");
        }
    }
}
// Verify the firmware header's signature using the public key
int process_header(const fw_header_t* header, const uint8_t public_key[64]) {
    if (header==NULL) {
        fprintf(stderr,"header is null\n");
        return -2;
    }
    uint8_t header_hash[32];
    struct tc_sha256_state_struct sha_ctx;
    const struct uECC_Curve_t * curve = uECC_secp256r1();

    // Hash the header (excluding the signature fields)
    tc_sha256_init(&sha_ctx);
    tc_sha256_update(&sha_ctx, (uint8_t *)header, sizeof(fw_header_t) - sizeof(header->signature));
    tc_sha256_final(header_hash, &sha_ctx);

    fprintf(stdout,"Header hash: \n");
    printf_buffer(header_hash,sizeof(header_hash),stdout,16);
    fprintf(stdout,"\n");
    // Verify the ECDSA signature
    if (uECC_verify(public_key, header_hash, sizeof(header_hash), header->signature,  curve)) {
        fprintf(stdout,"Signature is valid\n");
        return 0;  // Signature valid
    } else {
        fprintf(stdout,"Signature is not valid\n");
        return -1; // Signature invalid
    }
}

int process_init(fw_t* firmware, const fw_header_t* header, const uint8_t public_key[64]) {
    if (firmware==NULL || header==NULL) {
        return -2;
    }
    int result = process_header(header,public_key);
    if (result!=0) {
        return result;
    }
    tc_sha256_init(&firmware->sha_ctx);
    firmware->header=header;
    firmware->bytes_processed=0;

    return 0;
}

// Process binary data in chunks
int process_binary(fw_t* firmware, uint8_t* data, uint32_t size) {
    if (firmware==NULL || firmware->header==NULL || data==NULL) {
        return -1;  // Null pointer error
    }

    // Ensure the size doesn't exceed the remaining firmware size
    if (firmware->bytes_processed + size > firmware->header->fw_size) {
        return -2;  // Out of bounds
    }

    // Update the cumulative SHA-256 hash
    tc_sha256_update(&firmware->sha_ctx, data, size);
    firmware->bytes_processed += size;

    // Check if the entire binary has been processed
    if (firmware->bytes_processed == firmware->header->fw_size) {
        uint8_t final_hash[32];
        tc_sha256_final(final_hash, &firmware->sha_ctx);

        // Verify the final hash matches the one in the header
        if (memcmp(final_hash, firmware->header->fw_hash, 32) == 0) {
            return 1;  // Firmware successfully processed
        } else {
            return -3;  // Hash mismatch
        }
    }

    return 0;  // Processing is incomplete but no errors
}


const uint8_t public_key[] = {
    0x38, 0x2f, 0x12, 0xb2, 0x1a, 0x11, 0xf4, 0xcf, 0x36, 0x3d, 0xd5, 0xc3, 0xe5, 0xcb, 0x15, 0xb7,
    0x86, 0x34, 0x30, 0xe1, 0xa7, 0xfd, 0x62, 0xd4, 0x5c, 0xba, 0x9d, 0x8c, 0xbc, 0x42, 0xef, 0x7d,
    0xed, 0x88, 0x54, 0xb4, 0xa2, 0x2f, 0x97, 0xe6, 0x67, 0x84, 0xec, 0x6e, 0xf3, 0xb4, 0xc2, 0xfe,
    0x2c, 0x5a, 0x30, 0x05, 0xd6, 0xbc, 0x5e, 0xaf, 0x1b, 0xd5, 0xab, 0x41, 0xfd, 0xe5, 0x05, 0x7a
};



int main(int argc, char *argv[])
{


    int opt;
    char *firmware_file = NULL;

    // Parse command-line options using getopt
    while ((opt = getopt(argc, argv, "f:")) != -1) {
        switch (opt) {
            case 'f':
                firmware_file = optarg;  // Private key in der
            break;

            default:
                fprintf(stderr, "Usage: %s -f <firmware_file>\n", argv[0]);
            return -1;
        }
    }



    uint8_t buffer[8];

    fw_t fw = {
        .bytes_processed = 0,
        .header = NULL,
    };

    fw_header_t header;




    FILE *fp = fopen(firmware_file,"rb");
    if (fp==NULL) {
        fprintf(stderr, "Error opening file %s\n",firmware_file);
        return -1;
    }


    uint32_t remaining = sizeof(fw_header_t);
    uint32_t read = 0;
    // Loop through the file reading 128-byte chunks
    while (remaining>0) {
        uint32_t to_read = remaining;
        if (to_read>=sizeof(buffer)) {
            to_read=sizeof(buffer);
        }
        size_t bytes_read = fread(buffer, 1, to_read, fp);
        if (bytes_read==0) {
            fprintf(stderr,"Error reading file %s. %lu\n",firmware_file,bytes_read);
            break;
        }

        uint8_t* ptr = (uint8_t*)&header;
        memcpy(&ptr[read],buffer,bytes_read);
        read+=bytes_read;
        remaining-=bytes_read;
    }
    fprintf(stdout,"Header: \n");
    printf_buffer((uint8_t*)&header,sizeof(header),stdout,16);
    fprintf(stdout,"\n");

    if (remaining==0) {
        //header is complete
        int result = process_init(&fw,&header,public_key);
        if (result!=0) {
            fprintf(stderr, "Error processing header. %d\n",result);
            fclose(fp);
            return -1;
        }
    }

    remaining = header.fw_size;

    while (remaining>0) {
        uint32_t to_read = remaining;
        if (to_read>=sizeof(buffer)) {
            to_read=sizeof(buffer);
        }
        size_t bytes_read = fread(buffer, 1, to_read, fp);
        if (bytes_read<0) {
            fprintf(stderr,"Error reading file %s. %lu\n",firmware_file,bytes_read);
            break;
        }
        remaining-=bytes_read;
        int result = process_binary(&fw, buffer, bytes_read);
        if (result<0) {
            fprintf(stderr,"Error processing data %d\n",result);
            break;
        } else if (result >0) {
            fprintf(stdout, "Firmware completed successfull\n");
            break;
        }
    }
    if (remaining>0) {
        fprintf(stderr,"Something went wrong. remaining to read %d\n",remaining);
    } else {
        fprintf(stdout,"Process finished\n");
    }


    fclose(fp);
    return 0;
}
