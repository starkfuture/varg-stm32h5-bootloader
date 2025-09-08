#include "utils.h"

#include <string.h>
#include <stddef.h>
#include <math.h>

int binary_search(float x0, const float*x, unsigned int size){
    if (size<1){
        return -1;
    }
    int left = 0;
    int right = (int)size -1;
       
    int below=-1;
    if (x0>x[right]){
        return (int)size;
    }
    while (left <= right) {
        int mid = left + ((right - left)>>1);

        // Check if the target is at the middle
        if (x[mid]<=x0){ 
            below = mid;
            left = mid + 1;
        } else {
            right = mid - 1;
        }
    }
    return below;
}



int interp_1d(float* out, float *m, float x0, const float_xy_table_t * table){

    if (table==NULL){
        return -1;
    }
    if (table->size<2){
        return -2;
    }

    int i1=0;
    int i2=0;

    //Find the index of X
    i1 = binary_search(x0,table->x,table->size);

    if (i1<0){
        i1=0;
        i2=i1;
    } else if(i1>=table->size-1){
        i1=(int)table->size-1;
        i2=i1;
    } else {
        i2=i1+1;
    }

    float x1,x2;
    float y1,y2;

    x1 = table->x[i1];
    x2 = table->x[i2];
    if (x0<x1){
        x0=x1;
    } else if (x0>x2){
        x0=x2;
    }

    y1 = table->y[i1];
    y2 = table->y[i2];

    float m_val=0.0f;
    if (x2!=x1) {
        m_val = (y2-y1)/(x2-x1);
    }

    float y = m_val*(x0-x1) + y1;
    if (out!=NULL){
        *out=y;
    }
    if (m!=NULL) {
        *m=m_val;
    }
    return 0;
}

int interp_2d(float* out, float *my,
    float x0, float y0, const float_table_t *table){

    if (table==NULL){
        return -1;
    }
    if (table->x_size<2 || table->y_size<2){
        return -2;
    }

    int i1=0;
    int j1=0;
    int i2=0;
    int j2=0;

    //Find the index of X
    i1 = binary_search(x0,table->x,table->x_size);

    if (i1<0){
        i1=0;
        i2=i1;
    } else if(i1>=table->x_size-1){
        i1=(int)table->x_size-1;
        i2=i1;
    } else {
        i2=i1+1;
    }

    //Find the index of Y
    j1 = binary_search(y0,table->y,table->y_size);
    if (j1<0){
        j1=0;
        j2=j1;
    } else if(j1>=table->y_size-1){
        j1=(int)table->y_size-1;
        j2=j1;
    } else {
        j2=j1+1;
    }
    float z11,z12,z21,z22;
    z11 = FLOAT_TABLE_Z(table, i1, j1);

    z12 = FLOAT_TABLE_Z(table, i1, j2);
    z21 = FLOAT_TABLE_Z(table, i2, j1);
    z22 = FLOAT_TABLE_Z(table, i2, j2);

    float x1,x2;
    float y1,y2;

    x1 = table->x[i1];
    x2 = table->x[i2];
    if (x0<x1){
        x0=x1;
    } else if (x0>x2){
        x0=x2;
    }

    y1 = table->y[j1];
    y2 = table->y[j2];
    if (y0<y1){
        y0=y1;
    } else if (y0>y2){
        y0=y2;
    }

    /*
              ----------------mx_y2-------------------
      z12(x1,y2)         z02(x0,y2)             z22(x2,y2)
  |        .                   .                     .          |
  |                            |                                |
  |                          my_x0_2                            |
  |                            |                                |
 my_x1     . -----mx_y0_1----- . -----mx_y0_2------- .        my_x2
  |    z10(x1,y0)         z00(x0,y0)             z20(x2,y0)     |
  |                            |                                |
  |                          my_x0_1                            |
  |                            |                                |
  |        .                   .                     .          |
      z11(x1,y1)         z01(x0,y1)             z21(x2,y1)
              ----------------mx_y1-------------------    */



        // Calculate my

        float mx_y1;    // slope at y1 between x2 and x1
        float mx_y2;    // slope at y2 between x2 and x1

        if (x2==x1){
            mx_y1=0.0f;
            mx_y2=0.0f;
        } else {
            // slope at y1 between x2 and x1
            mx_y1 = (z21 - z11) / (x2 - x1);
            // slope at y2 between x2 and x1
            mx_y2 = (z22-z12)/(x2-x1);
        }
        // z01 at (x0,y1)
        float z01 = mx_y1*(x0-x1) + z11;

        // z02 at (x0,y2)
        float z02 = mx_y2*(x0-x1) + z12;


        float my_x0;
        if (y2==y1){
            my_x0=0;
        } else {
            my_x0 = (z02-z01)/(y2-y1);
        }
        float z00 = my_x0*(y0-y1) + z01;
        if (out!=NULL){
            *out=z00;
        }
        if (my!=NULL) {
            *my = my_x0;
        }


    return 0;
}


