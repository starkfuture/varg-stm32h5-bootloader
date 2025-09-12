/*!
 ****************************************************************************
 * @file can_message_handler.c
 * @brief Implementation of the CAN Message Processor module.
 *
 * This module contains the implementation of functions to process incoming
 * CAN messages and execute appropriate actions based on message IDs.
 * It is part of the services layer and depends on the CAN HAL for message
 * reception and transmission.
 ****************************************************************************
 */
#include <string.h>
#include "can_message_handler.h"
#include "bootloader.h"
#include "sf_bootloader_hal.h"
#include "sf_can_hal.h"
#include "sf_timer_hal.h"


#define FDCAN_PERIPHERAL 1

/* Forward declaration */
static void can_message_handler_ECU_status_periodic(uint8_t ecu_id, uint8_t status, uint8_t boot_version);

void can_message_handler_init(void) {
    can_filter_message_t can_filter_message = {0};

    can_general_filter_t can_general_filter = (can_general_filter_t){
      .non_matching_std = SF_FDCAN_REJECT,
      .non_matching_ext = SF_FDCAN_REJECT,
      .reject_remote_std = 	SF_FDCAN_REJECT_REMOTE,
      .reject_remote_ext = 	SF_FDCAN_REJECT_REMOTE,
  };
  can_activate_notification_t can_activate_notification = (can_activate_notification_t){
      .rx_fifo0_interrupts = SF_FDCAN_IT_RX_FIFO0_NEW_MESSAGE,
  };

//   Configure the low-level filtering on the CAN peripheral
  can_filter_message = (can_filter_message_t){
      .identifier_type = SF_FDCAN_EXTENDED_ID,
      .filter_index = 0,
      .filter_type = SF_FDCAN_FILTER_RANGE,
      .filter_config = SF_FDCAN_FILTER_TO_RXFIFO0,
      .filter_id1 = CAN_MSG_RECV_REQUEST_RUN_MODE_ID,
      .filter_id2 = CAN_MSG_RECV_DATABURST_COMPLETE_MESSAGE_ID,
  };

//   Configure CAN ID filtering
  sf_can_configure_filters(FDCAN_PERIPHERAL, can_filter_message);

  // Configure general CAN ID filtering (non-specified messages)
  sf_can_configure_general_filter(FDCAN_PERIPHERAL, can_general_filter);

  // Activate RX FIFO0 new message notification
  sf_can_activate_notification(FDCAN_PERIPHERAL, can_activate_notification);

  // Start CAN peripheral
  sf_can_start(FDCAN_PERIPHERAL);
}

/*!
 ****************************************************************************
 * @brief Processes an incoming CAN message frame.
 *
 * This function interprets the received CAN message based on its ID and
 * executes the corresponding action, such as controlling lights, indicators,
 * or handling requests for ADC values and version information.
 *
 * @param[in] can_msg Pointer to the received CAN message frame.
 ****************************************************************************
 */
void can_message_handler_process_frame(const can_message_rx_t *frame, uint8_t ecu_id)
{
    if (ecu_id != frame->data[CAN_MSG_ECU_CODE_BYTE_INDEX] || frame->identifier_type==SF_FDCAN_STANDARD_ID){
        return;
    }

    switch (frame->identifier) {
        case CAN_MSG_RECV_REQUEST_RUN_MODE_ID: {
            uint8_t request_mode = frame->data[1];

            if (request_mode == BOOTLOADER_RUN_REQUEST_MODE_APPLICATION) {
                bootloader_start_app(true);
            } else if (request_mode == BOOTLOADER_RUN_REQUEST_MODE_BOOTLOADER) {
                bootloader_stay(true);
            }
            break;
        }

        case CAN_MSG_RECV_PREPARE_REQUEST_ID:
        case CAN_MSG_RECV_INFO_MESSAGE_ID:
        case CAN_MSG_RECV_BURST_CRC_ID:
        case CAN_MSG_RECV_BURST_DATA_ID:
        case CAN_MSG_RECV_DATABURST_COMPLETE_MESSAGE_ID: {
            data_comm_msg_type_t type = 0;
            const uint8_t *relevant_data_pointer = &(frame->data[1]);
            uint16_t size = frame->data_length - CAN_MSG_ECU_CODE_SIZE;

            if (frame->identifier == CAN_MSG_RECV_PREPARE_REQUEST_ID) {
                type = DATA_COMM_MSG_TYPE_PREPARE_REQUEST;
            } else if (frame->identifier == CAN_MSG_RECV_INFO_MESSAGE_ID) {
                type = DATA_COMM_MSG_TYPE_INFO;
            } else if (frame->identifier == CAN_MSG_RECV_BURST_CRC_ID) {
                type = DATA_COMM_MSG_TYPE_BURST_CRC;
            } else if (frame->identifier == CAN_MSG_RECV_BURST_DATA_ID) {
                type = DATA_COMM_MSG_TYPE_BURST_PACKET;
            } else if (frame->identifier == CAN_MSG_RECV_DATABURST_COMPLETE_MESSAGE_ID) {
                type = DATA_COMM_MSG_TYPE_BURST_COMPLETION;
            }

            bootloader_rx_message_received((uint32_t) get_1ms_counter(), type, ecu_id, relevant_data_pointer, size);
            break;
        }

        default:
            break;
    }
}

