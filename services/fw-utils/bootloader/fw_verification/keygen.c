#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>  // For getopt
#include <openssl/ec.h>
#include <openssl/pem.h>
#include <tinycrypt/ecc.h>
#include <tinycrypt/ecc_dh.h>

#include "utils.h"


void save_key(const char* key_file, const char* name, const uint8_t *data, int len) {
    if (key_file!=NULL) {
        FILE *fp = fopen(key_file, "w");
        if (fp) {
            print_key_c(fp,name,data,len);
            fclose(fp);
        } else {
            fprintf(stderr, "Error: Unable to open file for writing public key %s\n",key_file);
        }
    }
}

// Function to generate an EC key pair using OpenSSL
int generate_keypair(const char *private_key_bin, const char *public_key_c) {
    const struct uECC_Curve_t * curve = uECC_secp256r1();

    uint8_t private_key[32];
    uint8_t public_key[64];
    if(!uECC_make_key(private_key,public_key,curve)) {
        fprintf(stderr,"Error creating key pair");
    }


    /* Save the private key in DER (binary) format */
    FILE *privkey_file = fopen(private_key_bin, "wb");
    if (privkey_file) {
        int priv_key_len = sizeof(private_key);
        if (priv_key_len > 0) {
            fwrite(private_key, 1,sizeof(private_key), privkey_file);
        } else {
            fprintf(stderr, "Error converting private key to bin format.\n");
        }
        fclose(privkey_file);
    }

    print_key_c(stdout,"public_key", public_key,sizeof(public_key));

    save_key(public_key_c, "public_key", public_key,sizeof(public_key));
    // Save the public key in C code format

    printf("Key pair generated and saved in binary format.\n");
    return 0;

}

int load_private_key_generate_public(const char *private_key_bin, const char *public_key_c) {
    FILE* fp = fopen(private_key_bin,"rb");
    if (fp==NULL) {
        fprintf(stderr,"Fail to open private key %s\n",private_key_bin);
        return -1;
    }
    uint8_t private_key[32];
    uint8_t public_key[64];
    fread(private_key,1,sizeof(private_key),fp);
    fclose(fp);
    const struct uECC_Curve_t * curve = uECC_secp256r1();

    if (!uECC_compute_public_key(private_key,public_key,curve)) {
        fprintf(stderr,"Error computing public key\n");
        return -2;
    }
    print_key_c(stdout,"public_key", public_key,sizeof(public_key));

    save_key(public_key_c, "public_key", public_key,sizeof(public_key));


    return 0;
}

// Main function using getopt for argument parsing
int main(int argc, char *argv[]) {
    int opt;
    char *command = NULL;
    char *private_key_bin = NULL;
    char *public_key_c = NULL;

    // Parse command-line options using getopt
    while ((opt = getopt(argc, argv, "c:k:p:")) != -1) {
        switch (opt) {
        case 'c':
            command = optarg;  // Command: generate or load
            break;
        case 'k':
            private_key_bin = optarg;  // Private key file
            break;
        case 'p':
            public_key_c = optarg;  // Public key C file
            break;
        default:
            fprintf(stderr, "Usage: %s -c <command> -k <private_key_bin> -p <public_key_c>\n", argv[0]);
            return -1;
        }
    }

    // Ensure all required arguments are provided
    if (command==NULL || private_key_bin==NULL) {
        fprintf(stderr, "Usage: %s -c <command> -k <private_key_bin> [-p <public_key_c>]\n", argv[0]);
        return -1;
    }

    if (strcmp(command, "generate") == 0) {
        return generate_keypair(private_key_bin, public_key_c);
    } else if (strcmp(command, "load") == 0) {
        return load_private_key_generate_public(private_key_bin, public_key_c);
    } else {
        fprintf(stderr, "Error: Unknown command '%s'\n", command);
        return -1;
    }
}
