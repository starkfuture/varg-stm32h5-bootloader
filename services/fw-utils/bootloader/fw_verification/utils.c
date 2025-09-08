//
// Created by nico on 11/10/24.
//

#include "utils.h"

#include <stdio.h>
#include <stdlib.h>
// #include <openssl/ec.h>
// #include <openssl/evp.h>
// #include <openssl/err.h>

// #include <openssl/core_names.h>
// #include <openssl/x509.h>

// EVP_PKEY* load_public_key_from_der(const char* filepath) {
//     FILE *file = fopen(filepath, "rb");
//     if (!file) {
//         fprintf(stderr, "Error opening file %s.\n", filepath);
//         return NULL;
//     }
//
//     // Read the file contents
//     fseek(file, 0, SEEK_END);
//     long len = ftell(file);
//     fseek(file, 0, SEEK_SET);
//
//     uint8_t *buffer = (uint8_t*)malloc(len);
//     if (fread(buffer, 1, len, file) != len) {
//         fprintf(stderr, "Error reading DER file.\n");
//         free(buffer);
//         fclose(file);
//         return NULL;
//     }
//     fclose(file);
//
//     // Load the public key from the buffer
//     const uint8_t *p = buffer;
//     EVP_PKEY *pkey = d2i_PrivateKey(EVP_PKEY_EC, NULL, &p, len);
//
//     free(buffer);
//
//     if (!pkey) {
//         fprintf(stderr, "Error loading public key from DER file.\n");
//         // ERR_print_errors_fp(stderr);
//         return NULL;
//     }
//
//     return pkey;
// }


/* Function to extract the private key as bytes using OpenSSL 3.0 */
// int extract_private_key_as_bytes(EVP_PKEY *pkey, uint8_t* data, size_t len) {
//     if (pkey==NULL) {
//         fprintf(stderr, "Invalid EVP_PKEY structure.\n");
//         return -1;
//     }
//
//     if (data==NULL) {
//         fprintf(stderr, "Invalid data pointer\n");
//         return -2;
//     }
//
//
//
//     // Extract the private key as a BIGNUM
//     BIGNUM *priv_bn=NULL;
//     if (!EVP_PKEY_get_bn_param(pkey,OSSL_PKEY_PARAM_PRIV_KEY,&priv_bn)) {
//         fprintf(stderr, "Failed to extract private key BIGNUM.\n");
//         return -1;
//     }
//
//     // Get the private key size (it should be 32 bytes for prime256v1)
//     int key_len = BN_num_bytes(priv_bn);
//     unsigned char *priv_key_bytes = (uint8_t *)malloc(key_len);
//     if (!*priv_key_bytes) {
//         fprintf(stderr, "Memory allocation failed.\n");
//         BN_clear_free(priv_bn);
//
//         return -1;
//     }
//     // Convert the BIGNUM to raw bytes
//     BN_bn2bin(priv_bn, priv_key_bytes);
//
//     BN_clear_free(priv_bn);
//
//
//     if (len<key_len) {
//         fprintf(stderr, "Allocated data is not enough %lu < %lu\n",len,key_len);
//         free(priv_key_bytes); // Free the allocated private key bytes
//         return -3;
//     }
//
//     memcpy(data,priv_key_bytes,key_len);
//     free(priv_key_bytes); // Free the allocated private key bytes
//
//     // // Print the private key bytes in hex format
//     // printf("Private key bytes (%zu bytes):\n", key_len);
//     // for (size_t i = 0; i < key_len; i++) {
//     //     printf("%02x", data[i]);
//     //     if (i != key_len - 1) {
//     //         printf(":");
//     //     }
//     // }
//     // printf("\n");
//
//     return key_len;
// }

// Function to generate C code for the public key
int print_key_c(FILE* fp, const char* name, const uint8_t *data, int len) {

    if (data==NULL||name==NULL) {
        return -1;
    }





    if (len > 0) {
        fprintf(fp,"const uint8_t %s[] = {\n",name);
        printf_buffer(data,len,fp,16);
        fprintf(fp,"\n};\n");
    } else {
        fprintf(stderr, "Error converting public key to C format.\n");
    }

    return 0;
}


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