void bubble_sort(void* arr,unsigned int array_size, unsigned int element_size, void* temp, compare_func_t compare) {
    if (arr==NULL || temp==NULL || compare==NULL){
        return;
    }
    unsigned int i;
    unsigned int j;
    for (i = 0; i < array_size - 1; i++) {
        for (j = 0; j < array_size - i - 1; j++) {
            void* a = (uint8_t*)arr+(j*element_size);
            void *b = (uint8_t*)arr+((j+1)*element_size);
            if (compare(a,b)>0){
                // Swap arr[j] and arr[j+1]
                memcpy(temp,a,element_size);
                memcpy(a,b,element_size);
                memcpy(b,temp,element_size);

            }
        }
    }
}



void buffer_append_uint32_big(uint32_t source, uint8_t *dst){
    dst[0] = (uint8_t)(source>>24);
    dst[1] = (uint8_t)(source>>16);
    dst[2] = (uint8_t)(source>>8);
    dst[3] = (uint8_t)(source);
}
void buffer_append_uint32_little(uint32_t source, uint8_t *dst){
    dst[3] = (uint8_t)(source>>24);
    dst[2] = (uint8_t)(source>>16);
    dst[1] = (uint8_t)(source>>8);
    dst[0] = (uint8_t)(source);
}

void buffer_append_uint24_big(uint32_t source, uint8_t *dst){
    dst[0] = (uint8_t)(source>>16);
    dst[1] = (uint8_t)(source>>8);
    dst[2] = (uint8_t)(source);
}
void buffer_append_uint24_little(uint32_t source, uint8_t *dst){
    dst[0] = (uint8_t)(source);
    dst[1] = (uint8_t)(source>>8);
    dst[2] = (uint8_t)(source>>16);
}

void buffer_append_uint16_big(uint16_t source, uint8_t *dst){
    dst[0] = (uint8_t)(source>>8);
    dst[1] = (uint8_t)(source);
}
void buffer_append_uint16_little(uint16_t source, uint8_t *dst){
    dst[1] = (uint8_t)(source>>8);
    dst[0] = (uint8_t)(source);
}

uint32_t buffer_extract_uint32(uint8_t const *src, uint16_t *index, uint8_t reverse){
    uint32_t result;
    if (reverse){
        result = buffer_extract_uint32_little(&src[(*index)]);

    }else {
        result = buffer_extract_uint32_big(&src[(*index)]);
    }
    (*index) +=4;
    return result;
}


uint32_t buffer_extract_uint32_big(uint8_t const *src){
    uint32_t result = 0;
    result |= (uint32_t)(src[0]<<24);
    result |= (uint32_t)(src[1]<<16);
    result |= (uint32_t)(src[2]<<8);
    result |= (uint32_t)(src[3]);
    return result;
}
uint32_t buffer_extract_uint32_little(uint8_t const *src){
    uint32_t result = 0;
    result |= (uint32_t)(src[3]<<24);
    result |= (uint32_t)(src[2]<<16);
    result |= (uint32_t)(src[1]<<8);
    result |= (uint32_t)(src[0]);
    return result;
}

uint32_t buffer_extract_uint24_big(uint8_t const *src){
    uint32_t result = 0;
    result |= (uint32_t)(src[0]<<16);
    result |= (uint32_t)(src[1]<<8);
    result |= (uint32_t)(src[2]);
    return result;
}
uint32_t buffer_extract_uint24_little(uint8_t const *src){
    uint32_t result = 0;
    result |= (uint32_t)(src[0]);
    result |= (uint32_t)(src[1]<<8);
    result |= (uint32_t)(src[2]<<16);
    return result;
}

int32_t buffer_extract_int32_big(uint8_t const *src) {
    uint32_t result = buffer_extract_uint32_big(src);
    if (result & 0x800000) {
        result|=0xFF000000;
    }
    return (int32_t)result;
}
int32_t buffer_extract_int32_little(uint8_t const *src) {
    uint32_t result = buffer_extract_uint32_little(src);
    if (result & 0x800000) {
        result|=0xFF000000;
    }
    return (int32_t)result;
}



