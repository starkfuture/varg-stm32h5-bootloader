//
// Created by nico on 09/11/24.
//

#include "data_comm_tx.h"

#include <assert.h>
#include <stdbool.h>
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




static void set_state(data_comm_tx_t* self, data_comm_tx_state_t state) {
    assert(self);
    if (self->state!=state) {
        LOG_D("[%d] State %d->%d\n",self->now, self->state,state);
    }
    self->state=state;
    self->timeout_timer=self->now;
    self->count=0;
    self->delay_timer=self->now;
}


static bool check_delay(data_comm_tx_t* self, uint32_t delay) {
    if (self->now - self->delay_timer<=delay) {
        return false;
    }
    // LOG_D("[%d] Delay %d finished\n",self->now, delay);
    self->delay_timer=self->now;
    return true;
}

static bool check_timeout(data_comm_tx_t* self, uint32_t timeout) {
    if (self->now - self->timeout_timer<=timeout) {
        return false;
    }
    LOG_D("[%d] Timeout %d\n",self->now, timeout);
    return true;
}

static bool check_count(data_comm_tx_t* self, uint32_t count) {
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

static int dummy_comm_log_func(int level, const char *format, ...){
    return 0;
}

int data_comm_tx_init(data_comm_tx_t* self, const data_comm_tx_settings_t* settings) {
    if (self==NULL) {
        // LOG_E("%s: Error, no data_comm_tx object",__FUNCTION__);
        return -1;
    }
    if (settings==NULL) {
        // LOG_E("%s: Error, no settings",__FUNCTION__);
        return -2;
    }
    if (settings->buffer==NULL || settings->buffer_size==0) {
        // LOG_E("%s: Error, no buffer",__FUNCTION__);
        return -3;
    }
    if (settings->config==NULL) {
        // LOG_E("%s: Error, no config",__FUNCTION__);
        return -4;
    }
    if (settings->finish_callback==NULL || settings->read_func==NULL) {
        // LOG_E("%s: Error, no callbacks",__FUNCTION__);

        return -5;
    }
    if (settings->packet_size==0) {
        // LOG_E("%s: Error, no packet_size",__FUNCTION__);

        return -6;
    }
    memcpy(&self->settings, settings, sizeof(data_comm_tx_settings_t));
    self->config = self->settings.config;
    self->state=DATA_COMM_TX_STATE_IDLE;
    self->requested_sequence=0;
    self->requested_size=0;
    self->burst_index=0;
    self->burst_sequence=0;
    self->timeout_timer=0;
    self->delay_timer=0;
    self->count=0;
    self->status=0;
    self->error_status=0;
    memset(self->error_data,0,sizeof(self->error_data));
    if (settings->log_func==NULL) {
        self->settings.log_func=dummy_comm_log_func;
    }
    LOG_I("%s: Success",__FUNCTION__);
    return 0;
}

int data_comm_tx_start(data_comm_tx_t* self, uint32_t time_ms) {
    assert(self);
    self->now=time_ms;
    if (self->state!=DATA_COMM_TX_STATE_IDLE) {
        LOG_W("[%d] %s: Still running",self->now,__FUNCTION__);

        return -1;
    }
    LOG_I("[%d] %s: Success",self->now,__FUNCTION__);
    set_state(self,DATA_COMM_TX_STATE_PREPARE_REQ_AND_WAIT);

    return 0;
}


void data_comm_tx_tick(data_comm_tx_t* self, uint32_t time_ms) {
    assert(self);
    assert(self->config);
    assert(self->settings.send_func);
    assert(self->settings.read_func);
    assert(self->settings.finish_callback);
    assert(self->settings.log_func);
    assert(self->settings.buffer);
    assert(self->settings.buffer_size>0);

    self->now=time_ms;
    const data_comm_tx_config_t* config = self->config;
    data_comm_tx_settings_t* settings = &self->settings;

    switch (self->state) {
        case DATA_COMM_TX_STATE_IDLE: {
            //Do nothing
            break;
        }
        case DATA_COMM_TX_STATE_PREPARE_REQ_AND_WAIT: {
            self->packets_sent=false;

            //The user requested the start, wait while sending the request to bootloader
            if (check_timeout(self,config->ready_timeout)) {
                LOG_W("[%d] %s : %d. Timeout, didn't receive Ready", self->now, __FUNCTION__, self->state);
                //Timeout, the PEER didn't reported ready
                self->status = DATA_COMM_STATUS_READY_TIMEOUT;
                set_state(self,DATA_COMM_TX_STATE_ERROR);
                break;
            }
            if (!check_delay(self,config->prepare_delay)) {
                //Wait until the delay passed
                break;
            }
            //the delay expired
            //Send the request
            LOG_I("[%d] %s : %d. Sending Prepare", self->now, __FUNCTION__, self->state);
            int result =settings->send_func(DATA_COMM_MSG_TYPE_PREPARE_REQUEST,settings->peer_id,
                NULL,0, NULL,0, settings->context);
            if (result!=0) {
                //there was an error sending the data, just stop everything
                LOG_E("[%d] %s : %d. Error sending %d", self->now, __FUNCTION__, self->state,result);
                self->status=DATA_COMM_ERROR_COMM_ERROR;
                set_state(self,DATA_COMM_TX_STATE_ERROR);
            }
            break;
        }
        case DATA_COMM_TX_STATE_WAIT_FOR_MESSAGES: {
            if (self->error_status) {
                //We need to send some errors
                self->timeout_timer=time_ms;  //Do not timeout if you're sending errors
                //We need to send an error to the PEER based on a request
                if (!check_delay(self,config->error_delay)) {
                    //Wait before sending stuffs
                    break;
                }
                LOG_I("[%d] %s : %d. Sending Error report 0x%02x", self->now, __FUNCTION__, self->state, self->error_status);
                data_comm_msg_error_t error_report = {
                    .status =self->error_status,
                };
                memcpy(error_report.data,self->error_data,sizeof(error_report.data));
                int result =settings->send_func(DATA_COMM_MSG_TYPE_ERROR_RESPONSE,settings->peer_id,
                (const uint8_t*)&error_report,sizeof(error_report), NULL,0, settings->context);
                if (result !=0) {
                    //Something went wrong sending, stop
                    LOG_E("[%d] %s : %d. Error sending %d", self->now, __FUNCTION__, self->state,result);

                    self->status=DATA_COMM_ERROR_COMM_ERROR;
                    set_state(self,DATA_COMM_TX_STATE_ERROR);
                    break;
                }
                //We send a number of copies of the error
                if (check_count(self,config->error_count)) {
                    //We're done sending errors
                    self->error_status=0;
                    memset(self->error_data,0,sizeof(self->error_data));
                    self->count=0;
                }
                break;

            }
            //Waiting for the PEER request packets
            if (check_timeout(self,config->packets_timeout)) {
                //The PEER never requested anything, just stop everything
                LOG_W("Timeout waiting for packets");
                self->status = DATA_COMM_STATUS_REQUEST_TIMEOUT;
                set_state(self,DATA_COMM_TX_STATE_ERROR);
                break;
            }

            if (!check_delay(self,config->info_delay)) {
                // But wait a bit
                break;
            }
            if (self->packets_sent) {
                break;
            }
            // The delay expired, send it now
            // Send the info
            LOG_I("[%d] %s : %d. SI total: 0x%08x, buffer: %d", self->now, __FUNCTION__, self->state, settings->total_size, settings->buffer_size);
            data_comm_msg_info_t msg_info;
            msg_info.total_size = settings->total_size;
            msg_info.buffer_size = settings->buffer_size;
            int result =settings->send_func(DATA_COMM_MSG_TYPE_INFO,settings->peer_id,(const uint8_t*)&msg_info, sizeof(msg_info),NULL,0,settings->context);
            if(result!=0) {
                //There was an issue sending the data, just stop everything
                LOG_E("[%d] %s : %d. Error sending %d", self->now, __FUNCTION__, self->state,result);
                self->status=DATA_COMM_ERROR_COMM_ERROR;
                set_state(self,DATA_COMM_TX_STATE_ERROR);
                break;
            }
            break;
        }
        case DATA_COMM_TX_STATE_SEND_CRC: {
            //PEER requested a burst
            if (!check_delay(self,config->crc_delay)) {
                //Wait before sending stuffs
                break;
            }
            //Delay expired, start processing
            //Calculate the address of the burst
            uint32_t address = self->requested_sequence;
            uint32_t size = self->requested_size;
            if (address>settings->total_size) {
                LOG_W("[%d] %s : %d. Sequence OOB %d>%d", self->now, __FUNCTION__, self->requested_sequence,settings->total_size);
                //The address goes out of bound, send an error to the PEER and go back to wait for messages
                self->error_status = DATA_COMM_ERROR_SEQUENCE_OOB;
                set_state(self,DATA_COMM_TX_STATE_WAIT_FOR_MESSAGES);
                break;
            }
            if (size>settings->buffer_size) {
                size=settings->buffer_size;
                // self->error_status = DATA_COMM_ERROR_BURST_SIZE_TOO_BIG;
                // set_state(self,DATA_COMM_TX_STATE_WAIT_FOR_MESSAGES);
                // break;
            }
            if ((address+size)>settings->total_size) {
                //This is the last burst
                size = settings->total_size-address;
            }
            self->requested_size=size;

            //We need to calculate the CRC
            if (self->count==0) {

                //Read from the memory only the first time
                int result =settings->read_func(address,size,settings->buffer,settings->context);
                if (result!=0) {
                    LOG_E("[%d] %s : %d. Error reading %d", self->now, __FUNCTION__, self->state,result);

                    self->status = DATA_COMM_ERROR_READ_ERROR;
                    set_state(self,DATA_COMM_TX_STATE_ERROR);
                    break;
                }
                //calculate the CRC of the burst
                self->burst_crc =calc_crc16(settings->buffer,size,settings->crc_seed,0);
            }


            //Send the crc
            LOG_I("[%d] %s : %d. SCRC seq: 0x%08x, crc: %04x size: %d", self->now, __FUNCTION__, self->state, self->requested_sequence, self->burst_crc,self->requested_size);

            data_comm_msg_crc_t msg_crc;
            buffer_append_uint24_little(self->requested_sequence, &msg_crc.sequence[0]);
            msg_crc.crc=self->burst_crc;
            msg_crc.burst_size=self->requested_size;
            int result =settings->send_func(DATA_COMM_MSG_TYPE_BURST_CRC,settings->peer_id,(const uint8_t*) &msg_crc,sizeof(msg_crc),NULL,0,settings->context);
            if (result!=0) {
                //Error sending the CRC, just stop
                LOG_E("[%d] %s : %d. Error sending %d", self->now, __FUNCTION__, self->state,result);

                self->status = DATA_COMM_ERROR_COMM_ERROR;
                set_state(self,DATA_COMM_TX_STATE_ERROR);
                break;
            }
            self->packets_sent=true;
            //We need to repeat the message several times
            if (check_count(self,config->crc_count)) {
                //We sent all, now we need to send the packets
                self->burst_index=0;
                self->burst_sequence=0;
                set_state(self,DATA_COMM_TX_STATE_SEND_PACKETS);
            }
            break;
        }
        case DATA_COMM_TX_STATE_SEND_PACKETS: {
            //Wait the delay
            if (!check_delay(self,config->packets_delay)) {
                break;
            }
            //Delay expired
            //Calculate the size of the packet
            uint16_t size = self->requested_size - self->burst_index;
            if (size > settings->packet_size) {
                size = settings->packet_size;
            }
            const uint8_t* header=NULL;
            uint16_t header_size;
            const uint8_t* data=NULL;
            uint16_t data_size=0;
            int result = 0;
            data_comm_msg_burst_complete_t complete_msg;
            data_comm_msg_packet_header_t packet_msg;
            if (size==0) {
                //Last packet was sent, now we send the completion message
                LOG_I("[%d] %s : %d. SBC seq: 0x%08x, size: %d", self->now, __FUNCTION__, self->state, self->burst_index, size);
                buffer_append_uint24_little(self->requested_sequence,complete_msg.sequence);
                complete_msg.burst_size=self->burst_index;
                header = (const uint8_t*)&complete_msg;
                header_size = sizeof(complete_msg);
                result = settings->send_func(DATA_COMM_MSG_TYPE_BURST_COMPLETION,settings->peer_id,header,header_size,data,data_size,settings->context);

            } else {
                //send the packet
                uint32_t sequence = self->requested_sequence + self->burst_sequence;
                LOG_I("[%d] %s : %d. SP seq: 0x%08x, size: %d", self->now, __FUNCTION__, self->state, sequence, size);
                buffer_append_uint24_little(sequence,packet_msg.sequence);
                header = (const uint8_t*)&packet_msg;
                header_size = sizeof(packet_msg);
                data = &settings->buffer[self->burst_index];
                data_size = size;
                result = settings->send_func(DATA_COMM_MSG_TYPE_BURST_PACKET,settings->peer_id,header,header_size,data,data_size,settings->context);
            }

            if (result !=0) {
                LOG_E("[%d] %s : %d. Error sending %d", self->now, __FUNCTION__, self->state,result);
                //There was an error sending the packet
                self->status = DATA_COMM_ERROR_COMM_ERROR;
                set_state(self,DATA_COMM_TX_STATE_ERROR);
                break;
            }
            // We need to send a number of copies of the packet
            if (check_count(self,config->packets_count)) {
                //Done sending the copies
                if (size==0) {
                    //done with the burst, just wait for more messages
                    set_state(self,DATA_COMM_TX_STATE_WAIT_FOR_MESSAGES);
                } else {

                    // We will send more packets, increment the sequence and the index
                    self->burst_index+=size;
                    self->burst_sequence+=size;
                    self->count=0;
                }
            }

            break;
        }
        case DATA_COMM_TX_STATE_PEER_FINISH_WAIT: {

            //We need to wait for the PEER to enter APP mode
            if (check_timeout(self,config->finished_timeout)) {
                // Timeout, never reported finished
                LOG_W("[%d] %s : %d. Timeout, didn't receive finish", self->now, __FUNCTION__, self->state);
                self->status = DATA_COMM_STATUS_FINISH_TIMEOUT;
                set_state(self,DATA_COMM_TX_STATE_ERROR);
            }
            break;
        }
        case DATA_COMM_TX_STATE_ERROR: {
            //The process ended with an error, call the callback and go to idle
            LOG_E("[%d] %s : Failed with 0x%02x", self->now, __FUNCTION__, self->status);
            settings->finish_callback(self->status,settings->peer_id,settings->context);
            set_state(self,DATA_COMM_TX_STATE_IDLE);
            break;
        }
        case DATA_COMM_TX_STATE_DONE: {
            LOG_E("[%d] %s : Succeed", self->now, __FUNCTION__, self->status);
            //The process ended successfully, call the calback and go to idle
            settings->finish_callback(DATA_COMM_STATUS_OK,settings->peer_id,settings->context);
            set_state(self,DATA_COMM_TX_STATE_IDLE);
            break;
        }
    }
}

bool data_comm_tx_is_running(const data_comm_tx_t * self) {
    return self->state!=DATA_COMM_TX_STATE_IDLE;
}
bool data_comm_tx_process_message(data_comm_tx_t* self, uint32_t time_ms, data_comm_msg_type_t type, uint8_t peer, const uint8_t* data, uint16_t size) {
    assert(self);
    self->now=time_ms;
    data_comm_tx_settings_t* settings = &self->settings;

    if (peer!=settings->peer_id) {
        LOG_D("[%d] %s : Unexpected id 0x%02x != 0x%02x", self->now, __FUNCTION__, peer,settings->peer_id);
        return false;
    }
    switch (self->state) {
        case DATA_COMM_TX_STATE_IDLE: {
            //We're not expecting messages here
            LOG_D("[%d] %s : %d. Unexpected msg: %d", self->now, __FUNCTION__, self->state,type);
            return false;
        }
        case DATA_COMM_TX_STATE_PREPARE_REQ_AND_WAIT: {
            if (type!=DATA_COMM_MSG_TYPE_READY_REPORT) {
                LOG_D("[%d] %s : %d. Unexpected msg: %d", self->now, __FUNCTION__, self->state,type);
                return false;
            }
            if (data==NULL || size<sizeof(data_comm_msg_ready_t)) {
                LOG_D("[%d] %s : %d. no data or size %d<%d msg: %d",
                    self->now, __FUNCTION__, self->state,size, sizeof(data_comm_msg_ready_t) ,type);
                //We're expecting some data in this message
                return false;
            }
            const data_comm_msg_ready_t* msg = (const data_comm_msg_ready_t*)data;
            if (msg->status==DATA_COMM_PEER_STATUS_RSP_READY) {
                //PEER is reporting ready
                LOG_I("[%d] %s : %d. peer 0x%02x, ready", self->now, __FUNCTION__, self->state,peer);
                set_state(self, DATA_COMM_TX_STATE_WAIT_FOR_MESSAGES);
                return true;
            }
            LOG_D("[%d] %s : %d. peer 0x%02x, not ready 0x%02x", self->now, __FUNCTION__, self->state,peer,msg->status);
            return false;
        }
        case DATA_COMM_TX_STATE_WAIT_FOR_MESSAGES: {
            switch(type) {
                case DATA_COMM_MSG_TYPE_BURST_REQUEST: {
                    if (data==NULL || size<sizeof(data_comm_burst_request_t)) {
                        LOG_D("[%d] %s : %d. no data or size %d<%d msg: %d",
                            self->now, __FUNCTION__, self->state,size, sizeof(data_comm_burst_request_t) ,type);
                        return false;
                    }
                    const data_comm_burst_request_t * msg = (const data_comm_burst_request_t*) data;
                    self->requested_sequence = buffer_extract_uint24_little(msg->sequence);
                    self->requested_size = msg->burst_size;
                    LOG_I("[%d] %s : %d. burst req 0x%02x, seq: %d, size:%d",
                        self->now, __FUNCTION__, self->state,peer, self->requested_sequence, self->requested_size);
                    set_state(self,DATA_COMM_TX_STATE_SEND_CRC);
                    return true;
                }
                case DATA_COMM_MSG_TYPE_FINISH_REPORT: {
                    if (data==NULL || size<sizeof(data_comm_status_t)) {
                        LOG_D("[%d] %s : %d. no data or size %d<%d msg: %d",
                            self->now, __FUNCTION__, self->state,size, sizeof(data_comm_status_t) ,type);

                        return false;
                    }
                    const data_comm_status_t * msg = (const data_comm_status_t*) data;
                    if (msg->status==DATA_COMM_STATUS_OK) {
                        LOG_I("[%d] %s : %d. Finish 0x%02x, success",
                            self->now, __FUNCTION__, self->state,peer);

                        set_state(self,DATA_COMM_TX_STATE_PEER_FINISH_WAIT);
                    } else {
                        LOG_I("[%d] %s : %d. Finish 0x%02x, with 0x%02x",
                            self->now, __FUNCTION__, self->state,peer, msg->status);

                        self->status=msg->status;
                        set_state(self,DATA_COMM_TX_STATE_ERROR);
                    }
                    return true;
                }
                default: {
                    LOG_D("[%d] %s : %d. Unexpected type %d %d",
                    self->now, __FUNCTION__, self->state,type);
                    break;
                }
            }
            return false;
        }
        case DATA_COMM_TX_STATE_SEND_CRC: {
            //Not waiting
            LOG_D("[%d] %s : %d. Unexpected msg: %d", self->now, __FUNCTION__, self->state,type);

            return false;
        }
        case DATA_COMM_TX_STATE_SEND_PACKETS: {
            //Not waiting
            LOG_D("[%d] %s : %d. Unexpected msg: %d", self->now, __FUNCTION__, self->state,type);

            return false;
        }
        case DATA_COMM_TX_STATE_PEER_FINISH_WAIT: {
            if (type!=DATA_COMM_MSG_TYPE_FINISH_REPORT) {
                LOG_D("[%d] %s : %d. Unexpected msg: %d", self->now, __FUNCTION__, self->state,type);
                return false;
            }
            if (data==NULL || size<sizeof(data_comm_status_t)) {
                //We're expecting some data in this message
                LOG_D("[%d] %s : %d. no data or size %d<%d msg: %d",
                    self->now, __FUNCTION__, self->state,size, sizeof(data_comm_status_t) ,type);

                return false;
            }
            const data_comm_status_t* msg = (const data_comm_status_t*)data;
            if (msg->status==DATA_COMM_STATUS_OK) {
                //PEER is reporting in app
                LOG_I("[%d] %s : %d. Finish 0x%02x, success",
                    self->now, __FUNCTION__, self->state,peer);

                set_state(self, DATA_COMM_TX_STATE_DONE);

            } else {
                LOG_I("[%d] %s : %d. Finish 0x%02x, with 0x%02x",
               self->now, __FUNCTION__, self->state,peer, msg->status);

                self->status=msg->status;
                set_state(self, DATA_COMM_TX_STATE_ERROR);
            }
            return true;

        }
        case DATA_COMM_TX_STATE_ERROR:{
            LOG_D("[%d] %s : %d. Unexpected msg: %d", self->now, __FUNCTION__, self->state,type);

            //Not waiting
            return false;
        }
        case DATA_COMM_TX_STATE_DONE:{
            //Not waiting
            LOG_D("[%d] %s : %d. Unexpected msg: %d", self->now, __FUNCTION__, self->state,type);

            return false;
        }
        default:{
            //Not waiting
            LOG_D("[%d] %s : %d. Unexpected msg: %d", self->now, __FUNCTION__, self->state,type);

            return false;
        }
    }


}
