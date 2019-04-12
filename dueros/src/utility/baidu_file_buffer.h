// Copyright 2017 Baidu Inc. All Rights Reserved.
// Author: Chen Xihao (chenxihao@baidu.com)
//
// Description: baidu file buffer

#ifndef BAIDU_IOT_TINYDU_IOT_OS_SRC_UTILITY_BAIDU_FILE_BUFFER_H
#define BAIDU_IOT_TINYDU_IOT_OS_SRC_UTILITY_BAIDU_FILE_BUFFER_H

#include "baidu_buffer_base.h"

namespace duer {

/**
 * file buffer
 */
class FileBuffer : public IBuffer {
public:
    /*
     * max size of file is max_size
     */
    FileBuffer(FILE* file, size_t max_size);

    virtual ~FileBuffer();

    virtual int write(size_t pos, const void* buffer, size_t length);

    virtual int read(size_t pos, void* buffer, size_t length);

    virtual size_t size();

    virtual int get_type();

private:
    FileBuffer(const FileBuffer&);

    FileBuffer& operator=(const FileBuffer&);

    FILE*  _file;
    size_t _size;
};

} // namespace duer

#endif // BAIDU_IOT_TINYDU_IOT_OS_SRC_UTILITY_BAIDU_FILE_BUFFER_H