static void can_message_handler_ECU_status_periodic(uint8_t ecu_id, uint8_t app_status, uint8_t boot_version)
{
    static uint32_t last_send_ms = 0U;
    uint32_t now = get_1ms_counter();

    if ((now - last_send_ms) >= CAN_MSG_SEND_ECU_STATUS_PERIOD_MS)
    {
        last_send_ms = now;

        can_message_tx_t status_frame = {0};

        status_frame.identifier = CAN_MSG_SEND_ECU_STATUS_PERIODIC_ID;
        status_frame.data_length = CAN_MSG_SEND_ECU_STATUS_RESPONSE_LENGTH;
        status_frame.identifier_type = SF_FDCAN_EXTENDED_ID;
        status_frame.tx_frame_type = SF_FDCAN_DATA_FRAME;

        status_frame.data[CAN_MSG_ECU_CODE_BYTE_INDEX] = ecu_id;
        status_frame.data[CAN_MSG_ECU_STATUS_BYTE_INDEX] = 0x00;
        status_frame.data[CAN_MSG_ECU_STATE_BYTE_INDEX] = app_status;

        uint32_t fw_version = bootloader_get_installed_fw_version();
        status_frame.data[CAN_MSG_ECU_INSTALLED_FW_VERSION_BYTE_0] = (uint8_t)(fw_version & 0xFF);
        status_frame.data[CAN_MSG_ECU_INSTALLED_FW_VERSION_BYTE_1] = (uint8_t)((fw_version >> 8) & 0xFF);
        status_frame.data[CAN_MSG_ECU_INSTALLED_FW_VERSION_BYTE_2] = (uint8_t)((fw_version >> 16) & 0xFF);
        status_frame.data[CAN_MSG_ECU_INSTALLED_FW_VERSION_BYTE_3] = (uint8_t)((fw_version >> 24) & 0xFF);
        status_frame.data[CAN_MSG_BOOT_VERSION_BYTE_INDEX] = boot_version;

        sf_can_send_message(FDCAN_PERIPHERAL, status_frame);
    }
}

void can_message_handler_task(uint8_t ecu_id, uint8_t app_status, uint8_t boot_version)
{
    can_message_rx_t new_msg;
    uint8_t can_msg_rcv_data[MAX_DATA_LENGTH];
    new_msg.data = &can_msg_rcv_data[0];

    /* Get lastest message received */
    while(sf_can_get_last_rx_filtered_message(&new_msg) == CAN_STATUS_OK) {
        can_message_handler_process_frame(&new_msg, ecu_id);
    }

    can_message_handler_ECU_status_periodic(ecu_id, app_status, boot_version);
}


//int can_message_handler_send_bootloader_message(data_comm_msg_type_t type, uint8_t id, const uint8_t* header, uint16_t header_size, const uint8_t* data, uint16_t data_size, void *context)
//{
//	/* Suppress unused parameters */
//    (void)context;
//    (void)data_size;
//    (void)data;
//
//    can_message_tx_t frame = {0};
//    frame.identifier_type = SF_FDCAN_EXTENDED_ID;
//
//    /* Set ECU ID as the first byte of data */
//    frame.data[CAN_MSG_ECU_CODE_BYTE_INDEX] = id;
//
//    switch (type)
//    {
//        case DATA_COMM_MSG_TYPE_READY_REPORT:
//        {
//            frame.identifier = CAN_MSG_SEND_READY_REPORT_ID;
//            frame.data_length = CAN_MSG_ECU_CODE_SIZE + header_size;
//            if (header_size > (CAN_MSG_MAX_LENGTH - CAN_MSG_ECU_CODE_SIZE)) {
//                return -2;
//            }
//
//            memcpy(&frame.data[1], header, header_size);
//            break;
//        }
//
//        case DATA_COMM_MSG_TYPE_BURST_REQUEST:
//        {
//            frame.identifier = CAN_MSG_SEND_BURST_REQUEST_ID;
//            frame.data_length = CAN_MSG_ECU_CODE_SIZE + header_size;
//
//            if (header_size > (CAN_MSG_MAX_LENGTH - CAN_MSG_ECU_CODE_SIZE)) {
//                return -2;
//            }
//
//            memcpy(&frame.data[1], header, header_size);
//            break;
//        }
//
//        case DATA_COMM_MSG_TYPE_COMPLETION:
//        {
//            frame.identifier = CAN_MSG_SEND_COMPLETION_MESSAGE_ID;
//            frame.data_length = CAN_MSG_ECU_CODE_SIZE;
//            break;
//        }
//
//        case DATA_COMM_MSG_TYPE_FINISH_REPORT:
//        {
//            frame.identifier = CAN_MSG_SEND_FINISH_REPORT_ID;
//            frame.data_length = CAN_MSG_ECU_CODE_SIZE + header_size;
//
//            if (header_size > (CAN_MSG_MAX_LENGTH - CAN_MSG_ECU_CODE_SIZE)) {
//                return -2;
//            }
//
//            memcpy(&frame.data[1], header, header_size);
//            break;
//        }
//
//        default:
//        {
//            return -1;  /* Unknown message type */
//        }
//    }
//
//    /* Send the frame */
//    can_status_t ret = sf_can_send_message(FDCAN_PERIPHERAL, frame);
//
//    return (ret == CAN_STATUS_OK) ? 0 : -1;
//
//}



