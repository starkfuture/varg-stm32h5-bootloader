/*!
 ****************************************************************************
 * @file CAN_MessageHandler.h
 * @brief Header file for the CAN Message Processor module.
 *
 * This module provides functionality to process incoming CAN messages and
 * execute corresponding actions based on message IDs. It acts as an interface
 * between the CAN hardware abstraction layer (HAL) and the application layer,
 * handling the interpretation and processing of application-specific CAN messages.
 ****************************************************************************
 */

#ifndef CAN_MESSAGE_PROCESSOR_H
#define CAN_MESSAGE_PROCESSOR_H

#include "bootloader.h"
#include "sf_can_hal.h"


/* Send DLCs */
// #define CAN_MSG_SEND_ECU_STATUS_RESPONSE_LENGTH                     8U
#define CAN_MSG_SEND_START_ACK_LENGTH                               3U
#define CAN_MSG_SEND_PACKET_REQUEST_LENGTH                          5U
#define CAN_MSG_SEND_COMPLETION_MESSAGE_LENGTH                      8U
#define CAN_MSG_SEND_ECU_STATUS_RESPONSE_LENGTH                     8U
#define CAN_MSG_SEND_ERROR_MESSAGE_LENGTH                           8U

/* Start ACK message */
#define CAN_MSG_SEND_START_ACK_BUFF_MAX_SIZE_BYTE_0_INDEX           1U
#define CAN_MSG_SEND_START_ACK_BUFF_MAX_SIZE_BYTE_1_INDEX           2U

/* Packet request message*/
#define CAN_MSG_SEND_PACKET_REQ_SEQUENCE_NUM_BYTE_0_INDEX           1U
#define CAN_MSG_SEND_PACKET_REQ_SEQUENCE_NUM_BYTE_1_INDEX           2U
#define CAN_MSG_SEND_PACKET_REQ_SEQUENCE_NUM_BYTE_2_INDEX           3U
#define CAN_MSG_SEND_PACKET_REQ_PACKETS_NUM_BYTE_INDEX              4U

/* Start msg from VCU */
#define CAN_MSG_RECV_START_MSG_FW_SIZE_BYTE_0_INDEX                 1U
#define CAN_MSG_RECV_START_MSG_FW_SIZE_BYTE_1_INDEX                 2U
#define CAN_MSG_RECV_START_MSG_FW_SIZE_BYTE_2_INDEX                 3U
#define CAN_MSG_RECV_START_MSG_FW_SIZE_BYTE_3_INDEX                 4U
#define CAN_MSG_RECV_START_MSG_MAX_BUFF_BYTE_0_INDEX                5U
#define CAN_MSG_RECV_START_MSG_MAX_BUFF_BYTE_1_INDEX                6U

/* Packet CRC */
#define CAN_MSG_RECV_PACKET_CRC_BYTE_0_INDEX                        1U
#define CAN_MSG_RECV_PACKET_CRC_BYTE_1_INDEX                        2U



#define CAN_MSG_ECU_CODE_BYTE_INDEX                     0U
#define CAN_MSG_ECU_STATUS_BYTE_INDEX                   1U
#define CAN_MSG_ECU_STATE_BYTE_INDEX                    2U
#define CAN_MSG_ECU_INSTALLED_FW_VERSION_BYTE_0         3U
#define CAN_MSG_ECU_INSTALLED_FW_VERSION_BYTE_1         4U
#define CAN_MSG_ECU_INSTALLED_FW_VERSION_BYTE_2         5U
#define CAN_MSG_ECU_INSTALLED_FW_VERSION_BYTE_3         6U
#define CAN_MSG_BOOT_VERSION_BYTE_INDEX                 7U

#define CAN_MSG_ECU_CODE_SIZE                           1U

#define CAN_MSG_RECV_DATA_OFFSET                        1U

#define CAN_MSG_SEND_ECU_STATUS_PERIOD_MS               100U

#define CAN_MSG_MAX_LENGTH                              8U

// Define CAN message IDs for receiving
typedef enum {
    CAN_MSG_RECV_REQUEST_RUN_MODE_ID                    = 0x0001F001,
    CAN_MSG_RECV_PREPARE_REQUEST_ID                     = 0x0001F100,
    CAN_MSG_RECV_INFO_MESSAGE_ID                        = 0x0001F102,
    CAN_MSG_RECV_BURST_CRC_ID                           = 0x0001F104,
    CAN_MSG_RECV_BURST_DATA_ID                          = 0x0001F105,
    CAN_MSG_RECV_DATABURST_COMPLETE_MESSAGE_ID          = 0x0001F106
} can_recv_msg_ids_e;

typedef enum {
    CAN_MSG_RECV_ECU_STATUS_REQUEST_INDEX = 0,
    CAN_MSG_RECV_ENTER_BOOTLOADER_MODE_INDEX,
    CAN_MSG_RECV_START_MESSAGE_INDEX,
    CAN_MSG_RECV_PACKET_REQUEST_INDEX,
    CAN_MSG_RECV_VERIFICATION_CRC_INDEX,
    CAN_MSG_RECV_DATA_PACKET_INDEX,
    CAN_MSG_RECV_ERROR_ACK_INDEX,
    CAN_MSG_RCV_MAX_MSG
} can_recv_msg_index_e;

// Define CAN message IDs for sending
typedef enum {
    CAN_MSG_SEND_ECU_STATUS_PERIODIC_ID 				= 0x0001F000,
    CAN_MSG_SEND_READY_REPORT_ID 					    = 0x0001F101,
    CAN_MSG_SEND_BURST_REQUEST_ID 						= 0x0001F103,
	CAN_MSG_SEND_COMPLETION_MESSAGE_ID					= 0x0001F107,
    CAN_MSG_SEND_ERROR_MESSAGE_ID 						= 0x0001F108,
    CAN_MSG_SEND_FINISH_REPORT_ID                       = 0x0001F109
} can_send_msg_ids_e;

/*!
 ****************************************************************************
 * @brief Processes an incoming CAN message frame.
 *
 * This function interprets the received CAN message based on its ID and
 * executes the corresponding action, such as controlling lights, indicators,
 * or handling requests for ADC values and version information.
 *
 * @param[in] can Pointer to the received CAN message frame.
 ****************************************************************************
 */
void can_message_handler_process_frame(const can_message_rx_t *frame, uint8_t ecu_id);
void can_message_handler_task(uint8_t ecu_id, uint8_t app_status, uint8_t boot_version);
int can_message_handler_send_bootloader_message(data_comm_msg_type_t type, uint8_t id, const uint8_t* header, uint16_t header_size, const uint8_t* data, uint16_t data_size, void *context);

#endif // CAN_MESSAGE_PROCESSOR_H
