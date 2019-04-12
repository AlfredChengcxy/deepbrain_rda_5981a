// Copyright 2017 Baidu Inc. All Rights Reserved.
// Author: Gang Chen (chengang12@baidu.com)
//
// Description: Media play progress statistic

#ifndef BAIDU_TINYDU_IOT_OS_SRC_MEDIA_DATA_MANAGER_BAIDU_MEDIA_PLAY_PROGRESS_H
#define BAIDU_TINYDU_IOT_OS_SRC_MEDIA_DATA_MANAGER_BAIDU_MEDIA_PLAY_PROGRESS_H

#include <stddef.h>
#include "baidu_media_play.h"

namespace duer {

class MdmMeaidaPlayerListener : public MediaPlayerListener {
public:
    virtual int on_start(int flags);

    virtual int on_stop(int flags);

    virtual int on_finish(int flags);
};

class MediaPlayBufferListener : public MediaPlayBuffer::IListener {
public:
    virtual void on_wait_read();

    virtual void on_release_read();
};

void mdm_restore_previous_play_progress();

void mdm_media_play_progress_init(MediaPlayBuffer* g_media_play_buffer);

int mdm_get_play_progress();

void mdm_media_register_stuttered_cb(media_stutter_cb cb);
} // namespace duer

#endif // BAIDU_TINYDU_IOT_OS_SRC_MEDIA_DATA_MANAGER_BAIDU_MEDIA_PLAY_PROGRESS_H
