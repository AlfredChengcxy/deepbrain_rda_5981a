// Copyright 2017 Baidu Inc. All Rights Reserved.
// Author: Chen Xihao (chenxihao@baidu.com)
//
// Description: wrapper of SpiRAM

#ifndef BAIDU_IOT_TINYDU_IOT_OS_SRC_UTILITY_BAIDU_BUFFER_H
#define BAIDU_IOT_TINYDU_IOT_OS_SRC_UTILITY_BAIDU_BUFFER_H

#ifdef PSRAM_ENABLED

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

int psram_write_buffer(uint32_t address, void* buffer, uint32_t length);

int psram_read_buffer(uint32_t address, void* buffer, uint32_t length);

#ifdef __cplusplus
}
#endif

#endif // PSRAM_ENABLED

#endif // BAIDU_IOT_TINYDU_IOT_OS_SRC_UTILITY_BAIDU_BUFFER_H