uint16_t buffer_extract_uint16_big(uint8_t const *src){
    uint16_t result = 0;
    result |= (uint16_t)(src[0]<<8);
    result |= (uint16_t)(src[1]);
    return result;
}
uint16_t buffer_extract_uint16_little(uint8_t const *src){
    uint16_t result = 0;
    result |= (uint16_t)(src[1]<<8);
    result |= (uint16_t)(src[0]);
    return result;
    
}


uint32_t calc_crc32(const volatile uint8_t *bytes, uint32_t size, uint32_t seed, uint32_t crc) {
    for (uint32_t j=0;j<size;j++) {
        uint8_t b = bytes[j];
        crc ^= (uint32_t) (b << 24); /* move byte into MSB of 32bit CRC */
        for (int i = 0; i < 8; i++) {
            if ((crc & 0x80000000) != 0) {/* test for MSB = bit 31 */
                crc = (uint32_t) ((crc << 1) ^ seed);
            } else {
                crc <<= 1;
            }
        }
    }
    return crc;
}

uint16_t calc_crc16(const volatile uint8_t *bytes, uint32_t size, uint16_t seed, uint16_t crc){
    for (uint32_t j=0;j<size;j++) {
        uint8_t b = bytes[j];
        crc ^= (uint16_t) (b << 8); /* move byte into MSB of 16bit CRC */
        for (int i = 0; i < 8; i++) {
            if ((crc & 0x8000) != 0) {/* test for MSB = bit 15 */
                crc = (uint16_t) ((crc << 1) ^ seed);
            } else {
                crc <<= 1;
            }
        }
    }
    return crc;
}


uint8_t calc_crc8(const volatile uint8_t *bytes, uint32_t size, uint8_t seed, uint8_t crc) {
    
    for (uint32_t j=0;j<size;j++) {
        uint8_t b = bytes[j];
        crc ^=b;
        for (int i = 0; i < 8; i++) {
            if ((crc & 0x80) != 0) {/* test for MSB = bit 15 */
                crc = (uint8_t) ((crc << 1) ^ seed);
            } else {
                crc <<= 1;
            }
        }
    }
    return crc;
    
}

// FUNCTION TO CALCULATE LINEAR/CONVEX/CONCAVE REGRESSION
float linear_convex_concave_regression_pointbypoint(float Xinit, float Xfinal, float Yinit, float Yfinal, int RegressionType, float RegressionCoeffiecient, float X) {
    // This function calculates the derating factors regarding the temperature
    // values and the applied (linear, convex, and concave)

    // The factor RegressionCoeffiecient expresses the convexity (for static convex methods) and the concavity (for static concave methods)

    // Inputs:
    //      Xinit: init x-axis coordinates
    //      Xfinal: end x-axis coordinates
    //      Yinit: init x-axis coordinates
    //      Yfinal: end x-axis coordinates
    //      RegressionType: linear, concave or convex curve
    //      RegressionCoeffiecient: only for convex or concave curves
    //      X: current x-axis
    float Y = 0.0f;
    float a, b, c = 0.0f;
    float c0_concave, d0_concave = 0.0f;
    float c0_convex, d0_convex = 0.0f;

    if (RegressionType == 0) {
        // Linear
        Y = (Yinit - Yfinal) / (Xinit - Xfinal) * X + (Yfinal * Xinit - Yinit * Xfinal) / (Xinit - Xfinal);
    } else if (RegressionType == 1 || RegressionType == 2) {
        //        if (RegressionCoeffiecient == 0)
        //            disp('Error: Regression coefficient value is not valid');
        //        else
        //        end
        // Parameters of a second order equation (convex / concave curves)
        a = Yfinal - Yinit;
        b = (-Xinit - Xfinal)*(Yfinal - Yinit);
        c = (Yfinal - Yinit)*(Xinit * Xfinal) + RegressionCoeffiecient * (Xfinal - Xinit);

        if (RegressionType == 1) {
            // Concave
            // Calculation of the concave parameters
            c0_concave = (-b - sqrt(b*b - 4 * a * c)) / 2 / a;
            d0_concave = Yinit - RegressionCoeffiecient / (Xinit - c0_concave);

            // Calculation of the concave curve
            Y = RegressionCoeffiecient / (X - c0_concave) + d0_concave;
        } else if (RegressionType == 2) {
            // Convex
            // Parameters of a second order equation (convex / concave curves)
            a = Yfinal - Yinit;
            b = (-Xinit - Xfinal)*(Yfinal - Yinit);
            c = (Yfinal - Yinit)*(Xinit * Xfinal) + RegressionCoeffiecient * (Xfinal - Xinit);

            // Calculation of the convex parameters
            c0_convex = (-b + sqrt(b*b - 4 * a * c)) / 2 / a;
            d0_convex = Yinit - RegressionCoeffiecient / (Xinit - c0_convex);

            // Calculation of the convex curve
            Y = RegressionCoeffiecient / (X - c0_convex) + d0_convex;
        }
    }
    return Y;
}





