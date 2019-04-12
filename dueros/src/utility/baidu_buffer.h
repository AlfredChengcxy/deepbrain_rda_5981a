// Copyright 2017 Baidu Inc. All Rights Reserved.
// Author: Chen Xihao (chenxihao@baidu.com)
//
// Description: baidu buffer for c

#ifndef BAIDU_IOT_TINYDU_IOT_OS_SRC_UTILITY_BAIDU_BUFFER_H
#define BAIDU_IOT_TINYDU_IOT_OS_SRC_UTILITY_BAIDU_BUFFER_H

#include "stdint.h"
#include "stdio.h"
#include "stdbool.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct DuerBuffer {
    void* specific_buffer;

    /**
     * @brief write data to buffer
     *
     * @param[in] duer_buffer point of buffer
     * @param[in] pos position of buffer to write
     * @param[in] buffer source data
     * @param[in] length length of write data
     *
     * @return 0:success -1:fail
     */
    int (*write)(struct DuerBuffer* duer_buffer, size_t pos, const void* buffer, size_t length);

    /**
     * @brief read data from buffer
     *
     * @param[in] duer_buffer point of buffer
     * @param[in] pos position of buffer to read
     * @param[in] buffer destination data
     * @param[in] length length of read data
     *
     * @return 0:success -1:fail
     */
    int (*read)(struct DuerBuffer* duer_buffer, size_t pos, void* buffer, size_t length);

    /**
     * @brief get size of buffer
     */
    size_t (*size)(struct DuerBuffer* duer_buffer);
} DuerBuffer;

/**
 * create heap or data segment buffer
 *
 * @param[in] heap or data segment buffer
 * @param[in] size of buffer
 * @param[in] indicate whether free buffer when destory DuerBuffer
 *
 * @return point of DuerBuffer
 */
DuerBuffer* create_common_buffer(void* buffer, size_t size, bool need_free);

/**
 * destory DuerBuffer created by create_common_buffer
 */
void destroy_common_buffer(DuerBuffer* duer_buffer);

/**
 * create file buffer
 *
 * @param[in] fd of file which must be readable and writable
 * @param[in] size of file
 *
 * @return point of DuerBuffer
 */
DuerBuffer* create_file_buffer(FILE* file, size_t size);

/**
 * destory DuerBuffer created by create_file_buffer
 */
void destroy_file_buffer(DuerBuffer* duer_buffer);

#ifdef PSRAM_ENABLED

/**
 * create psram buffer
 * @param[in] start address of buffer in psram
 * @param[in] size of buffer
 *
 * @return point of DuerBuffer
 */
DuerBuffer* create_psram_buffer(uint32_t address, size_t size);

/**
 * destory DuerBuffer created by create_psram_buffer
 */
void destroy_psram_buffer(DuerBuffer* duer_buffer);

#endif // PSRAM_ENABLED

#ifdef __cplusplus
}
#endif

#endif // BAIDU_IOT_TINYDU_IOT_OS_SRC_UTILITY_BAIDU_BUFFER_H
