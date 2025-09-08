//
// Created by nico on 09/11/24.
//

#pragma once
#include <stdint.h>

#define DATA_COMM_STATUS_OK                         (0x00)
#define DATA_COMM_STATUS_PREPARE_TIMEOUT            (0x01)
#define DATA_COMM_STATUS_READY_TIMEOUT              (0x02)
#define DATA_COMM_STATUS_INFO_TIMEOUT               (0x03)
#define DATA_COMM_STATUS_FINISH_TIMEOUT             (0x04)
#define DATA_COMM_STATUS_REQUEST_TIMEOUT            (0x05)
#define DATA_COMM_STATUS_MAX_INFO_RETRIES           (0x06)
#define DATA_COMM_STATUS_MAX_BURST_RETRIES          (0x07)
#define DATA_COMM_STATUS_MAX_FULL_RETRIES           (0x08)
#define DATA_COMM_STATUS_UNEXPECTED_COMPLETION      (0x09)
#define DATA_COMM_STATUS_CRC_MISMATCH               (0x0A)
#define DATA_COMM_STATUS_PEER_ERROR                 (0x0B)
#define DATA_COMM_ERROR_COMM_ERROR                  (0x0C)
#define DATA_COMM_ERROR_READ_ERROR                  (0x0D)
#define DATA_COMM_ERROR_BUFFER_TOO_SMALL            (0x0E)
#define DATA_COMM_ERROR_SEQUENCE_OOB                (0x0F)
#define DATA_COMM_ERROR_BURST_SIZE_TOO_BIG          (0x10)
#define DATA_COMM_ERROR_UNKNOWN_WRITE_RESULT        (0x11)
#define DATA_COMM_ERROR_UNKNOWN                     (0xFF)

#define DATA_COMM_PEER_STATUS_REQ_PREPARE           (0xFF)
#define DATA_COMM_PEER_STATUS_RSP_READY             (0xFF)
#define DATA_COMM_PEER_STATUS_REQ_IS_SUCCESS        (0x00)
#define DATA_COMM_PEER_STATUS_RSP_SUCCEED           (0x00)


typedef enum {
    DATA_COMM_MSG_TYPE_PREPARE_REQUEST=0,
    DATA_COMM_MSG_TYPE_READY_REPORT,
    DATA_COMM_MSG_TYPE_INFO,
    DATA_COMM_MSG_TYPE_BURST_REQUEST,
    DATA_COMM_MSG_TYPE_BURST_CRC,
    DATA_COMM_MSG_TYPE_BURST_PACKET,
    DATA_COMM_MSG_TYPE_BURST_COMPLETION,
    DATA_COMM_MSG_TYPE_COMPLETION,
    DATA_COMM_MSG_TYPE_ERROR_RESPONSE,
    DATA_COMM_MSG_TYPE_FINISH_REPORT,
}data_comm_msg_type_t;

#define DATA_COMM_LOG_LEVEL_DEBUG   0x00
#define DATA_COMM_LOG_LEVEL_INFO    0x01
#define DATA_COMM_LOG_LEVEL_WARNING 0x02
#define DATA_COMM_LOG_LEVEL_ERROR   0x03


#ifndef DATA_COMM_LOG_LEVEL
#define DATA_COMM_LOG_LEVEL 0xFF
#endif


typedef int (*data_comm_send_func_t)(data_comm_msg_type_t type, uint8_t id, const uint8_t* header, uint16_t header_size, const uint8_t* data, uint16_t data_size, void *context);
typedef int (*data_comm_log_func_t)(int level, const char *format, ...);


#pragma pack(push,1)

typedef struct {
    uint8_t status;
} data_comm_msg_ready_t;

typedef struct {
    uint8_t status;
    uint8_t data[4];
} data_comm_msg_error_t;


typedef struct {
    uint32_t total_size;
    uint16_t buffer_size;
} data_comm_msg_info_t;

typedef struct {
    uint8_t sequence[3];
    uint16_t crc;
    uint16_t burst_size;
} data_comm_msg_crc_t;

typedef struct {
    uint8_t sequence[3];
} data_comm_msg_packet_header_t;

typedef struct {
    uint8_t sequence[3];
    uint16_t burst_size;
} data_comm_msg_burst_complete_t;

typedef struct {
    uint8_t sequence[3];
    uint16_t burst_size;
} data_comm_burst_request_t;

typedef struct {
    uint8_t status;
} data_comm_status_t;

#pragma pack(pop)



// void buffer_append_uint24_little(uint32_t source, uint8_t *dst);
// uint32_t buffer_extract_uint24_little(uint8_t const *src);
//
//
// uint16_t calc_crc16(const uint8_t *bytes, uint32_t size, uint16_t seed, uint16_t crc);
