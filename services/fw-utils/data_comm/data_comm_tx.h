//
// Created by nico on 09/11/24.
//

#pragma once

#include <stdbool.h>
#include <stdint.h>
#include "data_comm.h"

typedef enum {
    DATA_COMM_TX_STATE_IDLE=0,
    DATA_COMM_TX_STATE_PREPARE_REQ_AND_WAIT,
    DATA_COMM_TX_STATE_WAIT_FOR_MESSAGES,
    DATA_COMM_TX_STATE_SEND_CRC,
    DATA_COMM_TX_STATE_SEND_PACKETS,
    DATA_COMM_TX_STATE_PEER_FINISH_WAIT,
    DATA_COMM_TX_STATE_ERROR,
    DATA_COMM_TX_STATE_DONE,
} data_comm_tx_state_t;


typedef int (*data_comm_tx_read_func_t)(uint32_t address, uint32_t size, uint8_t* buffer, void *context);
typedef int (*data_comm_tx_finish_func_t)(uint8_t status, uint8_t id, void *context);

typedef struct {
    uint32_t ready_timeout;
    uint32_t finished_timeout;
    uint32_t packets_timeout;

    uint32_t prepare_delay;
    uint32_t crc_delay;
    uint32_t error_delay;
    uint32_t packets_delay;
    uint32_t info_delay;


    uint32_t error_count;
    uint32_t crc_count;
    uint32_t packets_count;
} data_comm_tx_config_t;


typedef struct {
    const data_comm_tx_config_t* config;
    uint8_t peer_id;
    uint8_t *buffer;
    uint16_t buffer_size;
    uint16_t packet_size;
    uint32_t total_size;
    data_comm_tx_read_func_t read_func;
    data_comm_send_func_t send_func;
    data_comm_tx_finish_func_t finish_callback;
    data_comm_log_func_t log_func;
    uint16_t crc_seed;
    void* context;
} data_comm_tx_settings_t;;

typedef struct {

    data_comm_tx_settings_t settings;
    const data_comm_tx_config_t* config;


    data_comm_tx_state_t state;
    uint32_t requested_sequence;
    uint16_t requested_size;


    uint16_t burst_crc;
    uint16_t burst_index;
    uint16_t burst_sequence;
    uint32_t timeout_timer;
    uint32_t delay_timer;
    uint32_t count;
    uint8_t status;
    uint8_t error_status;
    uint8_t error_data[4];
    bool packets_sent;
    uint32_t now;
} data_comm_tx_t;

int data_comm_tx_init(data_comm_tx_t* self, const data_comm_tx_settings_t* settings);

int data_comm_tx_start(data_comm_tx_t* self, uint32_t time_ms);

void data_comm_tx_tick(data_comm_tx_t* self, uint32_t time_ms);

bool data_comm_tx_process_message(data_comm_tx_t* self, uint32_t time_ms, data_comm_msg_type_t type, uint8_t peer, const uint8_t* data, uint16_t size);

bool data_comm_tx_is_running(const data_comm_tx_t * self);