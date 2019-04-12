// Copyright 2017 Baidu Inc. All Rights Reserved.
// Author: Chen Xihao (chenxihao@baidu.com)
//
// Description: baidu buffer

#ifndef BAIDU_IOT_TINYDU_IOT_OS_SRC_UTILITY_BAIDU_BUFFER_BASE_H
#define BAIDU_IOT_TINYDU_IOT_OS_SRC_UTILITY_BAIDU_BUFFER_BASE_H

#include "stdint.h"
#include "stdio.h"

namespace duer {

/**
 * base class of all buffer class
 */
class IBuffer {
public:
    enum BufferType {
        BUFFER_TYPE_HEAP,
        BUFFER_TYPE_DATA_SEGMENT,
        BUFFER_TYPE_PSRAM,
        BUFFER_TYPE_FILE
    };

    virtual ~IBuffer() {}

    /**
     * @brief write data to buffer
     *
     * @param[in] pos position of buffer to write
     * @param[in] buffer source data
     * @param[in] length length of write data
     *
     * @return 0:success -1:fail
     */
    virtual int write(size_t pos, const void* buffer, size_t length) = 0;

    /**
     * @brief read data from buffer
     *
     * @param[in] pos position of buffer to read
     * @param[in] buffer destination data
     * @param[in] length length of read data
     *
     * @return 0:success -1:fail
     */
    virtual int read(size_t pos, void* buffer, size_t length) = 0;

    /**
     * @brief get size of buffer
     */
    virtual size_t size() = 0;

    /**
     * @brief get the type of buffer
     */
    virtual int get_type() = 0;
};

} // namespace duer

#endif // BAIDU_IOT_TINYDU_IOT_OS_SRC_UTILITY_BAIDU_BUFFER_BASE_H
