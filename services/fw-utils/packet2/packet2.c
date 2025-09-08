//
// Created by aleja on 04/03/2024.
//

#include <string.h>
#include "packet2.h"

void send_crc_fail(packet2_t *pack);
static uint16_t crc_calc(packet2_t *pack, uint16_t crc_init, uint8_t *data, uint16_t len);

void packet2_init(packet2_t *pack, uint16_t crc_init, uint8_t *tx_buffer, uint16_t tx_buffer_size, uint8_t *rx_buffer,
                  uint16_t rx_buffer_size, uint32_t timeout, void* custom_data){
    pack->tx_buffer = tx_buffer;
    pack->tx_buffer_size = tx_buffer_size;
    pack->rx_buffer = rx_buffer;
    pack->rx_buffer_size = rx_buffer_size;
    pack->custom_data = custom_data;
    pack->rx_state = 0;
    pack->timeout = timeout;
    pack->timer = pack->timeout;
    pack->crc_init = crc_init;

}

void packet_init_func(packet2_t *pack, packet_func_t send_func, packet_func_t process_func, crc_func_t crc_func, reset_func_t reset_func){
    pack->send_func = send_func;
    pack->process_func = process_func;
    pack->crc_func = crc_func;
    pack->reset_func = reset_func;
}

void packet_timeout_func(packet2_t *pack){
    if (pack->timer) {
        pack->timer--;
    }else{
        if (pack->rx_state) {
            if (pack->reset_func){
                pack->reset_func();
            }
//            printf("packet_timer_timeout rx_state = %d, len=%d\n", pack->rx_state, pack->len);
//            for (uint16_t i=0;i<pack->len; i++){
//                printf("data[%d] = 0x%02X\n",pack->rx_buffer[i]);
//            }
            pack->rx_state = 0;

        }
    }

}

static void packet_restart_timer(packet2_t *pack){
    pack->timer = pack->timeout;
}

void packet_receive_byte(packet2_t *pack, uint8_t data){
    uint8_t good = 1;
    do {
        switch (pack->rx_state) {
            case 0: //first byte
                if (data == PACKET_START_BYTE || data == PACKET_START_BYTE + 1) {
                    pack->rx_state = 2 - (data - PACKET_START_BYTE);
                    pack->hdr_ptr = 0;
                    pack->header[pack->hdr_ptr++] = data;
                    packet_restart_timer(pack);
                    pack->len = 0;
//                pack->crc = pack->crc_init;

                }
                break;
            case 1: //data len [0]
            case 2: //data len [1]
                pack->len |= ((uint16_t) data) << (8 * (2 - (pack->rx_state)));
                pack->rx_state++;
                packet_restart_timer(pack);
                pack->ptr = 0;
                pack->header[pack->hdr_ptr++] = data;
                if (pack->rx_state == 2 && pack->len >= pack->rx_buffer_size) {
                    pack->rx_state = 0;
                    good=0;
//                    printf("len>size\n");
                    break;
                }
                break;
            case 3: //header crc
            {
                uint8_t hdr_crc = calc_crc8(pack->header, pack->hdr_ptr, PACKET_CRC_HDR_SEED, 0x00);
                if (hdr_crc == data) {
                    pack->rx_state++;

                } else {
                    pack->rx_state = 0;
                    good = 0;
                }
                break;
            }
            case 4: //start of data
                packet_restart_timer(pack);

//            pack->crc = crc_ccitt_update(pack->crc, data);
                pack->rx_buffer[pack->ptr++] = data;
                if (pack->ptr == pack->len) {
                    pack->rx_state++; //voy al 5 porque incremento aqui y en lo que salgo del if
                    pack->crc = crc_calc(pack, pack->crc_init, pack->rx_buffer, pack->len);
                }
                break;

            case 5:
                pack->crc_rec = 0;
            case 6:
                packet_restart_timer(pack);
                pack->crc_rec |= (uint16_t) data << (8 * (6 - (pack->rx_state)));
                pack->rx_state++;
                break;
            case 7:
                if (data == PACKET_END_BYTE) {
                    if (pack->crc_rec == pack->crc) {
                        if (pack->process_func) {
                            pack->process_func(pack->custom_data, pack->rx_buffer, pack->len);
                        }

                    } else {
                        good = 0;
                        send_crc_fail(pack);
                    }

                }
                pack->rx_state = 0;
                break;

        }
    }while((data == PACKET_START_BYTE || data == PACKET_START_BYTE+1) && good == 0);

}

static uint16_t crc_calc(packet2_t *pack, uint16_t crc_init, uint8_t *data, uint16_t len){
    if (pack->crc_func){
        return pack->crc_func(crc_init, data, (uint32_t)len);
    }else{
        return calc_crc16(data,(uint32_t)len,crc_init,0x0000);
    }

}



void packet_send_packet(packet2_t *pack, uint8_t *data, uint16_t len) {
    int32_t b_ind;
    if (len > pack->tx_buffer_size) {
        return;
    }
    uint16_t crc;
    b_ind = 0;

    if (len < 256) {
        pack->tx_buffer[b_ind++] = PACKET_START_BYTE;
        pack->tx_buffer[b_ind++] = len;
    } else {
        pack->tx_buffer[b_ind++] = PACKET_START_BYTE+1;
        pack->tx_buffer[b_ind++] = len >> 8;
        pack->tx_buffer[b_ind++] = len & 0xFF;
    }
    uint8_t hdr_crc = calc_crc8(pack->tx_buffer,b_ind, PACKET_CRC_HDR_SEED, 0);
    pack->tx_buffer[b_ind++] = hdr_crc;
    memcpy(&pack->tx_buffer[b_ind], data, len);
    crc = crc_calc(pack, pack->crc_init, &pack->tx_buffer[b_ind], len);
    b_ind += len;
    pack->tx_buffer[b_ind++] = (uint8_t)(crc >> 8);
    pack->tx_buffer[b_ind++] = (uint8_t)(crc & 0xFF);
    pack->tx_buffer[b_ind++] = PACKET_END_BYTE;

    if (pack->send_func) {
        pack->send_func(pack->custom_data,pack->tx_buffer, b_ind);
    }
}

void send_crc_fail(packet2_t *pack){

    pack->tx_buffer[0] = PACKET_CRC_FAIL;
    pack->tx_buffer[1] = PACKET_CRC_FAIL;
    pack->tx_buffer[2] = PACKET_CRC_FAIL;
    if (pack->send_func) {
        pack->send_func(pack->custom_data,pack->tx_buffer, 3);
    }

}