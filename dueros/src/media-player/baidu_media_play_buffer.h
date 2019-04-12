// Copyright 2017 Baidu Inc. All Rights Reserved.
// Author: Chen Xihao (chenxihao@baidu.com)
//
// Description: Buffer for media player

#ifndef BAIDU_TINYDU_IOT_OS_SRC_MEDIA_PLAYER_BAIDU_MEDIA_PLAY_BUFFER_H
#define BAIDU_TINYDU_IOT_OS_SRC_MEDIA_PLAYER_BAIDU_MEDIA_PLAY_BUFFER_H

#include "rtos.h"
#include "baidu_buffer_base.h"

namespace duer {

// FIFO circle buffer
class MediaPlayBuffer {
public:
    class IListener {
    public:
        virtual ~IListener() {
        }

        virtual void on_wait_read() = 0;

        virtual void on_release_read() = 0;
    };

    MediaPlayBuffer(IBuffer* buffer);

    ~MediaPlayBuffer();

    // write buffer until buffer has enough space
    void write(const void* data, size_t size);

    // read buffer until buffer has enough data
    void read(void* data, size_t size);

    void clear();

    void set_listener(IListener* listener);

    static const size_t MIN_BUFFER_SIZE = 1*1024;//5 * 1024;

    static const size_t DEFAULT_BUFFER_SIZE = 2 * 1024;//10 * 1024;

private:
    MediaPlayBuffer(const MediaPlayBuffer&);

    MediaPlayBuffer& operator=(const MediaPlayBuffer&);

    IBuffer* _buffer;
    const size_t _BUFFER_SIZE;
    Mutex _mutex;
    bool _wait;
    Semaphore _semaphore;
    int _start;
    int _end;
    IListener* _listener;
};

} // namespace duer

#endif // BAIDU_TINYDU_IOT_OS_SRC_MEDIA_PLAYER_BAIDU_MEDIA_PLAY_BUFFER_H
