//
// Created by nico on 09/11/24.
//

#include "data_comm_rx.h"

#include <assert.h>
#include <string.h>

#include "../utils.h"

#if DATA_COMM_LOG_LEVEL <= DATA_COMM_LOG_LEVEL_DEBUG
#define LOG_D(fmt,...) \
self->settings.log_func(DATA_COMM_LOG_LEVEL_DEBUG,fmt "\n",##__VA_ARGS__)
#else
#define LOG_D(fmt,...)
#endif

#if DATA_COMM_LOG_LEVEL <= DATA_COMM_LOG_LEVEL_INFO
#define LOG_I(fmt,...) \
self->settings.log_func(DATA_COMM_LOG_LEVEL_INFO,fmt "\n",##__VA_ARGS__)
#else
#define LOG_I(fmt,...)
#endif

#if DATA_COMM_LOG_LEVEL <= DATA_COMM_LOG_LEVEL_WARNING
#define LOG_W(fmt,...) \
self->settings.log_func(DATA_COMM_LOG_LEVEL_WARNING,fmt "\n",##__VA_ARGS__)
#else
#define LOG_W(fmt,...)
#endif


#if DATA_COMM_LOG_LEVEL <= DATA_COMM_LOG_LEVEL_ERROR
#define LOG_E(fmt,...) \
self->settings.log_func(DATA_COMM_LOG_LEVEL_ERROR,fmt "\n",##__VA_ARGS__)
#else
#define LOG_E(fmt,...)
#endif




static void set_state(data_comm_rx_t* self, data_comm_rx_state_t state) {
    assert(self);
    if (self->state!=state) {
        LOG_D("[%d] State %d->%d\n",self->now, self->state,state);
    }
    self->state=state;
    self->timeout_timer=self->now;
    self->count=0;
    self->delay_timer=self->now;
}

static bool check_delay(data_comm_rx_t* self, uint32_t delay) {
    if (self->now - self->delay_timer<=delay) {
        return false;
    }
    // LOG_D("[%d] Delay %d finished\n",self->now, delay);
    self->delay_timer=self->now;
    return true;
}

static bool check_timeout(data_comm_rx_t* self, uint32_t timeout) {
    if (self->now - self->timeout_timer<=timeout) {
        return false;
    }
    LOG_D("[%d] Timeout %d\n",self->now, timeout);
    return true;
}

static bool check_count(data_comm_rx_t* self, uint32_t count) {
    if (self->count<count) {
        self->count++;
        if (self->count==count) {
            LOG_D("[%d] Count %d reached\n",self->now, count);
            return true;
        }
        return false;
    }
    return true;
}

static void request_burst(data_comm_rx_t* self, uint32_t sequence) {
    self->burst_size=0;
    self->burst_sequence=sequence;
    self->burst_index=0;
    self->expected_sequence=self->burst_sequence;
    set_state(self,DATA_COMM_RX_STATE_SEND_BURST_REQ_AND_WAIT_BURST_CRC);
}

static bool retry_burst(data_comm_rx_t* self, uint32_t sequence) {

    const data_comm_rx_config_t* config = self->settings.config;
    // const data_comm_rx_settings_t* settings = &self->settings;
    if (self->burst_retries<config->max_retries_burst) {
        LOG_I("[%d] %d. Retry burst %d/%d", self->now, self->state,self->burst_retries,config->max_retries_burst);
        self->burst_retries++;
        request_burst(self,sequence);
        return true;
    }
    LOG_E("[%d] %d. Max burst retries %d/%d", self->now, self->state,self->burst_retries,config->max_retries_burst);
    return false;
}

static bool retry_full(data_comm_rx_t* self) {

    const data_comm_rx_config_t* config = self->settings.config;
    // const data_comm_rx_settings_t* settings = &self->settings;
    if (self->full_retries<config->max_full_retries) {
        LOG_I("[%d] %d. Retry full %d/%d", self->now, self->state,self->full_retries,config->max_full_retries);
        self->full_retries++;
        self->burst_retries=0;
        request_burst(self,0);
        return true;
    }
    LOG_E("[%d] %d. Max full retries %d/%d", self->now, self->state,self->full_retries,config->max_full_retries);
    return false;
}

bool data_comm_rx_is_running(const data_comm_rx_t* self) {
    if (self->state!=DATA_COMM_RX_STATE_IDLE || self->started) {
        return true;
    }
    return false;
}

bool data_comm_rx_is_receiving(const data_comm_rx_t* self) {
    if (self->state>DATA_COMM_RX_STATE_SEND_READY_AND_WAIT_INFO) {
        return true;
    }
    return false;
}

void data_comm_rx_tick(data_comm_rx_t* self, uint32_t time_ms) {
    assert(self);
    assert(self->settings.config);
    assert(self->settings.finish_func);
    assert(self->settings.send_func);
    assert(self->settings.buffer);
    assert(self->settings.buffer_size!=0);
    self->now=time_ms;
    const data_comm_rx_config_t* config = self->settings.config;
    const data_comm_rx_settings_t* settings = &self->settings;
    switch (self->state) {
        case DATA_COMM_RX_STATE_IDLE: {
            //In Idle
            if (!self->started) {
                //Doing nothing, not even started
                self->timeout_timer=time_ms;
                break;
            }

            //It was started, now we check the timeout
            if (config->prepare_timeout && check_timeout(self,config->prepare_timeout)) {
                LOG_W("[%d] %s : %d. Timeout, didn't receive Prepare", self->now, __FUNCTION__, self->state);

                //If there was no Prepare message, then Abort everything
                self->status = DATA_COMM_STATUS_PREPARE_TIMEOUT;
                set_state(self,DATA_COMM_RX_STATE_PROCESS);
            }
            break;
        }
        case DATA_COMM_RX_STATE_SEND_READY_AND_WAIT_INFO:{
            //While we send the ready message, we wait for the INFO
            if (check_timeout(self,config->info_timeout)) {
                //Timeout expired, Abort
                LOG_W("[%d] %s : %d. Timeout, didn't receive Info", self->now, __FUNCTION__, self->state);

                self->status = DATA_COMM_STATUS_INFO_TIMEOUT;
                set_state(self,DATA_COMM_RX_STATE_PROCESS);
                break;
            }

            if (!check_delay(self, config->ready_status_delay)) {
                //Wait the delay before sending the message
                break;
            }
            //Delay expired, send the READY message
            LOG_I("[%d] %s : %d. Sending Ready", self->now, __FUNCTION__, self->state);

            data_comm_msg_ready_t msg_ready = {
                .status =DATA_COMM_PEER_STATUS_RSP_READY
            };
            int result = settings->send_func(DATA_COMM_MSG_TYPE_READY_REPORT,settings->id,
                (const uint8_t*) &msg_ready,sizeof(msg_ready),NULL,0,settings->context);
            if (result!=0) {
                //There was an error sending the data, Abort
                LOG_E("[%d] %s : %d. Error sending %d", self->now, __FUNCTION__, self->state,result);
                self->status=DATA_COMM_ERROR_COMM_ERROR;
                set_state(self,DATA_COMM_RX_STATE_PROCESS);
                break;
            }



            break;
        }
        case DATA_COMM_RX_STATE_SEND_BURST_REQ_AND_WAIT_BURST_CRC: {
            //Wait for the delay
            if (check_delay(self,config->request_delay)) {
                //Delay expired
                if (!check_count(self,config->request_count)) {
                    //Count hasn't reached max, send the message
                    //While sending, reset the timeout
                    self->timeout_timer=time_ms;

                    //Calculate the requested burst_size
                    //Try to request the rest of the size
                    self->requested_burst_size= self->total_size - self->burst_sequence;
                    //But if it's too big:
                    if (self->requested_burst_size>self->max_burst_size) {
                        //Just request the max_burst_size calculated on the INFO reception
                        self->requested_burst_size = self->max_burst_size;
                    }
                    //Send the message
                    data_comm_burst_request_t msg_burst_request = {
                        .burst_size = self->requested_burst_size
                    };
                    buffer_append_uint24_little(self->burst_sequence, msg_burst_request.sequence);
                    LOG_I("[%d] %s : %d. Sending Burst Req 0x%08x, %d",
                        self->now, __FUNCTION__, self->state, self->burst_sequence, self->requested_burst_size);

                    int result = settings->send_func(DATA_COMM_MSG_TYPE_BURST_REQUEST,settings->id,
                            (const uint8_t*) &msg_burst_request,sizeof(msg_burst_request),NULL,0,settings->context);
                    if (result!=0) {
                        //There was an error sending, abort
                        self->status=DATA_COMM_ERROR_COMM_ERROR;
                        set_state(self,DATA_COMM_RX_STATE_PROCESS);
                        break;
                    }
                    break;
                }
            }
            //If after sending the burst request, there's no response
            if (check_timeout(self,config->packets_timeout)) {
                //Timet out
                //Timet out, try to retry
                if (!retry_burst(self,self->burst_sequence)) {
                    //No more retries, abort

                    self->status = DATA_COMM_STATUS_REQUEST_TIMEOUT;
                    set_state(self,DATA_COMM_RX_STATE_PROCESS);
                }
            }
            break;
        }
        case DATA_COMM_RX_STATE_WAIT_BURST_DATA: {
            //Here we just wait for messages
            if (check_timeout(self,config->packets_timeout)) {
                LOG_W("[%d] %s : %d. Timeout, didn't receive packets", self->now, __FUNCTION__, self->state);
                //Timet out, try to retry
                if (!retry_burst(self,self->burst_sequence)) {
                    //No more retries, abort

                    self->status = DATA_COMM_STATUS_REQUEST_TIMEOUT;
                    set_state(self,DATA_COMM_RX_STATE_PROCESS);
                }
            }
            break;
        }
        case DATA_COMM_RX_STATE_BURST_COMPLETE: {
            //We have a complete burst
            uint16_t crc = calc_crc16(settings->buffer, self->burst_size,settings->crc_seed,0);
            if (crc!=self->burst_crc) {
                //CRC MISMATCH, try to restart
                LOG_W("[%d] %s : %d. CRC Mismatch %04x != %04x",
                    self->now, __FUNCTION__, self->state,crc,self->burst_crc);

                if (!retry_burst(self,self->burst_sequence)) {
                    //Max retry reached, abort
                    self->status = DATA_COMM_STATUS_CRC_MISMATCH;
                    set_state(self,DATA_COMM_RX_STATE_PROCESS);
                }
                break;
            }

            uint32_t new_seq = self->burst_sequence+self->burst_size;
            bool last_burst = new_seq >= self->total_size;

            //CRC is OK, send the buffer to the app
            LOG_D("[%d] %s : %d. Calling Write func", self->now, __FUNCTION__, self->state);

            data_comm_rx_write_result_t result = settings->write_func(self->burst_sequence,settings->buffer,self->burst_size,last_burst, settings->context);
            //What does the app wants to do?
            if (result==DATA_COMM_RX_WRITE_SUCCESS) {
                // App wants to continue
                //Calculate the new_sequence based on the completed burst
                LOG_I("[%d] %s : %d. Write func succees", self->now, __FUNCTION__, self->state);

                // assert(new_seq<=self->total_size); //this should be ok, just in case
                //Check if this was the last burst
                if (last_burst) {
                    LOG_I("[%d] %s : %d. Burst complete %d = %d",
                        self->now, __FUNCTION__, self->state, new_seq, self->total_size);

                    // Yes, it was the last burst, send the complete message
                    set_state(self,DATA_COMM_RX_STATE_SEND_COMPLETE);
                } else {
                    //No, it was not the last, we need to keep requesting
                    LOG_I("[%d] %s : %d. Burst %d/%d", self->now, __FUNCTION__, self->state, new_seq, self->total_size);
                    self->burst_retries=0;
                    request_burst(self,new_seq);
                }
            }
            else if (result==DATA_COMM_RX_WRITE_ERROR_REPEAT_LAST_BURST) {
                //The app wants to retry the last burst
                LOG_I("[%d] %s : %d. Write func repeat last", self->now, __FUNCTION__, self->state);

                if (!retry_burst(self,self->burst_sequence)){
                    //Max retries reached
                    self->status = DATA_COMM_STATUS_MAX_BURST_RETRIES;
                    set_state(self,DATA_COMM_RX_STATE_PROCESS);
                }
            }
            else if (result==DATA_COMM_RX_WRITE_ERROR_REPEAT_FROM_BEGGINING) {
                //The app wants to retry from the begining
                LOG_I("[%d] %s : %d. Write func full retry", self->now, __FUNCTION__, self->state);


                if (!retry_full(self)) {
                    self->status=DATA_COMM_STATUS_MAX_FULL_RETRIES;
                    set_state(self,DATA_COMM_RX_STATE_SEND_FINISH_REPORT);
                }

            }
            else {
                //The result from the app is unknown, abort
                LOG_E("[%d] %s : %d. Write func unknown", self->now, __FUNCTION__, self->state);

                self->status=DATA_COMM_ERROR_UNKNOWN_WRITE_RESULT;
                set_state(self,DATA_COMM_RX_STATE_PROCESS);
            }
            break;
        }
        case DATA_COMM_RX_STATE_SEND_COMPLETE: {
            //Wait between sending messages
            if (!check_delay(self,config->complete_delay)) {
                break;
            }
            //Delay expired

            //Check if count reached maximum
            if (!check_count(self,config->complete_count)) {
                //We still need to send messaged
                LOG_I("[%d] %s : %d. Sending Completion", self->now, __FUNCTION__, self->state);

                int result = settings->send_func(DATA_COMM_MSG_TYPE_COMPLETION,settings->id,NULL,0,NULL,0,settings->context);
                if (result!=0) {
                    LOG_E("[%d] %s : %d. Error sending (completion) %d", self->now, __FUNCTION__, self->state,result);
                    //Error in comm, abort
                    self->status = DATA_COMM_ERROR_COMM_ERROR;
                    set_state(self,DATA_COMM_RX_STATE_PROCESS);
                    break;
                }
                break;
            }
            //Done sending
            self->status=DATA_COMM_STATUS_OK;
            set_state(self,DATA_COMM_RX_STATE_PROCESS);
            break;
        }
        case DATA_COMM_RX_STATE_PROCESS: {
            // Here the app can process errors, or the completion
            uint8_t status=DATA_COMM_ERROR_UNKNOWN;
            //Call the function
            LOG_I("[%d] %s : %d. Calling finish with status %02x", self->now, __FUNCTION__, self->state,self->status);

            data_comm_rx_finish_result_t result = settings->finish_func(self->status,&status,settings->context);
            if (self->status!=DATA_COMM_STATUS_OK) {
                LOG_I("[%d] %s : %d. Finish said %d, but aborting because status %02x !=0 ", self->now, __FUNCTION__, self->state,result, self->status);
                result = DATA_COMM_RX_FINISH_ERROR_ABORT;
            }
            //Check what the app wants to do
            if (result==DATA_COMM_RX_FINISH_SUCCESS) {
                LOG_I("[%d] %s : %d. Finish => success", self->now, __FUNCTION__, self->state);

                //Finished successfully
                self->status=0x00;
                set_state(self,DATA_COMM_RX_STATE_SEND_FINISH_REPORT);
            } else if (result==DATA_COMM_RX_FINISH_ERROR_RETRY) {
                //The app wants to retry from the begining
                LOG_I("[%d] %s : %d. Finish => retry", self->now, __FUNCTION__, self->state);

                if (!retry_full(self)) {
                    //Max retries reached
                    self->status=DATA_COMM_STATUS_MAX_FULL_RETRIES;
                    set_state(self,DATA_COMM_RX_STATE_SEND_FINISH_REPORT);
                }

            } else {
                LOG_I("[%d] %s : %d. Finish => abort %02x", self->now, __FUNCTION__, self->state, status);

                //App has an error and wants to abort
                self->status=status;
                set_state(self,DATA_COMM_RX_STATE_SEND_FINISH_REPORT);
            }

            break;
        }
        case DATA_COMM_RX_STATE_SEND_FINISH_REPORT: {
            //Here we send the FINISH report
            //Wait until the delay expired
            if (!check_delay(self,config->finish_delay)) {
                break;
            }
            //Delay expired. max count reached?
            if (!check_count(self,config->finish_count)) {
                //Still sending messages
                data_comm_status_t status_msg = {
                    .status = self->status
                };
                LOG_I("[%d] %s : %d. Sending Finish %02x", self->now, __FUNCTION__, self->state,self->status);

                settings->send_func(DATA_COMM_MSG_TYPE_FINISH_REPORT,settings->id,
                    (const uint8_t*)&status_msg,sizeof(status_msg),NULL,0,settings->context);
                //Do not check for errors, since this is the last step
                break;
            }
            //Done sending
            set_state(self,DATA_COMM_RX_STATE_IDLE);
            self->started=false;
            break;
        }
    }


}

bool data_comm_rx_process_message(data_comm_rx_t* self, uint32_t time_ms, data_comm_msg_type_t type, uint8_t id, const uint8_t* data,
    uint16_t size) {
    assert(self);
    self->now=time_ms;
    // const data_comm_rx_config_t* config = self->settings.config;
    const data_comm_rx_settings_t* settings = &self->settings;

    if (id!=settings->id) {
        LOG_D("[%d] %s : Unexpected id 0x%02x != 0x%02x", self->now, __FUNCTION__, id,settings->id);

        return false;
    }

    switch(self->state){
        case DATA_COMM_RX_STATE_IDLE: {
            if (!self->started) {
                //If it hasn't started, just ignore any messages
                return false;
            }

            if (type!=DATA_COMM_MSG_TYPE_PREPARE_REQUEST) {
                //If it's not the prepare request, ignore it
                LOG_D("[%d] %s : %d. Unexpected msg in IDLE: %d", self->now, __FUNCTION__, self->state,type);

                return false;
            }
            LOG_I("[%d] %s : %d. RX Prepare %02x", self->now, __FUNCTION__, self->state, id);
            self->burst_index=0;
            self->burst_sequence=0;
            self->burst_retries=0;
            self->burst_size=0;
            self->burst_crc=0;
            self->max_burst_size=0;
            self->total_size=0;
            self->expected_sequence=0;
            self->status = DATA_COMM_ERROR_UNKNOWN;
            //Change state to send ready message and wait for info
            set_state(self,DATA_COMM_RX_STATE_SEND_READY_AND_WAIT_INFO);
            return true;
        }
        case DATA_COMM_RX_STATE_SEND_READY_AND_WAIT_INFO: {
            if (type!=DATA_COMM_MSG_TYPE_INFO) {
                LOG_D("[%d] %s : %d. Unexpected msg in WAIT INFO: %d", self->now, __FUNCTION__, self->state,type);

                //We expect only INFO in this state, anything else won't be consumed
                return false;
            }
            if (size<sizeof(data_comm_msg_info_t)) {
                LOG_D("[%d] %s : %d. msg: %d size too small %d<%d",
                    self->now, __FUNCTION__, self->state,type, size,sizeof(data_comm_msg_info_t));

                //Ignore any message that doesn't have the bytes
                return false;
            }
            //The message arrived
            const data_comm_msg_info_t* msg_info = (const data_comm_msg_info_t*)data;
            //Save the info
            self->total_size = msg_info->total_size;
            //Calculate the max burst size based on the minimum between each buffer
            self->max_burst_size = msg_info->buffer_size;
            if (self->max_burst_size>settings->buffer_size) {
                self->max_burst_size = settings->buffer_size;
            }
            LOG_I("[%d] %s : %d. Info size: %d, max_burst: %d final: %d",
                self->now, __FUNCTION__, self->state,msg_info->total_size,msg_info->buffer_size,self->max_burst_size);
            self->burst_size=0;
            self->burst_crc=0;
            self->burst_sequence=0;
            //Jump to start requesting
            set_state(self,DATA_COMM_RX_STATE_SEND_BURST_REQ_AND_WAIT_BURST_CRC);
            break;
        }
        case DATA_COMM_RX_STATE_SEND_BURST_REQ_AND_WAIT_BURST_CRC: {
            if (type!=DATA_COMM_MSG_TYPE_BURST_CRC) {
                //In this state we only wait for CRCs
                LOG_D("[%d] %s : %d. Unexpected msg in WAIT CRC: %d", self->now, __FUNCTION__, self->state,type);

                return false;
            }
            if (size<sizeof(data_comm_msg_crc_t)) {
                //Message not big enough
                LOG_D("[%d] %s : %d. msg: %d size too small %d<%d",
                    self->now, __FUNCTION__, self->state,type, size,sizeof(data_comm_msg_crc_t));

                return false;
            }
            //We received a valid crc
            const data_comm_msg_crc_t* msg_crc = (const data_comm_msg_crc_t*) data;
            //extract the sequence
            uint32_t crc_sequence = buffer_extract_uint24_little(msg_crc->sequence);
            if (crc_sequence!=self->burst_sequence) {
                LOG_W("[%d] %s : %d. Sequence mismatch in CRC: rx %08x, expected: %08x",
                    self->now, __FUNCTION__, self->state,type, crc_sequence,self->burst_sequence);

                //Not the sequence we were expecting
                return false;
            }
            //Check the burst size, we're allowing to send a smaller than requested burst, but not higher
            if (msg_crc->burst_size>self->requested_burst_size) {
                //It's higher, try to retry
                LOG_W("[%d] %s : %d. Size too big: rx %08x > requested: %08x",
                    self->now, __FUNCTION__, self->state,type, msg_crc->burst_size, self->requested_burst_size);

                if (!retry_burst(self,self->burst_sequence)) {
                    //Can't retry, max retries reached, abort
                    self->status=DATA_COMM_ERROR_BURST_SIZE_TOO_BIG;
                    set_state(self,DATA_COMM_RX_STATE_PROCESS);
                }
                return true;
            }
            //CRC ok and valid

            //Save the CRC for later
            self->burst_crc=msg_crc->crc;
            //Save the burst_size for later
            self->burst_size=msg_crc->burst_size;
            //Reset the burst_index (position in the buffer)

            LOG_I("[%d] %s : %d. CRC received seq: %08x, crc: %04x, size %d",
                self->now, __FUNCTION__, self->state,type, self->burst_sequence,msg_crc->crc,msg_crc->burst_size);

            self->burst_index=0;
            //Set the expected sequence
            self->expected_sequence=self->burst_sequence;
            //Jump to wait Burst data
            set_state(self,DATA_COMM_RX_STATE_WAIT_BURST_DATA);
            return true;
        }
        case DATA_COMM_RX_STATE_WAIT_BURST_DATA: {
            //In this state we should receive the data from the burst or a completion
            if (type==DATA_COMM_MSG_TYPE_BURST_PACKET) {
                // Burst data packet
                if (size<sizeof(data_comm_msg_packet_header_t)) {
                    //Not big enough to handle the header
                    LOG_D("[%d] %s : %d. msg: %d size too small %d<%d",
                        self->now, __FUNCTION__, self->state,type, size,sizeof(data_comm_msg_packet_header_t));

                    return false;
                }
                const data_comm_msg_packet_header_t* header = (const data_comm_msg_packet_header_t*) data;
                //Calculate the size of the data
                uint16_t data_size = size-sizeof(data_comm_msg_packet_header_t);

                if (data_size==0) {
                    //Size is 0, ignore this message
                    LOG_D("[%d] %s : %d. msg: %d no data",
                    self->now, __FUNCTION__, self->state,type);

                    return false;
                }


                //packet seems ok, extract sequence
                uint32_t sequence = buffer_extract_uint24_little(header->sequence);

                if (sequence!=self->expected_sequence) {
                    //Not the expected sequence
                    LOG_W("[%d] %s : %d. Sequence mismatch in WAIT DATA (PACKET): rx %08x, expected: %08x",
                        self->now, __FUNCTION__, self->state, sequence,self->expected_sequence);
                    return false;
                }
                if ((self->burst_index+data_size)>self->burst_size) {
                    //The data overflows the accorded burst size, ignore this message
                    LOG_W("[%d] %s : %d. Sequence %08x overflows: current %08x, data_size: %d, total: %d",
                        self->now, __FUNCTION__, self->state, sequence,self->burst_index,data_size,self->burst_size);
                    return false;
                }
                //Packet is OK
                const uint8_t* packet_data = &data[sizeof(data_comm_msg_packet_header_t)];
                //Copy the data to the correct position inside the buffer
                memcpy(&settings->buffer[self->burst_index], packet_data,data_size);
                //Update the burst index;
                self->burst_index+=data_size;
                //Update the next expected sequence
                self->expected_sequence+=data_size;
                //Check if this was the last part of the burst
                LOG_I("[%d] %s : %d. Data 0x%08x, size: %d",
                    self->now, __FUNCTION__,self->state, sequence,data_size);

                if (self->burst_index == self->burst_size) {
                    //it was the last part, jump to BURST_COMPLETE
                    LOG_I("[%d] %s : %d. Burst complete 0x%08x, size: %d",
                        self->now, __FUNCTION__,self->state,self->burst_sequence,self->burst_size);

                    set_state(self,DATA_COMM_RX_STATE_BURST_COMPLETE);
                }
            }
            else if (type==DATA_COMM_MSG_TYPE_BURST_COMPLETION) {
                //Data completion received
                if (size<sizeof(data_comm_msg_burst_complete_t)) {
                    //Message too small
                    LOG_D("[%d] %s : %d. msg: %d size too small %d<%d",
                        self->now, __FUNCTION__, self->state,type, size,sizeof(data_comm_msg_burst_complete_t));

                    return false;
                }
                //Packet seems ok
                const data_comm_msg_burst_complete_t* burst_complete = (const data_comm_msg_burst_complete_t*) data;
                //extract sequence
                uint32_t sequence = buffer_extract_uint24_little(burst_complete->sequence);
                // uint32_t burst_size = burst_complete->burst_size;

                if (sequence!=self->burst_sequence) {
                    //It was not for our sequence, just ignore it
                    LOG_D("[%d] %s : %d. Sequence mismatch in WAIT DATA (COMPLETION): rx %08x, expected: %08x",
                        self->now, __FUNCTION__, self->state,type, sequence,self->burst_sequence);

                    return false;
                }
                //we receive a completion when we weren't expecting it, try and retry the burst
                if (!retry_burst(self,self->burst_sequence)) {
                    //Retries reached maximum, abort
                    self->status = DATA_COMM_STATUS_UNEXPECTED_COMPLETION;
                    set_state(self,DATA_COMM_RX_STATE_PROCESS);
                }

            }
            else {
                LOG_D("[%d] %s : %d. Unexpected msg in WAIT DATA: %d", self->now, __FUNCTION__, self->state,type);

            }
            break;
        }
        case DATA_COMM_RX_STATE_SEND_COMPLETE: {
            //Here we could expect a completion, but we don't care,
            //we only use the completion in case we miss a packet so
            //we can retry faster
            LOG_D("[%d] %s : %d. Unexpected msg in SEND_COMPLETE: %d", self->now, __FUNCTION__, self->state,type);

            return false;
        }
        case DATA_COMM_RX_STATE_PROCESS: {
            //Not expecting
            LOG_D("[%d] %s : %d. Unexpected msg in PROCESS: %d", self->now, __FUNCTION__, self->state,type);

            return false;
        }
        case DATA_COMM_RX_STATE_SEND_FINISH_REPORT: {
            //Not expecting
            LOG_D("[%d] %s : %d. Unexpected msg in FINISH REPORT: %d", self->now, __FUNCTION__, self->state,type);

            return false;
        }
        case DATA_COMM_RX_STATE_BURST_COMPLETE: {
            //Not expecting
            LOG_D("[%d] %s : %d. Unexpected msg in BURST COMPLETE: %d", self->now, __FUNCTION__, self->state,type);

            return false;
        }
        default: {
            //Not expecting
            LOG_D("[%d] %s : %d. Unexpected msg in DEFAULT: %d", self->now, __FUNCTION__, self->state,type);

            return false;
        }
    }



    return false;
}

int data_comm_rx_start(data_comm_rx_t* self, uint32_t time_ms) {
    assert(self);
    self->now=time_ms;
    if (self->state==DATA_COMM_RX_STATE_IDLE && !self->started) {
        self->started=true;
        LOG_I("[%d] %s: Success",self->now,__FUNCTION__);

        return 0;
    }
    LOG_W("[%d] %s: Still running",self->now,__FUNCTION__);

    return -1;
}

static int dummy_comm_log_func(int level, const char *format, ...){
    return 0;
}



int data_comm_rx_init(data_comm_rx_t* self, const data_comm_rx_settings_t* settings) {
    if (self==NULL) {
        return -1;
    }
    if (settings==NULL) {
        return -2;
    }
    if (settings->buffer==NULL || settings->buffer_size==0) {
        return -3;
    }
    if (settings->config==NULL) {
        return -4;
    }
    if (settings->finish_func==NULL || settings->write_func==NULL || settings->send_func==NULL) {
        return -6;
    }
    memcpy(&self->settings,settings,sizeof(data_comm_rx_settings_t));
    if (settings->log_func==NULL) {
        self->settings.log_func=dummy_comm_log_func;
    }
    LOG_D("%s Init",__FUNCTION__);
    self->state=DATA_COMM_RX_STATE_IDLE;
    self->started=false;

    self->total_size=0;
    self->delay_timer=0;
    self->count=0;
    self->timeout_timer=0;

    self->status=DATA_COMM_STATUS_OK;
    self->max_burst_size=0;

    self->burst_index=0;
    self->burst_size=0;
    self->requested_burst_size=0;
    self->burst_crc=0;
    self->expected_sequence=0;
    self->burst_sequence=0;
    self->burst_retries=0;
    self->full_retries=0;


    return 0;

}



