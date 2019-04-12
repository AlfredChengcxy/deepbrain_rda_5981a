// Copyright 2017 Baidu Inc. All Rights Reserved.
// Author: Chen Xihao (chenxihao@baidu.com)
//
// Description: Media player API

#ifndef BAIDU_TINYDU_IOT_OS_SRC_MEDIA_PLAYER_BAIDU_MEDIA_PLAY_LISTENER_H
#define BAIDU_TINYDU_IOT_OS_SRC_MEDIA_PLAYER_BAIDU_MEDIA_PLAY_LISTENER_H

namespace duer {

class MediaPlayerListener {
public:
    virtual int on_start(int flags) = 0;

    virtual int on_stop(int flags) = 0;

    virtual int on_finish(int flags) = 0;

    virtual ~MediaPlayerListener() {};
};

} // namespace duer

#endif // BAIDU_TINYDU_IOT_OS_SRC_MEDIA_PLAYER_BAIDU_MEDIA_PLAY_LISTENER_H
