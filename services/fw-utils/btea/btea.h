//
// Created by nico on 14/11/24.
//

#pragma once
#include <stdint.h>
typedef struct {
    uint32_t key[4];
}btea_key_t;


void btea(uint32_t *v, int n, const btea_key_t* key);



