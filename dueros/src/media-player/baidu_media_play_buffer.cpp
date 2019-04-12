// Copyright 2017 Baidu Inc. All Rights Reserved.
// Author: Chen Xihao (chenxihao@baidu.com)
//
// Description: Buffer for media player

#include "baidu_media_play_buffer.h"
#include "lightduer_log.h"

namespace duer {

#define LOG(...) DUER_LOGV(__VA_ARGS__)

MediaPlayBuffer::MediaPlayBuffer(IBuffer* buffer) :
        _buffer(buffer),
        _BUFFER_SIZE(buffer->size()),
        _wait(false),
        _start(-1),
        _end(-1),
        _listener(NULL) {
}

MediaPlayBuffer::~MediaPlayBuffer() {
    delete _buffer;
    _buffer = NULL;
}

void MediaPlayBuffer::write(const void* data, size_t size) {
    if (data == NULL || size < 1 || size > _BUFFER_SIZE) {
        return;
    }

    while (true) {
        _mutex.lock();
        size_t free = 0;
        if (_start == -1) {
            free = _BUFFER_SIZE;
            _start = _end = 0;
        } else if (_end > _start) {
            free = _BUFFER_SIZE - _end + _start;
        } else {
            free = _start - _end;
        }

        if (free >= size) {
            if (_end + size <= _BUFFER_SIZE) {
                _buffer->write(_end, data, size);
            } else {
                size_t part_size = _BUFFER_SIZE - _end;
                _buffer->write(_end, data, part_size);
                _buffer->write(0, (char*)data + part_size, size - part_size);
            }
            _end = (_end + size) % _BUFFER_SIZE;
            LOG("write size = %d, _start = %d, _end = %d", size, _start, _end);
            if (_wait) {
                _semaphore.release();
                _wait = false;
                if (_listener != NULL) {
                    _listener->on_release_read();
                }
            }
            _mutex.unlock();
            return;
        } else {
            _mutex.unlock();
            LOG("write wait free = %d", free);
            _wait = true;
            _semaphore.wait();
            LOG("write wait end");
        }
    }
}

void MediaPlayBuffer::read(void* data, size_t size) {
    if (data == NULL || size < 1 || size > _BUFFER_SIZE) {
        return;
    }

    while (true) {
        _mutex.lock();
        size_t valid = 0;
        if (_end > _start) {
            valid = _end - _start;
        } else if (_start != -1) {
            valid = _BUFFER_SIZE - _start + _end;
        } else {
            //do nothing
        }

        if (valid >= size) {
            if (_start + size <= _BUFFER_SIZE) {
                _buffer->read(_start, data, size);
            } else {
                size_t part_size = _BUFFER_SIZE - _start;
                _buffer->read(_start, data, part_size);
                _buffer->read(0, (char*)data + part_size, size - part_size);
            }
            _start = (_start + size) % _BUFFER_SIZE;
            if (_start == _end) {
                _start = _end = -1;
            }
            LOG("read size = %d, _start = %d, _end = %d", size, _start, _end);
            if (_wait) {
                _semaphore.release();
                _wait = false;
            }
            _mutex.unlock();
            return;
        } else {
            _mutex.unlock();
            LOG("read wait valid = %d", valid);
            if (_listener != NULL) {
                _listener->on_wait_read();
            }
            _wait = true;
            _semaphore.wait();
            LOG("read wait end");
        }
    }
}

void MediaPlayBuffer::clear() {
    _mutex.lock();
    _start = _end = -1;
    LOG("clear wait = %d", _wait);
    if (_wait) {
        _semaphore.release();
        _wait = false;
    }
    _mutex.unlock();
}

void MediaPlayBuffer::set_listener(IListener* listener) {
    _listener = listener;
}

} // namespace duer
