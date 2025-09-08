//
// Created by nico on 1/11/18.
//

#include "../utils.h"
#include "nand_flash.h"

#include <assert.h>
#include <string.h>
#include <stdbool.h>



#if NAND_LOG_LEVEL <= NAND_LOG_LEVEL_DEBUG
#define LOG_D(fmt,...) \
if(nand->log_func) nand->log_func(NAND_LOG_LEVEL_DEBUG,fmt "\n",##__VA_ARGS__)
#else
#define LOG_D(fmt,...)
#endif

#if NAND_LOG_LEVEL <= NAND_LOG_LEVEL_INFO
#define LOG_I(fmt,...) \
if(nand->log_func) nand->log_func(NAND_LOG_LEVEL_INFO,fmt "\n",##__VA_ARGS__)
#else
#define LOG_I(fmt,...)
#endif

#if NAND_LOG_LEVEL <= NAND_LOG_LEVEL_WARNING
#define LOG_W(fmt,...) \
if(nand->log_func) nand->log_func(NAND_LOG_LEVEL_WARNING,fmt "\n",##__VA_ARGS__)
#else
#define LOG_W(fmt,...)
#endif


#if NAND_LOG_LEVEL <= NAND_LOG_LEVEL_ERROR
#define LOG_E(fmt,...) \
if(nand->log_func) nand->log_func(NAND_LOG_LEVEL_ERROR,fmt "\n",##__VA_ARGS__)
#else
#define LOG_E(fmt,...)
#endif




//static void my_memcpy(uint8_t* dst, uint8_t* src, uint32_t size){
//    for (uint32_t i=0;i<size;i++){
//        dst[i]=src[i];
//    }
//}
//
//static void my_memset(uint8_t* dst, uint8_t value, uint32_t size){
//    for (uint32_t i=0;i<size;i++){
//        dst[i]=value;
//    }
//}


static bool get_page(const nand_t* nand, uint32_t address, nand_page_t* page){
    return nand->get_page_func(address,page,nand->context);
}

typedef struct {
    bool erase_need;
    uint32_t read_crc;
    uint32_t expected_crc;
    struct{
        uint8_t check_erase:1;
        uint8_t calc_read_crc:1;
        uint8_t calc_expected_crc:1;
    } settings;
} nand_check_t;
static uint8_t check_erase(const nand_t *nand, const nand_page_t* page, uint32_t address, uint32_t size,
                           const uint8_t * data, uint32_t data_size, uint32_t data_offset, nand_check_t *p_result){
    if (size<data_size+data_offset){
        return NAND_ERROR_OOB;
    }
    uint32_t remaining=size;
    uint32_t check_index=0;
    if (p_result->settings.calc_read_crc==1) {
        p_result->read_crc = 0;
    }
    if (p_result->settings.calc_expected_crc==1) {
        p_result->expected_crc = 0;
    }
    if (p_result->settings.check_erase==1) {
        p_result->erase_need = false;
    }
    while(remaining>0) {
        uint32_t check_size = remaining;
        if (check_size > page->buffer_size) {
            check_size = page->buffer_size;
        }
        memset(page->buffer, 0xFF, page->buffer_size);
        if (p_result->settings.calc_read_crc == 1 || p_result->settings.check_erase == 1 ) {
            uint8_t result = nand->read_func(address + check_index, page->buffer, check_size,nand->context);
            if (result != NAND_STATUS_SUCCESS) {
                LOG_E("NAND_WE ERROR reading 0x%02x", result);
                return result;
            }
        }
        if (p_result->settings.calc_read_crc == 1) {
            p_result->read_crc = calc_crc32(page->buffer, check_size, NAND_CRC_SEED, p_result->read_crc);
        }
        if (p_result->settings.calc_expected_crc == 1 || p_result->settings.check_erase==1) {

            uint32_t start;
            if (data_offset<check_index){
                start= check_index;
            } else {
                start = data_offset;
            }

            uint32_t end;

            uint32_t end_data=data_offset+data_size;
            uint32_t end_check=check_index+check_size;

            if (end_data<end_check){
                end = end_data;
            } else {
                end = end_check;
            }

            if (start<end ){
                uint32_t buffer_addr = start - check_index;
                uint32_t data_addr = start - data_offset;
                for (uint32_t i=0;i<(end-start);++i){
                    uint8_t byte_read = page->buffer[buffer_addr+i];
                    uint8_t byte_data = (data == NULL ? 0xFF : data[data_addr + i]);
                    if (p_result->settings.check_erase && !p_result->erase_need &&
                        nand->check_func(byte_read, byte_data)) {
                        p_result->erase_need = true;
                        if (p_result->settings.calc_read_crc == 0 && p_result->settings.calc_expected_crc == 0) {
                            return NAND_STATUS_SUCCESS;
                        }
                    }
                    page->buffer[buffer_addr+i] = byte_data;
                }


            }
        }
        if (p_result->settings.calc_expected_crc == 1) {
            p_result->expected_crc = calc_crc32(page->buffer, check_size, NAND_CRC_SEED, p_result->expected_crc);
        }
        remaining -= check_size;
        check_index += check_size;
    }
    return NAND_STATUS_SUCCESS;
}

uint8_t nand_write_erase(const nand_t *nand, uint32_t address, const uint8_t *data, uint32_t size, bool always_backup) {
    uint8_t result=NAND_STATUS_SUCCESS;
    if (size==0){
        LOG_D("NAND_WE done size=0");
        return result;
    }

    LOG_D("NAND_WE start addr:0x%08x s:%d del:%d base:0x%08x",address,size,data==NULL,nand->base_address);
    uint32_t remaining_size = size;
    uint32_t data_index = 0;
    uint32_t start_address=(address-nand->base_address);   //address in page
    do {
        nand_page_t m_page;
        if (!get_page(nand, start_address, &m_page)) {
            LOG_E("NAND_WE ERROR NO page in addr 0x%08x", start_address);
            return NAND_ERROR_OOB;
        }
        nand_page_t *page = &m_page;
        uint32_t page_start = page->start_address;
        uint32_t size_to_copy = remaining_size;
        uint32_t page_address = start_address - page_start;
        LOG_D("NAND_WE start di:%d, rs:%d addr:0x%08x", data_index, remaining_size, start_address);

        LOG_D("NAND_WE page {addr:0x%08x s:%d i:%d } pa:%d, rem:%d",
                   page->start_address, page->size, page->index, page_address, remaining_size);

        uint32_t fit_size = page->size - page_address;  //How much fits into this page

        if (size_to_copy > fit_size) {
            LOG_D("NAND_WE trimming %d=>%d", size_to_copy, fit_size);
            size_to_copy = fit_size;
        }

        if (page->buffer == NULL || page->buffer_size==0) {
            LOG_E("NAND_WE ERROR NO buffer");
            return NAND_ERROR_NO_BUFFER;
        }
        nand_check_t check;
        check.settings.check_erase=1;
        check.settings.calc_read_crc=1;
        check.settings.calc_expected_crc=1;
        const uint8_t* p_data=(data==NULL?NULL:&data[data_index]);
        result = check_erase(nand,page,page_start,page->size,p_data,size_to_copy,page_address,&check);
        if (result != NAND_STATUS_SUCCESS) {
            LOG_E("NAND_WE ERROR erase 0x%02x", result);
            return result;
        }

        if (page->buffer_size < page->size) {
            //Backup is not possible, the page size is too big
            if (check.erase_need && always_backup) {
                LOG_E("NAND_WE Insufficient Buffer %d < %d", page->buffer_size,page->size);
                return NAND_ERROR_INSUFICIENT_BUFFER;
            }


            if (check.erase_need){
                LOG_I("NAND_WE NO BACKUP erasing %d (0x%08x, size: 0x%08x)", page->index,page_start,page->size);
                result = nand->erase_page_func(page->index, page_start, page->size,nand->context);
                if (result != NAND_STATUS_SUCCESS) {
                    LOG_E("NAND_WE ERROR erase 0x%02x", result);
                    return result;
                }

            }
            if(data!=NULL) {
                //Data is null, so it was erasing
                result = nand->write_func(start_address, p_data, size_to_copy,nand->context);
                if (result != NAND_STATUS_SUCCESS) {
                    LOG_E("NAND_WE ERROR write 0x%02x", result);
                    return result;
                }
            }
            check.settings.check_erase=0;
            check.settings.calc_read_crc=1;
            check.settings.calc_expected_crc=1;
            result = check_erase(nand,page,start_address,size_to_copy,p_data,size_to_copy,0,&check);
            //Calculate the final crc
            if (result != NAND_STATUS_SUCCESS) {
                LOG_E("NAND_WE ERROR check 0x%02x", result);
                return result;
            }

        } else {
            //Buffer is enough for loading the entire page
            // Load the entire page into the buffer
            /*LOG_D("NAND_WE reading 0x%08x %d", page_start, page->size);
            result = nand->read_func(nand->context, page_start,
                                                                page->buffer, page->size);
            if (result != NAND_STATUS_SUCCESS) {
                LOG_E("NAND_WE ERROR read 0x%02x", result);
                return result;
            }


            //Check if the data can be written without deleting the page
            bool erase = false;
            for (uint32_t i = 0; i < size_to_copy; ++i) {
                uint8_t nand_byte = page->buffer[page_address + i];
                uint8_t data_byte = ((data == NULL) ? 0xFF : data[data_index + i]);
                if (!erase && nand->callbacks.check_func(nand_byte,data_byte)){
                    erase=true;
                }
                page->buffer[page_address + i]=data_byte;
            }
            LOG_D("NAND_WE sb:%d se:%d ep:%d sc:%d", skip_beginning, skip_end, erase_page,
                       size_to_copy);
            expected_crc = calc_crc32(page->buffer, page->size, NAND_CRC_SEED, 0);
            LOG_D("NAND_WE crc 0x%08x", expected_crc);*/
            if (!check.erase_need) {
                LOG_D("NAND_WE no erase");
                //there's no need to erase the page to write the data
                if (data != NULL) {
                    LOG_D("NAND_WE write to 0x%08x data[%d] s: %d", start_address, data_index, size_to_copy);
                    result = nand->write_func(start_address, p_data, size_to_copy,nand->context);
                    if (result != NAND_STATUS_SUCCESS) {
                        LOG_E("NAND_WE ERROR write 0x%02x", result);
                        return result;
                    }
                }


            } else {
                LOG_D("NAND_WE erasing page %d 0x%08x %d", page->index, page_start, page->size);
                // Since we already backed up the entire page into the buffer, we just need to modify the buffer
                // with the wanted data at the wanted location
                result = nand->erase_page_func(page->index, page_start, page->size,nand->context);
                if (result != NAND_STATUS_SUCCESS) {
                    LOG_E("NAND_WE ERROR erase 0x%02x", result);
                    return result;
                }
                // if I'm trying to delete (data==NULL) the whole page (buffer_index==0 and size_to_copy == nand->page_size)
                // then we don't need to write anything
                LOG_D("NAND_WE erasing done %d %d %d", data != NULL, page_address != 0,
                           size_to_copy != page->size);
                if (data != NULL || page_address != 0 || size_to_copy != page->size) {
                    //There's actually something to write
                    LOG_D("NAND_WE writing all buffer in 0x%08x s:%d", page_start, page->size);
                    result = nand->write_func(page_start, (uint8_t *) page->buffer, page->size,nand->context);
                    if (result != NAND_STATUS_SUCCESS) {
                        LOG_E("NAND_WE ERROR write 0x%02x", result);
                        return result;
                    }
                }

            }

            check.settings.check_erase=0;
            check.settings.calc_read_crc=1;
            check.settings.calc_expected_crc=0;
            result = check_erase(nand,page,page_start,page->size,p_data,size_to_copy,page_address,&check);
            if (result != NAND_STATUS_SUCCESS) {
                LOG_E("NAND_WE ERROR read chk 0x%02x", result);
                return result;
            }
//        for (uint32_t i = 0; i<page->size;++i){
//            if (page->buffer[i]!=0xFF){
//                return i;
//            }
//        }

            LOG_D("NAND_WE reading done crc: 0x%08x", check.read_crc);



        }
        if (check.expected_crc != check.read_crc) {
            LOG_E("NAND_WE ERROR CRC 0x%08x!=0x%08x", check.expected_crc, check.read_crc);
            return NAND_ERROR_CRC_MISMATCH;
        }
        data_index += size_to_copy;
        remaining_size -= size_to_copy;
        start_address += size_to_copy;
        LOG_D("NAND_WE next di:%d, rs:%d addr:0x%08x",data_index,remaining_size,start_address);
    } while(remaining_size>0);
    return NAND_STATUS_SUCCESS;
}

uint8_t nand_move(const nand_t *nand, uint32_t src_addr, uint32_t dst_addr, uint32_t size) {
    uint8_t result=NAND_STATUS_SUCCESS;
    if (src_addr==dst_addr || size==0){
        return result;
    }

    uint32_t remaining_size = size;

    if (src_addr>dst_addr){
        //Source is ahead of destination
        uint32_t dst_start=dst_addr-nand->base_address;
        uint32_t src_start=src_addr-nand->base_address;

        do {

            nand_page_t m_page;
            if (!get_page(nand,dst_addr,&m_page)){
                return NAND_ERROR_OOB;
            }
            nand_page_t *page = &m_page;
            if (page->buffer==NULL){
                return NAND_ERROR_NO_BUFFER;
            }
            uint32_t page_start=page->start_address;

//            uint32_t page = dst_start / nand->page_size;

            uint32_t dst_end = dst_start + remaining_size;

//            uint32_t page_start = page * nand->page_size;
            uint32_t page_end = page_start+page->size;

            uint32_t buffer_index = 0;
            if (dst_start > page_start) {
                //The destination is ahead of the start of the page
                //Copy what needs to be restored that won't be modified
                result =nand->read_func(page_start, &page->buffer[buffer_index], dst_start-page_start,nand->context);
                if(result!=NAND_STATUS_SUCCESS){
                    return result;
                }
                buffer_index += dst_start - page_start;
            }
            if (dst_end>page_end){
                //The end of the destination lies outside the page
                dst_end = page_end;
            }

            uint32_t size_to_copy = dst_end - dst_start;
            //Put in the buffer the data that comes from the src
            result=nand->read_func(src_start, &page->buffer[buffer_index], size_to_copy,nand->context);
            if(result!=NAND_STATUS_SUCCESS){
                return result;
            }
            buffer_index += size_to_copy;
            if (buffer_index < page->size) {
                //backup end
                result =nand->read_func(page_start+buffer_index,&page->buffer[buffer_index], page->size - buffer_index,nand->context);
                if(result!=NAND_STATUS_SUCCESS){
                    return result;
                }
            }
            //clean the page
            result =nand->erase_page_func(page->index, page_start, page->size,nand->context);
            if (result!=NAND_STATUS_SUCCESS){
                return result;
            }
            result=nand->write_func(page_start, page->buffer, page->size,nand->context);
            if(result!=NAND_STATUS_SUCCESS){
                return result;
            }

            remaining_size -= size_to_copy;
            dst_start += size_to_copy;
            src_start += size_to_copy;

        }while(remaining_size>0);
        return NAND_STATUS_SUCCESS;


    } else {
        //destination is below source

        uint32_t dst_end = dst_addr + size;
        uint32_t src_end = src_addr + size;


        do {
//            uint32_t page = (dst_end - 1) / nand->page_size;

            nand_page_t m_page;
            if (!get_page(nand,dst_end-1,&m_page)){
                return NAND_ERROR_OOB;
            }
            nand_page_t *page = &m_page;
            if (page->buffer==NULL){
                return NAND_ERROR_NO_BUFFER;
            }
            uint32_t page_start=page->start_address;
            uint32_t dst_start = dst_end - remaining_size;

//            uint32_t page_start = page * nand->page_size;

            uint32_t buffer_index = 0;
            if (dst_start <= page_start) {
                dst_start = page_start;
            } else {
                //backup begining
                result=nand->read_func(page_start, &page->buffer[buffer_index], dst_start - page_start,nand->context);
                if(result!=NAND_STATUS_SUCCESS){
                    return result;
                }
                buffer_index += dst_start - page_start;
            }
            uint32_t size_to_copy = dst_end - dst_start;

            result=nand->read_func(src_end - size_to_copy, &page->buffer[buffer_index], size_to_copy,nand->context);
            if(result!=NAND_STATUS_SUCCESS){
                return result;
            }
            buffer_index += size_to_copy;
            if (buffer_index < page->size) {
                //backup end
                result =nand->read_func(page_start + buffer_index, &page->buffer[buffer_index],
                                        page->size - buffer_index,nand->context);
                if(result!=NAND_STATUS_SUCCESS){
                    return result;
                }
            }
            //clean the page
            result =nand->erase_page_func(page->index,page_start, page->size,nand->context);
            if(result!=NAND_STATUS_SUCCESS){
                return result;
            }
            result=nand->write_func(page_start, page->buffer, page->size,nand->context);
            if (result!=NAND_STATUS_SUCCESS){
                return result;
            }

            remaining_size -= size_to_copy;
            dst_end -= size_to_copy;
            src_end -= size_to_copy;
        } while (remaining_size > 0);
        return NAND_STATUS_SUCCESS;
    }
}

void nand_init_irregular_pages(nand_page_t *page, const uint32_t start_address, const uint32_t page_count, const uint32_t *sizes, uint8_t *const *buffers) {
    uint32_t addresss = start_address;
    for (uint32_t i=0;i<page_count;++i){
        page[i].start_address=addresss;
        page[i].size=sizes[i];
        page[i].index=i;
        page[i].buffer=buffers[i];
        addresss+=sizes[i];
    }

}

void
nand_init_pages(nand_page_t *page, const uint32_t start_address, const uint32_t page_count, uint32_t page_size,
                uint8_t * const page_buffer) {
    uint32_t addresss = start_address;
    for (uint32_t i=0;i<page_count;++i){
        page[i].start_address=addresss;
        page[i].size=page_size;
        page[i].index=i;
        page[i].buffer=page_buffer;
        addresss+=page_size;
    }
}