#define CRC_START_32 0xFFFFFFFF



const uint32_t crc_tab32[256] = {
        0x00000000,  0x77073096,  0xEE0E612C,  0x990951BA,  0x076DC419,  0x706AF48F,  0xE963A535,  0x9E6495A3,
        0x0EDB8832,  0x79DCB8A4,  0xE0D5E91E,  0x97D2D988,  0x09B64C2B,  0x7EB17CBD,  0xE7B82D07,  0x90BF1D91,
        0x1DB71064,  0x6AB020F2,  0xF3B97148,  0x84BE41DE,  0x1ADAD47D,  0x6DDDE4EB,  0xF4D4B551,  0x83D385C7,
        0x136C9856,  0x646BA8C0,  0xFD62F97A,  0x8A65C9EC,  0x14015C4F,  0x63066CD9,  0xFA0F3D63,  0x8D080DF5,
        0x3B6E20C8,  0x4C69105E,  0xD56041E4,  0xA2677172,  0x3C03E4D1,  0x4B04D447,  0xD20D85FD,  0xA50AB56B,
        0x35B5A8FA,  0x42B2986C,  0xDBBBC9D6,  0xACBCF940,  0x32D86CE3,  0x45DF5C75,  0xDCD60DCF,  0xABD13D59,
        0x26D930AC,  0x51DE003A,  0xC8D75180,  0xBFD06116,  0x21B4F4B5,  0x56B3C423,  0xCFBA9599,  0xB8BDA50F,
        0x2802B89E,  0x5F058808,  0xC60CD9B2,  0xB10BE924,  0x2F6F7C87,  0x58684C11,  0xC1611DAB,  0xB6662D3D,
        0x76DC4190,  0x01DB7106,  0x98D220BC,  0xEFD5102A,  0x71B18589,  0x06B6B51F,  0x9FBFE4A5,  0xE8B8D433,
        0x7807C9A2,  0x0F00F934,  0x9609A88E,  0xE10E9818,  0x7F6A0DBB,  0x086D3D2D,  0x91646C97,  0xE6635C01,
        0x6B6B51F4,  0x1C6C6162,  0x856530D8,  0xF262004E,  0x6C0695ED,  0x1B01A57B,  0x8208F4C1,  0xF50FC457,
        0x65B0D9C6,  0x12B7E950,  0x8BBEB8EA,  0xFCB9887C,  0x62DD1DDF,  0x15DA2D49,  0x8CD37CF3,  0xFBD44C65,
        0x4DB26158,  0x3AB551CE,  0xA3BC0074,  0xD4BB30E2,  0x4ADFA541,  0x3DD895D7,  0xA4D1C46D,  0xD3D6F4FB,
        0x4369E96A,  0x346ED9FC,  0xAD678846,  0xDA60B8D0,  0x44042D73,  0x33031DE5,  0xAA0A4C5F,  0xDD0D7CC9,
        0x5005713C,  0x270241AA,  0xBE0B1010,  0xC90C2086,  0x5768B525,  0x206F85B3,  0xB966D409,  0xCE61E49F,
        0x5EDEF90E,  0x29D9C998,  0xB0D09822,  0xC7D7A8B4,  0x59B33D17,  0x2EB40D81,  0xB7BD5C3B,  0xC0BA6CAD,
        0xEDB88320,  0x9ABFB3B6,  0x03B6E20C,  0x74B1D29A,  0xEAD54739,  0x9DD277AF,  0x04DB2615,  0x73DC1683,
        0xE3630B12,  0x94643B84,  0x0D6D6A3E,  0x7A6A5AA8,  0xE40ECF0B,  0x9309FF9D,  0x0A00AE27,  0x7D079EB1,
        0xF00F9344,  0x8708A3D2,  0x1E01F268,  0x6906C2FE,  0xF762575D,  0x806567CB,  0x196C3671,  0x6E6B06E7,
        0xFED41B76,  0x89D32BE0,  0x10DA7A5A,  0x67DD4ACC,  0xF9B9DF6F,  0x8EBEEFF9,  0x17B7BE43,  0x60B08ED5,
        0xD6D6A3E8,  0xA1D1937E,  0x38D8C2C4,  0x4FDFF252,  0xD1BB67F1,  0xA6BC5767,  0x3FB506DD,  0x48B2364B,
        0xD80D2BDA,  0xAF0A1B4C,  0x36034AF6,  0x41047A60,  0xDF60EFC3,  0xA867DF55,  0x316E8EEF,  0x4669BE79,
        0xCB61B38C,  0xBC66831A,  0x256FD2A0,  0x5268E236,  0xCC0C7795,  0xBB0B4703,  0x220216B9,  0x5505262F,
        0xC5BA3BBE,  0xB2BD0B28,  0x2BB45A92,  0x5CB36A04,  0xC2D7FFA7,  0xB5D0CF31,  0x2CD99E8B,  0x5BDEAE1D,
        0x9B64C2B0,  0xEC63F226,  0x756AA39C,  0x026D930A,  0x9C0906A9,  0xEB0E363F,  0x72076785,  0x05005713,
        0x95BF4A82,  0xE2B87A14,  0x7BB12BAE,  0x0CB61B38,  0x92D28E9B,  0xE5D5BE0D,  0x7CDCEFB7,  0x0BDBDF21,
        0x86D3D2D4,  0xF1D4E242,  0x68DDB3F8,  0x1FDA836E,  0x81BE16CD,  0xF6B9265B,  0x6FB077E1,  0x18B74777,
        0x88085AE6,  0xFF0F6A70,  0x66063BCA,  0x11010B5C,  0x8F659EFF,  0xF862AE69,  0x616BFFD3,  0x166CCF45,
        0xA00AE278,  0xD70DD2EE,  0x4E048354,  0x3903B3C2,  0xA7672661,  0xD06016F7,  0x4969474D,  0x3E6E77DB,
        0xAED16A4A,  0xD9D65ADC,  0x40DF0B66,  0x37D83BF0,  0xA9BCAE53,  0xDEBB9EC5,  0x47B2CF7F,  0x30B5FFE9,
        0xBDBDF21C,  0xCABAC28A,  0x53B39330,  0x24B4A3A6,  0xBAD03605,  0xCDD70693,  0x54DE5729,  0x23D967BF,
        0xB3667A2E,  0xC4614AB8,  0x5D681B02,  0x2A6F2B94,  0xB40BBE37,  0xC30C8EA1,  0x5A05DF1B,  0x2D02EF8D
};


