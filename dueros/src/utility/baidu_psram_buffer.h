// Copyright 2017 Baidu Inc. All Rights Reserved.
// Author: Chen Xihao (chenxihao@baidu.com)
//
// Description: baidu common buffer

#ifndef BAIDU_IOT_TINYDU_IOT_OS_SRC_UTILITY_BAIDU_PSRAM_BUFFER_H
#define BAIDU_IOT_TINYDU_IOT_OS_SRC_UTILITY_BAIDU_PSRAM_BUFFER_H

#include "baidu_buffer_base.h"

namespace duer {

#ifdef PSRAM_ENABLED
/**
 * psram buffer
 */
class PsramBuffer : public IBuffer {
public:
    PsramBuffer(uint32_t address, size_t size);

    virtual int write(size_t pos, const void* buffer, size_t length);

    virtual int read(size_t pos, void* buffer, size_t length);

    virtual size_t size();

    virtual int get_type();

private:
    PsramBuffer(const PsramBuffer&);

    PsramBuffer& operator=(const PsramBuffer&);

    uint32_t _address;
    size_t   _size;
};
#endif // PSRAM_ENABLED

} // namespace duer

#endif // BAIDU_IOT_TINYDU_IOT_OS_SRC_UTILITY_BAIDU_PSRAM_BUFFER_H
