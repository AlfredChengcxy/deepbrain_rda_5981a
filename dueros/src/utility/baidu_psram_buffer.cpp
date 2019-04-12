// Copyright 2017 Baidu Inc. All Rights Reserved.
// Author: Chen Xihao (chenxihao@baidu.com)
//
// Description: baidu psram buffer

#include "baidu_psram_buffer.h"
#include "lightduer_log.h"
#ifdef PSRAM_ENABLED
#include "SpiRAM.h"
#include "baidu_psram_partition.h"
#endif

#ifdef PSRAM_ENABLED
SpiRAM g_psram(PD_2, PD_3, PD_0, PD_1, SPIRAM_MODE_BYTE);

namespace duer {

PsramBuffer::PsramBuffer(uint32_t address, size_t size) :
        _address(address),
        _size(size) {
    if (address + size > PSRAM_MAX_SIZE) {
        DUER_LOGE("Invalid argument.");
        _size = 0;
    }
}

int PsramBuffer::write(size_t pos, const void* buffer, size_t length) {
    if (pos + length > _size) {
        DUER_LOGE("out of buffer.");
        return -1;
    }

    return g_psram.writeBuffer(_address + pos, (void*)buffer, length);
}

int PsramBuffer::read(size_t pos, void* buffer, size_t length) {
    if (pos + length > _size) {
        DUER_LOGE("out of buffer.");
        return -1;
    }

    return g_psram.readBuffer(_address + pos, buffer, length);
}

size_t PsramBuffer::size() {
    return _size;
}

int PsramBuffer::get_type() {
    return BUFFER_TYPE_PSRAM;
}

} // namespace duer

#endif // PSRAM_ENABLED