inline uint32_t update_crc_32(uint32_t crc, uint8_t c)
{
    return (crc >> 8) ^ crc_tab32[(crc ^ (uint32_t)c) & 0x000000FF];
}


uint32_t crc_32(uint8_t *buf, uint32_t buflen)
{

    uint32_t crc = CRC_START_32;
    for (uint32_t i = 0; i < buflen; i++) {
        crc = update_crc_32(crc, *buf++);
    }
    crc = crc ^ 0xFFFFFFFF;
    return(crc);
}


// calculate resistance of PT1000 given ADC counts (vad)
int calc_ntc_resistance(int vad) {
    long res;

    if (vad <= 0) res = 100000000L;
    else {

        res = ((vad * 10000) / (30000 - vad));
        if (res < 0) res = 0;
        else if (res > 100000000) res = 100000000;
    }
    return(res);
}

// calculate NTC temperature given ohms, R25C, and B25_100
// return as signed integer, ten counts per deg C
int32_t ntc_ohms_to_deg_c(int32_t ohms, float R25C, float B25_100)
{
    // limit minimum resistance to 1 ohm
    if (ohms < 1) {
        ohms = 1;
    }
    // calculate 1 / temperature in deg Kelvin
    float f = (logf((float)ohms / R25C) / B25_100) + ((float)1.0 / (float)298.15);
    // calculate temperature in deg Kelvin * 10
    f = 10.0f / f;
    // return temperature in deg C * 10
    return((int32_t)(f - 2731.0f));
}

void calc_ntc(uint16_t voltage, int16_t *temp,float R25C, float B25_100) {

    // temp->voltage = voltage;
    // temp->ohms = calc_ntc_resistance(voltage);
    *temp /*->temperature */ = ntc_ohms_to_deg_c(calc_ntc_resistance(voltage), R25C, B25_100);
}


uint32_t hash_with_fnv1a(uint32_t prime, const uint8_t* data, size_t size, uint32_t last) {
    if (data==NULL) {
        return last;
    }
    for (size_t i = 0; i <size; ++i) {
        last ^= data[i];
        last *= prime;
    }
    return last;
}



uint16_t gray_code(const uint16_t in_byte) {
    return in_byte ^ (in_byte >> 1);
}