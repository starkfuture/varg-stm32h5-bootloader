//
// Created by aleja on 04/03/2024.
//

#ifndef PACKET2_H
#define PACKET2_H

#include <stdio.h>
#include "../utils.h"

#define PACKET_START_BYTE 0x5A
#define PACKET_END_BYTE 0xA5
#define PACKET_CRC_FAIL 0xF4
#define PACKET_CRC_HDR_SEED 0xA1




typedef void (*packet_func_t)(void* custom, uint8_t* data, uint16_t len);
typedef uint16_t (*crc_func_t)(uint16_t crc_init, uint8_t *data, uint32_t len);
typedef void (*reset_func_t)(void);
typedef struct{
    uint8_t *tx_buffer;
    uint16_t tx_buffer_size;
    uint8_t *rx_buffer;
    uint16_t rx_buffer_size;
    uint8_t rx_state;
    uint16_t len;
    uint16_t ptr;
    packet_func_t send_func;
    packet_func_t process_func;
    reset_func_t reset_func;
    crc_func_t crc_func;
    uint16_t crc_init;
    uint16_t crc;
    uint16_t crc_rec;
    uint32_t timer;
    uint32_t timeout;
    uint8_t header[3];
    uint8_t hdr_ptr;
    void * custom_data;
}packet2_t;

void packet2_init(packet2_t *pack, uint16_t crc_init, uint8_t *tx_buffer, uint16_t tx_buffer_size, uint8_t *rx_buffer,
                  uint16_t rx_buffer_size, uint32_t timeout, void* custom_data);
void packet_init_func(packet2_t *pack, packet_func_t send_func, packet_func_t process_func, crc_func_t crc_func, reset_func_t reset_func);
void packet_timeout_func(packet2_t *pack);
void packet_receive_byte(packet2_t *pack, uint8_t data);
void packet_send_packet(packet2_t *pack, uint8_t *data, uint16_t len);

unsigned short crc_ccitt_update (unsigned short crc, unsigned char data);

#endif //PACKET2_H
