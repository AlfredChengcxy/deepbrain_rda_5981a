// Copyright 2017 Baidu Inc. All Rights Reserved.
// Author: Chen Xihao (chenxihao@baidu.com)
//
// Description: baidu file buffer

#include "baidu_file_buffer.h"
#include "lightduer_log.h"

namespace duer {

FileBuffer::FileBuffer(FILE* file, size_t max_size) :
        _file(file),
        _size(max_size) {
    if (_file == NULL) {
        DUER_LOGE("Invalid argument.");
    }
}

FileBuffer::~FileBuffer() {
    fclose(_file);
}

int FileBuffer::write(size_t pos, const void* buffer, size_t length) {
    if (pos + length > _size) {
        DUER_LOGE("out of buffer.");
        return -1;
    }

    fseek(_file, pos, SEEK_SET);
    int ret = fwrite(buffer, 1, length, _file);
    fflush(_file);

    return ret == length ? 0 : -1;
}

int FileBuffer::read(size_t pos, void* buffer, size_t length) {
    if (pos + length > _size) {
        DUER_LOGE("out of buffer.");
        return -1;
    }

    fseek(_file, pos, SEEK_SET);
    return fread(buffer, 1, length, _file) == length ? 0 : -1;
}

size_t FileBuffer::size() {
    return _size;
}

int FileBuffer::get_type() {
    return BUFFER_TYPE_FILE;
}

} // namespace duer
