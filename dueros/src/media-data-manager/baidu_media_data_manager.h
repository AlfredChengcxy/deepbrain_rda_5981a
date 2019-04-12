// Copyright 2017 Baidu Inc. All Rights Reserved.
// Author: Chen Xihao (chenxihao@baidu.com)
//
// Description: Media data Manager

#ifndef BAIDU_TINYDU_IOT_OS_SRC_MEDIA_DATA_MANAGER_BAIDU_MEDIA_DATA_MANAGER_H
#define BAIDU_TINYDU_IOT_OS_SRC_MEDIA_DATA_MANAGER_BAIDU_MEDIA_DATA_MANAGER_H

#include <stddef.h>
#include "baidu_media_play_type.h"

namespace duer {

void mdm_send_media_file_path(const char* media_file_path, int flags);

void mdm_send_media_url(const char* media_url, int flags);

void mdm_send_media_data(const char* data, size_t size, int flags);

void mdm_send_magic_voice(char* data, int size, int flags);

MediaPlayerStatus mdm_pause_or_resume();

MediaPlayerStatus mdm_notify_to_stop();

MediaPlayerStatus mdm_notify_to_stop_completely();

void mdm_reset_stop_flag();

int mdm_check_need_to_stop();

void start_media_data_mgr_thread();

void mdm_get_download_progress(size_t* total_size, size_t* recv_size);

void mdm_seek_media(int position);

int mdm_set_previous_media(const char *url ,int flags);

int mdm_play_previous_media_continue();

void mdm_reg_play_failed_cb(mdm_media_play_failed_cb cb);
} // namespace duer

#endif // BAIDU_TINYDU_IOT_OS_SRC_MEDIA_DATA_MANAGER_BAIDU_MEDIA_DATA_MANAGER_H
