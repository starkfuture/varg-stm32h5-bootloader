//
// Created by nico on 11/10/24.
//

#ifndef UTILS_H
#define UTILS_H
#include <stddef.h>
#include <stdio.h>
#include <openssl/types.h>


EVP_PKEY* load_public_key_from_der(const char* filepath);

int extract_private_key_as_bytes(EVP_PKEY *pkey, uint8_t* data, size_t len);
void printf_buffer(const uint8_t* data, size_t size, FILE* fp, uint8_t align);

// Function to generate C code for the public key
int print_key_c(FILE* fp, const char* name, const uint8_t *data, int len);
#endif //UTILS_H
