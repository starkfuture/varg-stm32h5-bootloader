//
// Created by nico on 09/11/24.
//

#pragma once
#include <stdbool.h>
#include <stdint.h>

#include "data_comm.h"

typedef enum {
    DATA_COMM_RX_WRITE_SUCCESS=0,
    DATA_COMM_RX_WRITE_ERROR_REPEAT_LAST_BURST,
    DATA_COMM_RX_WRITE_ERROR_REPEAT_FROM_BEGGINING,
    DATA_COMM_RX_WRITE_ERROR_ABORT,
} data_comm_rx_write_result_t;

typedef enum {
    DATA_COMM_RX_FINISH_SUCCESS=0,
    DATA_COMM_RX_FINISH_ERROR_RETRY,
    DATA_COMM_RX_FINISH_ERROR_ABORT,
}data_comm_rx_finish_result_t;
typedef data_comm_rx_write_result_t (*data_comm_rx_write_func_t)(uint32_t address, const uint8_t* data, uint32_t size, bool last, void*context);
typedef data_comm_rx_finish_result_t (*data_comm_rx_fisnish_callback_t)(uint8_t status, uint8_t* finish_error, void*context);


typedef struct {
    uint32_t max_full_retries;
    uint32_t max_retries_burst;

    uint32_t prepare_timeout;
    uint32_t info_timeout;
    uint32_t packets_timeout;



    uint32_t ready_status_delay;

    uint32_t request_count;
    uint32_t request_delay;

    uint32_t complete_delay;
    uint32_t complete_count;

    uint32_t finish_delay;
    uint32_t finish_count;
}data_comm_rx_config_t;



typedef struct {
    const data_comm_rx_config_t* config;
    uint8_t* buffer;
    uint16_t buffer_size;
    uint8_t id;
    uint16_t crc_seed;
    data_comm_rx_write_func_t write_func;
    data_comm_send_func_t send_func;
    data_comm_rx_fisnish_callback_t finish_func;
    data_comm_log_func_t log_func;
    void* context;
}data_comm_rx_settings_t;


typedef enum {
    DATA_COMM_RX_STATE_IDLE=0,   //Wait for Offer or exit bootloader
    DATA_COMM_RX_STATE_SEND_READY_AND_WAIT_INFO,
    DATA_COMM_RX_STATE_SEND_BURST_REQ_AND_WAIT_BURST_CRC,
    DATA_COMM_RX_STATE_WAIT_BURST_DATA,
    DATA_COMM_RX_STATE_BURST_COMPLETE,
    DATA_COMM_RX_STATE_SEND_COMPLETE,
    DATA_COMM_RX_STATE_PROCESS,
    DATA_COMM_RX_STATE_SEND_FINISH_REPORT,
}data_comm_rx_state_t;

typedef struct {
    uint32_t total_size;
    data_comm_rx_state_t state;
    data_comm_rx_settings_t settings;
    uint32_t delay_timer;
    uint32_t count;
    uint32_t timeout_timer;
    uint8_t status;
    // bool send_info;

    // uint32_t data_index;
    uint16_t max_burst_size;
    bool started;

    // bool expecting_burst_complete;
    uint16_t burst_index;
    uint16_t burst_size;
    uint16_t requested_burst_size;
    uint16_t burst_crc;
    uint32_t expected_sequence;
    uint32_t burst_sequence;
    uint32_t burst_retries;

    uint32_t full_retries;
    uint32_t now;
} data_comm_rx_t;



int data_comm_rx_init(data_comm_rx_t* self, const data_comm_rx_settings_t* settings);

void data_comm_rx_tick(data_comm_rx_t* self, uint32_t time_ms);

bool data_comm_rx_process_message(data_comm_rx_t* self, uint32_t time_ms, data_comm_msg_type_t type, uint8_t id, const uint8_t* data, uint16_t size);

int data_comm_rx_start(data_comm_rx_t* self, uint32_t time_ms);

bool data_comm_rx_is_running(const data_comm_rx_t* self);

bool data_comm_rx_is_receiving(const data_comm_rx_t* self);