// Copyright 2017 Baidu Inc. All Rights Reserved.
// Author: Chen Xihao (chenxihao@baidu.com)
//
// Description: baidu common buffer

#ifndef BAIDU_IOT_TINYDU_IOT_OS_SRC_UTILITY_BAIDU_COMMON_BUFFER_H
#define BAIDU_IOT_TINYDU_IOT_OS_SRC_UTILITY_BAIDU_COMMON_BUFFER_H

#include "baidu_buffer_base.h"

namespace duer {

/**
 * heap or data segment buffer
 */
class CommonBuffer : public IBuffer {
public:
    /*
     * if buffer malloc from heap, must use MALLOC marco of heap_monitor.h.
     * if free is ture, buffer will be freed when CommonBuffer destructor.
     * data segment buffer should not be freed
     */
    CommonBuffer(void* buffer, size_t size, bool free);

    virtual ~CommonBuffer();

    virtual int write(size_t pos, const void* buffer, size_t length);

    virtual int read(size_t pos, void* buffer, size_t length);

    virtual size_t size();

    virtual int get_type();

private:
    CommonBuffer(const CommonBuffer&);

    CommonBuffer& operator=(const CommonBuffer&);

    void*  _buffer;
    size_t _size;
    bool   _free;
};

} // namespace duer

#endif // BAIDU_IOT_TINYDU_IOT_OS_SRC_UTILITY_BAIDU_COMMON_BUFFER_H
