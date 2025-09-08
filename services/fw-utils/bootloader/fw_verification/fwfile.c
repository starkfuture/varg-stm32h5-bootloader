
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <bits/getopt_core.h>
#include <openssl/evp.h>
#include <openssl/types.h>

#include "tinycrypt/sha256.h"
#include "tinycrypt/ecc_dsa.h"
#include "tinycrypt/ecc.h"
#include "tinycrypt/constants.h"
#include "fw.h"
#include "utils.h"

// Structure for the header

// Function to create the firmware file
int create_file(uint8_t private_key[32], const char* binary_file, uint8_t info[28], const char* output_file) {
    FILE *bin_fp = NULL, *out_fp = NULL;
    uint8_t *binary_data = NULL;
    fw_header_t header;
    struct tc_sha256_state_struct sha_ctx;
    uint8_t header_hash[32];

    const struct uECC_Curve_t * curve = uECC_secp256r1();
    // Open binary file
    bin_fp = fopen(binary_file, "rb");
    if (!bin_fp) {
        fprintf(stderr, "Error: Unable to open binary file\n");
        return -1;
    }
    uint8_t public_key[64];

    if (!uECC_compute_public_key(private_key,public_key,curve)) {
        fprintf(stderr,"Error computing public key\n");
        return -2;
    }

    fprintf(stdout,"Public key: \n");
    printf_buffer(public_key,sizeof(public_key),stdout,16);
    fprintf(stdout,"\n");

    // Get binary file size
    fseek(bin_fp, 0, SEEK_END);
    size_t binary_size = ftell(bin_fp);
    fprintf(stdout,"Size = %lu bytes 0x%08x\n",binary_size,binary_size);
    fseek(bin_fp, 0, SEEK_SET);

    // Allocate memory for binary data
    binary_data = malloc(binary_size);
    if (!binary_data) {
        fprintf(stderr, "Error: Unable to allocate memory for binary data\n");
        fclose(bin_fp);
        return -2;
    }

    // Read the binary file
    if (fread(binary_data, 1, binary_size, bin_fp) != binary_size) {
        fprintf(stderr, "Error: Unable to read binary file\n");
        free(binary_data);
        fclose(bin_fp);
        return -3;
    }
    fclose(bin_fp);

    // Compute SHA-256 hash of the binary
    tc_sha256_init(&sha_ctx);
    tc_sha256_update(&sha_ctx, binary_data, binary_size);
    tc_sha256_final(header.fw_hash, &sha_ctx);

    // Fill the rest of the header
    header.fw_size = (uint32_t)binary_size;
    memcpy(header.info, info, sizeof(header.info));

    // Hash the header (without the signature)
    tc_sha256_init(&sha_ctx);
    tc_sha256_update(&sha_ctx, (uint8_t *)&header, sizeof(fw_header_t) - sizeof(header.signature));
    tc_sha256_final(header_hash, &sha_ctx);
    fprintf(stdout,"Header hash: \n");
    printf_buffer(header_hash,sizeof(header_hash),stdout,16);
    fprintf(stdout,"\n");
    // Generate ECDSA signature for the header hash
    if (uECC_sign(private_key, header_hash, sizeof(header_hash),  header.signature, curve) != TC_CRYPTO_SUCCESS) {
        fprintf(stderr, "Error: ECDSA signature generation failed\n");
        free(binary_data);
        return -4;
    }



    if (uECC_verify(public_key, header_hash, sizeof(header_hash), header.signature,  curve) == TC_CRYPTO_SUCCESS) {
        fprintf(stdout,"Signature is valid\n");
    } else {
        fprintf(stdout,"Signature is not valid\n");
        return -1; // Signature invalid
    }


    fprintf(stdout,"Signature: \n");
    printf_buffer(header.signature,sizeof(header.signature),stdout,16);
    fprintf(stdout,"\n");


    // Open output file
    out_fp = fopen(output_file, "wb");
    if (!out_fp) {
        fprintf(stderr, "Error: Unable to open output file\n");
        free(binary_data);
        return -5;
    }

    // Write the header and binary to the output file
    fwrite(&header, sizeof(header), 1, out_fp);



    fprintf(stdout,"Header: \n");
    printf_buffer((uint8_t*)&header,sizeof(header),stdout,16);
    fprintf(stdout,"\n");


    fwrite(binary_data, binary_size, 1, out_fp);

    // Clean up and close files
    fclose(out_fp);
    free(binary_data);
    return 0;
}

int main(int argc, char *argv[])
{

    int opt;
    char *private_key_bin = NULL;
    char *binary_file = NULL;
    char *info_str = NULL;
    char *output_file = NULL;

    // Parse command-line options using getopt
    while ((opt = getopt(argc, argv, "k:b:i:o:")) != -1) {
        switch (opt) {
            case 'k':
                private_key_bin = optarg;  // Private key in der
            break;
            case 'b':
                binary_file = optarg;  // Path to binary file
            break;
            case 'i':
                info_str = optarg;  // Firmware info string
            break;
            case 'o':
                output_file = optarg;  // Output file path
            break;
            default:
                fprintf(stderr, "Usage: %s -k <private_key_hex> -b <binary_file> -i <info_string> -o <output_file>\n", argv[0]);
            return -1;
        }
    }

    if (private_key_bin==NULL || binary_file == NULL || output_file==NULL) {
        fprintf(stderr, "Invalid arguments\n");
        return -2;
    }


    FILE* fp = fopen(private_key_bin,"rb");
    if (fp==NULL) {
        fprintf(stderr,"Fail to open private key %s\n",private_key_bin);
        return -1;
    }
    uint8_t private_key[32];
    fread(private_key,1,sizeof(private_key),fp);
    fclose(fp);



    int result=0;
    do {
        uint8_t info[28]={0};
        if (info_str!=NULL) {
            strncpy(info,info_str,sizeof(info));
        }

        result = create_file(private_key,binary_file,info,output_file);
    } while(0);

    printf("Hello, World!\n");
    return result;
}
