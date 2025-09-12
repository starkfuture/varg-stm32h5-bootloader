/**
 * @file bootloader.c
 * @brief Bootloader state machine implementation for firmware update process.
 */

#include "bootloader.h"

#include <assert.h>
#include <stdio.h>

#include "./fw_verification/fw.h"
#include <string.h>

#include "../btea/btea.h"
#include "../data_comm/data_comm_rx.h"
#include "./fw_verification/fw_verification.h"
#include "../utils.h"


#if DATA_COMM_LOG_LEVEL <= DATA_COMM_LOG_LEVEL_DEBUG
#define LOG_D(fmt,...) \
bootloader.config->log_func(DATA_COMM_LOG_LEVEL_DEBUG,fmt "\n",##__VA_ARGS__)
#else
#define LOG_D(fmt,...)
#endif

#if DATA_COMM_LOG_LEVEL <= DATA_COMM_LOG_LEVEL_INFO
#define LOG_I(fmt,...) \
bootloader.config->log_func(DATA_COMM_LOG_LEVEL_INFO,fmt "\n",##__VA_ARGS__)
#else
#define LOG_I(fmt,...)
#endif

#if DATA_COMM_LOG_LEVEL <= DATA_COMM_LOG_LEVEL_WARNING
#define LOG_W(fmt,...) \
bootloader.config->log_func(DATA_COMM_LOG_LEVEL_WARNING,fmt "\n",##__VA_ARGS__)
#else
#define LOG_W(fmt,...)
#endif


#if DATA_COMM_LOG_LEVEL <= DATA_COMM_LOG_LEVEL_ERROR
#define LOG_E(fmt,...) \
bootloader.config->log_func(DATA_COMM_LOG_LEVEL_ERROR,fmt "\n",##__VA_ARGS__)
#else
#define LOG_E(fmt,...)
#endif




/** Bootloader state enumeration */
typedef enum {
    BOOT_STATE_INIT,
    BOOT_STATE_WAIT_APP_VERIFICATION,
    BOOT_STATE_COPY_UPGRADE,
    BOOT_STATE_START_DATA_COMM,
    BOOT_STATE_IDLE,
    BOOT_STATE_GO_TO_APP
} bootloader_state_t;

enum {
    BOOTLOADER_INSTALLED_FW_STATUS_NOT_FOUND=0,
    BOOTLOADER_INSTALLED_FW_STATUS_FOUND=1,
    BOOTLOADER_INSTALLED_FW_STATUS_VERIFIED=2,
    BOOTLOADER_INSTALLED_FW_STATUS_CORRUPTED=3,
};

struct {
    const bootloader_config_t* config;
    bootloader_sections_t sections;
    data_comm_rx_t data_comm;
    bootloader_state_t state;
    uint8_t rx_buffer[sizeof(fw_header_t)];
    struct {
        uint8_t app_status:2;
        uint8_t upgrade_status:2;
    };
    uint32_t copy_retries;
    bool jump_to_app;
    bool info_cleared;
    uint32_t start_time;
    bool stay_in_bootloader;
    bootloader_installed_fw_t app_info;
    bootloader_installed_fw_t upgrade_info;
    fw_header_t header;
    fw_t fw;
    uint32_t fw_index;
    uint32_t btea_index;
    bool receiving;
}bootloader;

const data_comm_rx_config_t data_comm_config={
    .max_full_retries = 3,
    .max_retries_burst = 20,
    .prepare_timeout = 0,
    .info_timeout = 1000,
    .packets_timeout = 1000,
    .ready_status_delay = 5,
    .request_count = 10,
    .request_delay = 2,
    .complete_delay = 2,
    .complete_count = 5,
    .finish_delay = 5,
    .finish_count = 5
};

static data_comm_rx_write_result_t data_comm_write_func(uint32_t address, const uint8_t* data, uint32_t size, bool last, void*context);
static data_comm_rx_finish_result_t data_comm_finish_callback(uint8_t status, uint8_t* finish_error, void*context);
//static int data_comm_log_func(int level, const char *format, ...);


static void decrypt_chunk(uint32_t *buffer, int size, const btea_key_t *key) {
    int n = size / sizeof(uint32_t);
    btea(buffer, -n, key);
}

static void clear_info() {
    if (!bootloader.info_cleared) {
        //Set the app to not found and not verified since we have just write to it
        bootloader.upgrade_status=BOOTLOADER_INSTALLED_FW_STATUS_NOT_FOUND;
        bootloader.app_status=BOOTLOADER_INSTALLED_FW_STATUS_NOT_FOUND;
        memset(&bootloader.upgrade_info,0xFF,sizeof(bootloader.upgrade_info));
        bootloader.config->mem_write_func( bootloader.sections.upgrade_info.address, (uint8_t*)&bootloader.upgrade_info,sizeof(bootloader.upgrade_info));
        bootloader.info_cleared=true;
    }
}

static data_comm_rx_write_result_t data_comm_write_func(uint32_t address, const uint8_t* data, uint32_t size, bool last, void*context) {

	(void)context;

    uint32_t index=0;
    uint32_t remaining=size;
    if (address<sizeof(fw_header_t)) {
        LOG_I("%s Doing header: %d\n",__FUNCTION__, address);

        //Doing still header
        uint32_t to_copy=size;
        bool header_complete=false;
        if ((address+to_copy)>=sizeof(fw_header_t)) {
            //part is for header and part is outside => the header is complete
            header_complete=true;
            bootloader.btea_index=0;
            LOG_I("%s header complete: %d\n",__FUNCTION__, address+to_copy);

            //copy only the part of the header
            to_copy = sizeof(fw_header_t)-address;
        }


        uint8_t* ptr = (uint8_t*)&bootloader.header;
        memcpy(&ptr[address],data,to_copy);
        index+=to_copy;
        remaining-=to_copy;

        if (header_complete) {

            //header is complete
            int verified = fw_verification_process_header(&bootloader.header,bootloader.config->public_key);
            if (verified!=0) {
                LOG_E("Header verification failed");
                return DATA_COMM_RX_WRITE_ERROR_REPEAT_FROM_BEGGINING;
            }
            if (bootloader.header.fw_info.fw_size > bootloader.sections.upgrade.size) {
                LOG_E("FW size is too big");
                return DATA_COMM_RX_WRITE_ERROR_ABORT;
            }
        }
    }
    //Process the rest
    LOG_I("%s Processing rest: %d\n",__FUNCTION__, remaining);

    while (remaining>0) {


        uint32_t to_copy=remaining;
        //unencrypt and copy and calcu
        uint32_t btea_remaining = to_copy;
        LOG_I("%s Btea remaining %d\n",__FUNCTION__, btea_remaining);

        while(btea_remaining>0) {
            LOG_I("%s Btea remaining %d\n",__FUNCTION__, btea_remaining);

            uint32_t btea_to_copy = btea_remaining;
            if ((bootloader.btea_index + btea_to_copy) > bootloader.config->btea_chunk_size) {
                //copy this much will fill more than the btea chunk, so copy only to fill it exactly
                to_copy = bootloader.config->btea_chunk_size - bootloader.btea_index;

            }
            //copy the data to the btea buffer
            LOG_I("%s Copy %d to btea buffer[%d]\n",__FUNCTION__, index,bootloader.btea_index);

            memcpy(&bootloader.config->btea_buffer[bootloader.btea_index],&data[index],btea_to_copy);
            //now there are less remaining
            btea_remaining-=btea_to_copy;
            //update the btea pointer
            bootloader.btea_index+=btea_to_copy;
            //check if the btea buffer is full
            if (bootloader.btea_index==bootloader.config->btea_chunk_size) {
                //buffer full
                clear_info();
                //Decrypt chunk
                decrypt_chunk((uint32_t *)bootloader.config->btea_buffer,bootloader.config->btea_chunk_size,bootloader.config->btea_key);
                //Write the chunk to the flash
                LOG_I("%s Writing decrypted buffer to %08x %d\n",__FUNCTION__, bootloader.config->upgrade_start_address + bootloader.fw_index);

                int result = bootloader.config->mem_write_func( bootloader.sections.upgrade.address + bootloader.fw_index, bootloader.config->btea_buffer, bootloader.config->btea_chunk_size);
                if (result!=0) {
                    return DATA_COMM_RX_WRITE_ERROR_REPEAT_FROM_BEGGINING;
                }
                //Update the pointer of the firmware
                bootloader.fw_index+=bootloader.config->btea_chunk_size;
                //reset the pointer to the btea buffer
                bootloader.btea_index=0;
                //clear the btea buffer to be ready
                memset(bootloader.config->btea_buffer,0xFF,bootloader.config->btea_chunk_size);
            }
        }
        index+=to_copy;
        remaining-=to_copy;
    }
    //If it's the last message, then decrypt the chunk and write
    if (last && bootloader.btea_index>0) {
        clear_info();
        decrypt_chunk((uint32_t *)bootloader.config->btea_buffer,bootloader.btea_index,bootloader.config->btea_key);
        uint32_t padding = bootloader.config->padding.num_bytes - (bootloader.btea_index % bootloader.config->padding.num_bytes);

        if (padding!=0) {
            memset(&bootloader.config->btea_buffer[bootloader.btea_index], bootloader.config->padding.padding_byte,padding);
            bootloader.btea_index+=padding;
        }
        bootloader.upgrade_status=BOOTLOADER_INSTALLED_FW_STATUS_NOT_FOUND;

        int result = bootloader.config->mem_write_func( bootloader.sections.upgrade.address + bootloader.fw_index, bootloader.config->btea_buffer, bootloader.btea_index);
        if (result!=0) {
            return DATA_COMM_RX_WRITE_ERROR_ABORT;
        }
        //Set the app to not found and not verified since we have just write to it


        bootloader.fw_index+=bootloader.btea_index;
    }

    return DATA_COMM_RX_WRITE_SUCCESS;



}
static data_comm_rx_finish_result_t data_comm_finish_callback(uint8_t status, uint8_t* finish_error, void*context) {

	(void)context;
    if (status!=DATA_COMM_STATUS_OK) {
        //There was an error
        if (finish_error) {
            *finish_error=status;
        }
        return DATA_COMM_RX_FINISH_ERROR_ABORT;
    }


    int result =  fw_verification_process_init(&bootloader.fw, &bootloader.header, bootloader.config->public_key);
    if (result!=0) {
        if (finish_error) {
            *finish_error=BOOTLOADER_ERROR_HEADER_VERIFICATION_FAILED;
        }

        //Error, probably header is wrong
        return DATA_COMM_RX_FINISH_ERROR_ABORT;
    }



    /* Get CRC */
    uint32_t output_crc = bootloader.config->crc32_func(bootloader.sections.upgrade.address,
                                                        bootloader.header.fw_info.fw_size);

    if (output_crc!=bootloader.header.fw_info.fw_crc) {
        //CRC Fail
        if (finish_error) {
            *finish_error=BOOTLOADER_ERROR_CRC_MISMATCH;
        }
        LOG_E("%s CRC mismatch %08x != %08x", __FUNCTION__,output_crc,bootloader.header.fw_info.fw_crc);
        return DATA_COMM_RX_FINISH_ERROR_RETRY;
    }

    uint32_t remaining = bootloader.header.fw_info.fw_size;
    uint32_t address = bootloader.sections.upgrade.address;
    while (remaining>0) {
        uint32_t to_read = remaining;
        if (to_read>=bootloader.config->btea_chunk_size) {
            to_read=bootloader.config->btea_chunk_size;
        }
        result = bootloader.config->mem_read_func(address, bootloader.config->btea_buffer, to_read);
        if (result!=0) {
            //Error reading file
            if (finish_error) {
                *finish_error=BOOTLOADER_ERROR_READ_ERROR;
            }
            return DATA_COMM_RX_FINISH_ERROR_ABORT;
        }
        result = fw_verification_process_binary(&bootloader.fw, bootloader.config->btea_buffer, to_read);
        remaining-=to_read;
        address+=to_read;

        if (result<0) {
            if (finish_error) {
                *finish_error=BOOTLOADER_ERROR_VERIFICATION_FAILED;
            }
            return DATA_COMM_RX_FINISH_ERROR_RETRY;
        }
        if (result >0) {
            bootloader.upgrade_info.magic=bootloader.config->magic;
            bootloader.upgrade_info.info = bootloader.header.fw_info;
            bootloader.upgrade_info.crc = calc_crc32((uint8_t*)&bootloader.upgrade_info,sizeof(bootloader.upgrade_info)-sizeof(bootloader.upgrade_info.crc),0xDEADBEEF,0);
            result = bootloader.config->mem_write_func( bootloader.sections.upgrade_info.address, (uint8_t *)&bootloader.upgrade_info,sizeof(bootloader.upgrade_info));
            if (result!=0) {
                if (finish_error) {
                    *finish_error=BOOTLOADER_ERROR_WRITE_ERROR;
                }
                return DATA_COMM_RX_FINISH_ERROR_ABORT;
            }
            if (finish_error) {
                *finish_error=BOOTLOADER_STATUS_OK;
            }
            return DATA_COMM_RX_FINISH_SUCCESS;
        }
    }

    if (finish_error) {
        *finish_error=BOOTLOADER_ERROR_UNKNOWN;
    }
    return DATA_COMM_RX_FINISH_ERROR_ABORT;
}

uint8_t bootloader_app_status() {
    uint8_t status=bootloader.state & 0x0F;
    status |= bootloader.upgrade_status<<4;
    status |= bootloader.app_status<<6;
    return status;
}

void bootloader_init(const bootloader_config_t* bootloader_config, const bootloader_sections_t* sections){
    assert(bootloader_config);
    assert(sections);
    assert(sections->app_info.size >= sizeof(bootloader_installed_fw_t));
    assert(sections->upgrade_info.size>= sizeof(bootloader_installed_fw_t) /* || sections->upgrade_info.size ==0 */);
    assert(sections->upgrade_info.size <= sections->app_info.size);
    assert(sections->upgrade.size <= sections->app.size);
    assert(sections->app.size >0);
    assert(sections->upgrade.size >0);

    bootloader.config = bootloader_config;

    memcpy(&bootloader.sections,sections,sizeof(bootloader_sections_t));

    bootloader.state = BOOT_STATE_INIT;

    memset(bootloader.rx_buffer, 0, sizeof(bootloader.rx_buffer));

    bootloader.upgrade_status=BOOTLOADER_INSTALLED_FW_STATUS_NOT_FOUND;
    bootloader.app_status=BOOTLOADER_INSTALLED_FW_STATUS_NOT_FOUND;
    bootloader.start_time=0;
    bootloader.stay_in_bootloader=false;

    bootloader.copy_retries=0;

    bootloader.info_cleared = false;

    bootloader.fw.bytes_processed = 0;
    memset(&bootloader.fw.sha_ctx, 0, sizeof(bootloader.fw.sha_ctx));
    bootloader.fw.header = NULL;

    bootloader.fw_index = 0;
    bootloader.btea_index = 0;
    memset(bootloader.config->btea_buffer,0xFF,bootloader.config->btea_chunk_size);
    data_comm_rx_settings_t data_comm_settings = {
        .config = &data_comm_config,
        .buffer = bootloader.rx_buffer,
        .buffer_size = 512,
        .id = bootloader.config->device_id,
        .crc_seed = 0xDEAD,
        .write_func = data_comm_write_func,
        .send_func = bootloader.config->send_msg_func,
        .finish_func = data_comm_finish_callback,
        .log_func = bootloader.config->log_func,
        .context = NULL
    };
    bootloader.receiving=false;
    data_comm_rx_init(&bootloader.data_comm,&data_comm_settings);
}


void bootloader_tick(uint32_t time_ms) {

    switch(bootloader.state) {
        case BOOT_STATE_INIT: {
            bootloader.info_cleared=false;
            bootloader.receiving=false;
            bootloader.app_status=BOOTLOADER_INSTALLED_FW_STATUS_NOT_FOUND;


            //Read the installed firmware information
            memset(&bootloader.app_info,0,sizeof(bootloader.app_info));
            bootloader.config->mem_read_func(bootloader.sections.app_info.address,(uint8_t*)&bootloader.app_info, sizeof(bootloader.app_info));
            uint32_t app_info_crc = calc_crc32((uint8_t*)&bootloader.app_info,sizeof(bootloader.app_info)-sizeof(bootloader.app_info.crc),0xDEADBEEF,0);

            if( bootloader.app_info.magic == bootloader.config->magic && app_info_crc==bootloader.app_info.crc) {
                bootloader.app_status=BOOTLOADER_INSTALLED_FW_STATUS_FOUND;
            }




            bootloader.upgrade_status=BOOTLOADER_INSTALLED_FW_STATUS_NOT_FOUND;
            memset(&bootloader.upgrade_info,0,sizeof(bootloader.upgrade_info));
            bootloader.config->mem_read_func(bootloader.sections.upgrade_info.address,(uint8_t*)&bootloader.upgrade_info, sizeof(bootloader.upgrade_info));
            uint32_t upgrade_info_crc = calc_crc32((uint8_t*)&bootloader.upgrade_info,sizeof(bootloader.upgrade_info)-sizeof(bootloader.upgrade_info.crc),0xDEADBEEF,0);

            if( bootloader.upgrade_info.magic == bootloader.config->magic && upgrade_info_crc==bootloader.upgrade_info.crc) {
                bootloader.upgrade_status=BOOTLOADER_INSTALLED_FW_STATUS_FOUND;
            }

            if (bootloader.upgrade_status==BOOTLOADER_INSTALLED_FW_STATUS_FOUND || bootloader.app_status==BOOTLOADER_INSTALLED_FW_STATUS_FOUND) {
                bootloader.state = BOOT_STATE_WAIT_APP_VERIFICATION;
            } else {
                bootloader.state = BOOT_STATE_START_DATA_COMM;
            }
            break;
        }
        case BOOT_STATE_WAIT_APP_VERIFICATION: {
            // Calculate the CRC of the installed firmware
            /* Get CRC */
            if (bootloader.app_status==BOOTLOADER_INSTALLED_FW_STATUS_FOUND) {
                if (bootloader.app_info.info.fw_size>bootloader.sections.app.size) {
                    bootloader.app_status=BOOTLOADER_INSTALLED_FW_STATUS_CORRUPTED;
                } else {
                    uint32_t crc = bootloader.config->crc32_func(bootloader.sections.app.address,
                                                        bootloader.app_info.info.fw_size);

                    /* Compare expected CRC with installed FW CRC */
                    if( bootloader.app_info.info.fw_crc == crc ) {
                        bootloader.app_status=BOOTLOADER_INSTALLED_FW_STATUS_VERIFIED;
                    } else {
                        bootloader.app_status=BOOTLOADER_INSTALLED_FW_STATUS_CORRUPTED;
                    }
                }
            }


            if (bootloader.upgrade_status==BOOTLOADER_INSTALLED_FW_STATUS_FOUND) {
                if (bootloader.upgrade_info.info.fw_size>bootloader.sections.upgrade.size) {
                    bootloader.upgrade_status=BOOTLOADER_INSTALLED_FW_STATUS_CORRUPTED;
                } else {
                    uint32_t crc = bootloader.config->crc32_func(bootloader.sections.upgrade.address,
                                                        bootloader.upgrade_info.info.fw_size);

                    /* Compare expected CRC with installed FW CRC */
                    if( bootloader.upgrade_info.info.fw_crc == crc ) {
                        bootloader.upgrade_status=BOOTLOADER_INSTALLED_FW_STATUS_VERIFIED;
                    } else {
                        bootloader.upgrade_status=BOOTLOADER_INSTALLED_FW_STATUS_CORRUPTED;
                    }
                }

            }
            bool need_to_copy=false;
            if (bootloader.upgrade_status == BOOTLOADER_INSTALLED_FW_STATUS_VERIFIED && bootloader.app_status!=BOOTLOADER_INSTALLED_FW_STATUS_VERIFIED) {
                //The app partition doesn't have a valid firmware but the upgrade partition yes
                need_to_copy=true;
            } else if (bootloader.upgrade_status == BOOTLOADER_INSTALLED_FW_STATUS_VERIFIED && bootloader.app_status == BOOTLOADER_INSTALLED_FW_STATUS_VERIFIED) {
                //Both partition have a valid firmware
                if (memcmp(&bootloader.app_info,&bootloader.upgrade_info, sizeof(bootloader_installed_fw_t))!=0) {
                    // the App has a different version from the upgrade, coyp the upgrade
                    need_to_copy=true;
                }
            }

            if (need_to_copy) {
                if (bootloader.copy_retries>=bootloader.config->max_copy_retries) {
                    //We've tried too many times, just go to DATA comm
                    bootloader.upgrade_status = BOOTLOADER_INSTALLED_FW_STATUS_CORRUPTED;

                    bootloader.state=BOOT_STATE_START_DATA_COMM;
                } else {
                    bootloader.state = BOOT_STATE_COPY_UPGRADE;
                    bootloader.copy_retries++;
                }
            } else {
                bootloader.state = BOOT_STATE_START_DATA_COMM;
            }
            break;
        }
        case BOOT_STATE_COPY_UPGRADE: {
            bootloader.upgrade_status = BOOTLOADER_INSTALLED_FW_STATUS_NOT_FOUND;
            bootloader.app_status = BOOTLOADER_INSTALLED_FW_STATUS_NOT_FOUND;

            bootloader.config->mem_copy_func(bootloader.sections.upgrade.address, bootloader.sections.app.address, bootloader.upgrade_info.info.fw_size);
            //
            // //Start copying the upgrade to the app
            // uint32_t remaining=bootloader.upgrade_info.info.fw_size;
            // uint32_t index=0;
            // while (remaining>0) {
            //     uint32_t size_to_copy = remaining;
            //     if (size_to_copy>bootloader.config->btea_chunk_size) {
            //         size_to_copy=bootloader.config->btea_chunk_size;
            //     }
            //     memset(bootloader.config->btea_buffer,0x00,bootloader.config->btea_chunk_size);
            //     bootloader.config->mem_write_func(bootloader.sections.app.address + index, bootloader.config->btea_buffer, size_to_copy);
            //     index+=size_to_copy;
            //     remaining-=size_to_copy;
            // }
            bootloader.config->mem_write_func(bootloader.sections.app_info.address,&bootloader.upgrade_info,sizeof(bootloader.upgrade_info));
            bootloader.state=BOOT_STATE_INIT;
            break;
        }
        case BOOT_STATE_START_DATA_COMM:{
            bootloader.fw_index = 0;
            bootloader.btea_index = 0;
            memset(bootloader.config->btea_buffer,0xFF,bootloader.config->btea_chunk_size);
            data_comm_rx_start(&bootloader.data_comm,time_ms);
            bootloader.state = BOOT_STATE_IDLE;
            bootloader.start_time = time_ms;
            break;
        }
        case BOOT_STATE_IDLE: {

            if( bootloader.app_status == BOOTLOADER_INSTALLED_FW_STATUS_VERIFIED) {
                bool timeout=false;
                if (!bootloader.stay_in_bootloader && bootloader.config->jump_delay>0) {
                    timeout = (time_ms-bootloader.start_time)>=bootloader.config->jump_delay;
                }
                if (bootloader.jump_to_app || timeout) {
                    bootloader.state=BOOT_STATE_GO_TO_APP;
                    break;
                }
            }
            data_comm_rx_tick(&bootloader.data_comm,time_ms);
            if (!data_comm_rx_is_running(&bootloader.data_comm)) {
                LOG_D("Bootloader Init");
                bootloader.state = BOOT_STATE_INIT;
            }
            bootloader.receiving = data_comm_rx_is_receiving(&bootloader.data_comm);
            break;
        }
        case BOOT_STATE_GO_TO_APP:
            bootloader.config->jump_to_app_func();
            break;
    }
}

void bootloader_rx_message_received(uint32_t time_ms, data_comm_msg_type_t type, uint8_t id, const uint8_t* data, uint16_t size){
    data_comm_rx_process_message( &(bootloader.data_comm), time_ms, type, id, data, size);
}

void bootloader_start_app( bool jump )
{
    // if (bootloader.app_found && bootloader.app_verified && bootloader.state == BOOT_STATE_IDLE) {
        bootloader.jump_to_app=jump;
    // }
    // return false;
}

void bootloader_stay(bool stay) {
    bootloader.stay_in_bootloader=stay;
}

uint32_t bootloader_get_installed_fw_version(void)
{
    return bootloader.app_info.info.fw_version;
}

bool bootloader_is_receiving() {
    return bootloader.receiving;
}
