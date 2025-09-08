#pragma once
#include <stddef.h>
#include <stdint.h>
#define CEIL_DIV(x, y) (((x) + (y) - 1) / (y))

#ifndef sizeof_array
#define sizeof_array(x) (sizeof(x)/sizeof(*x))
#endif

#define FNV_OFFSET_BASIS 0x811C9DC5
#define FNV_PRIME 0x01000193



typedef struct {
    const float *x;
    const float *y;
    const float *z;
    unsigned int x_size;
    unsigned int y_size;
} float_table_t;

typedef struct {
    const float *x;
    const float *y;
    unsigned int size;
} float_xy_table_t;


#define ABS_INT(x) ((x) < 0 ? -(x) : (x))

#define FLOAT_TABLE_INIT(_x,_y,_z) \
{ \
    .x = _x,                       \
    .x_size = sizeof_array(_x),                       \
    .y = _y,                       \
    .y_size = sizeof_array(_y),                       \
    .z = (const float*)_z,                       \
}

//#define FLOAT_TABLE_Z(table,i,j) ((table)->z[(( * (table)->y_size) + j)])
#define FLOAT_TABLE_Z(table,i,j) *((table)->z + (((i) * (table)->y_size) + (j)))
//
// (*((table)->z + ((i) * (table)->y_size) + (j)))    //z[i][j];
//#define FLOAT_TABLE_Z(table,i,j) ((table)->z[i][j])
int binary_search(float x0, const float*x, unsigned int size);

int interp_2d(float* out, float *my,
              float x0, float y0, const float_table_t *table);

int interp_1d(float* out, float *m, float x0, const float_xy_table_t * table);

float linear_convex_concave_regression_pointbypoint(float Xinit, float Xfinal, float Yinit, float Yfinal, int RegressionType, float RegressionCoeffiecient, float X);
typedef int (*compare_func_t)(void* a, void*b);
void bubble_sort(void* arr,unsigned int array_size, unsigned int element_size, void* temp, compare_func_t compare);



void buffer_append_uint32_big(uint32_t source, uint8_t *dst);
void buffer_append_uint32_little(uint32_t source, uint8_t *dst);

static inline void buffer_append_int32_big(int32_t source, uint8_t *dst) {
    buffer_append_uint32_big((uint32_t) source, dst);
}

static inline void buffer_append_int32_little(int32_t source, uint8_t *dst) {
    buffer_append_uint32_little((uint32_t) source, dst);
}

void buffer_append_uint16_big(uint16_t source, uint8_t *dst);
void buffer_append_uint16_little(uint16_t source, uint8_t *dst);

static inline void buffer_append_int16_big(int16_t source, uint8_t *dst) {
    buffer_append_uint16_big((uint16_t) source, dst);
}

static inline void buffer_append_int16_little(int16_t source, uint8_t *dst) {
    buffer_append_uint16_little((uint16_t) source, dst);
}


void buffer_append_uint24_big(uint32_t source, uint8_t *dst);
void buffer_append_uint24_little(uint32_t source, uint8_t *dst);

static inline void buffer_append_int24_big(int32_t source, uint8_t *dst) {
    buffer_append_uint24_big((uint32_t) source, dst);
}

static inline void buffer_append_int24_little(int32_t source, uint8_t *dst) {
    buffer_append_uint24_little((uint32_t) source, dst);
}
uint32_t buffer_extract_uint32(uint8_t const *src, uint16_t *index, uint8_t reverse);
uint32_t buffer_extract_uint32_big(uint8_t const *src);
uint32_t buffer_extract_uint32_little(uint8_t const *src);

int32_t buffer_extract_int32_big(uint8_t const *src);
int32_t buffer_extract_int32_little(uint8_t const *src);

uint32_t buffer_extract_uint24_big(uint8_t const *src);
uint32_t buffer_extract_uint24_little(uint8_t const *src);

static inline int32_t buffer_extract_int24_big(uint8_t const *src) {
    return (int32_t) buffer_extract_uint24_big(src);
}
static inline int32_t buffer_extract_int24_little(uint8_t const *src) {
    return (int32_t) buffer_extract_uint24_little(src);
}


uint16_t buffer_extract_uint16_big(uint8_t const *src);
uint16_t buffer_extract_uint16_little(uint8_t const *src);

static inline int16_t buffer_extract_int16_big(uint8_t const *src) {
    return (int16_t) buffer_extract_uint16_big(src);
}
static inline int16_t buffer_extract_int16_little(uint8_t const *src) {
    return (int16_t) buffer_extract_uint16_little(src);
}

uint32_t calc_crc32(const volatile uint8_t *bytes, uint32_t size, uint32_t seed, uint32_t crc);
uint16_t calc_crc16(const volatile uint8_t *bytes, uint32_t size, uint16_t seed, uint16_t crc);
uint8_t calc_crc8(const volatile uint8_t *bytes, uint32_t size, uint8_t seed, uint8_t crc);
uint32_t crc_32(uint8_t *buf, uint32_t buflen);
uint32_t update_crc_32(uint32_t crc, uint8_t c);
uint32_t hash_with_fnv1a(uint32_t prime, const uint8_t* data, size_t size, uint32_t last);

int calc_ntc_resistance(int vad);
int32_t ntc_ohms_to_deg_c(int32_t ohms, float R25C, float B25_100);
void calc_ntc(uint16_t voltage, int16_t *temp,float R25C, float B25_100);

/*  Function generating Gray code
    https://en.wikipedia.org/wiki/Gray_code */
uint16_t gray_code(uint16_t in_byte);
