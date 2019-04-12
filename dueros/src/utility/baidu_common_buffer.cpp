// Copyright 2017 Baidu Inc. All Rights Reserved.
// Author: Chen Xihao (chenxihao@baidu.com)
//
// Description: baidu common buffer

#include "baidu_common_buffer.h"
#include <string.h>
#include "lightduer_log.h"
#include "heap_monitor.h"

namespace duer {

CommonBuffer::CommonBuffer(void* buffer, size_t size, bool free) :
        _buffer(buffer),
        _size(size),
        _free(free) {
    if (_buffer == NULL || _size < 1) {
        DUER_LOGE("Invalid argument.");
        _size = 0;
    }
}

CommonBuffer::~CommonBuffer() {
    if (_free) {
        FREE(_buffer);
    }
}

int CommonBuffer::write(size_t pos, const void* buffer, size_t length) {
    if (pos + length > _size) {
        DUER_LOGE("out of buffer.");
        return -1;
    }

    memcpy((char*)_buffer + pos, buffer, length);
    return 0;
}

int CommonBuffer::read(size_t pos, void* buffer, size_t length) {
    if (pos + length > _size) {
        DUER_LOGE("out of buffer.");
        return -1;
    }

    memcpy(buffer, (char*)_buffer + pos, length);
    return 0;
}

size_t CommonBuffer::size() {
    return _size;
}

int CommonBuffer::get_type() {
    if (_free) {
        return BUFFER_TYPE_HEAP;
    } else {
        return BUFFER_TYPE_DATA_SEGMENT;
    }
}

} // namespace duer
