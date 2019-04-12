// Copyright 2017 Baidu Inc. All Rights Reserved.
// Author: Chen Xihao (chenxihao@baidu.com)
//
// Description: wrapper of SpiRAM

#include "baidu_psram_wrapper.h"

#ifdef PSRAM_ENABLED
#include "SpiRAM.h"

extern SpiRAM g_psram;

int psram_write_buffer(uint32_t address, void* buffer, uint32_t length) {
    return g_psram.writeBuffer(address, buffer, length);
}

int psram_read_buffer(uint32_t address, void* buffer, uint32_t length) {
    return g_psram.readBuffer(address, buffer, length);
}

#endif // PSRAM_ENABLED
