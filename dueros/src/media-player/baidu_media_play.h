// Copyright 2017 Baidu Inc. All Rights Reserved.
// Author: Chen Xihao (chenxihao@baidu.com)
//
// Description: Media player API

#ifndef BAIDU_TINYDU_IOT_OS_SRC_MEDIA_PLAYER_BAIDU_MEDIA_PLAY_H
#define BAIDU_TINYDU_IOT_OS_SRC_MEDIA_PLAYER_BAIDU_MEDIA_PLAY_H

#include "baidu_media_play_type.h"
#include "baidu_media_type.h"
#include "baidu_media_play_buffer.h"
#include "baidu_media_play_listener.h"

namespace duer {

enum MediaPlayerCmd {
    PLAYER_CMD_START,       //to start playing media
    PLAYER_CMD_CONTINUE,    //media data stream
    PLAYER_CMD_STOP         //to stop mediaplayer
};

enum MediaVolumeRange {
    //MIN_VOLUME = 25,
    MIN_VOLUME = 0,
    DEFAULT_VOLUME = 50,
    MAX_VOLUME = /*80*/100,
};

struct MediaPlayerMessage {
    MediaPlayerCmd cmd;     //command to mediaplayer
    MediaType type;
    int size;               //data size in MediaPlayBuffer
    int flags;
};

void media_play_codec_on();
void media_play_codec_off();
void media_play_set_5856_pin_on(int pin_name);
void media_play_set_5856_pin_off(int pin_name);

void media_play_set_bt_mode();
void media_play_set_uart_mode();

void start_media_play_thread();
// return last status of media player
MediaPlayerStatus media_play_pause_or_resume();
MediaPlayerStatus media_play_stop(bool no_callback);
MediaPlayerStatus media_play_get_status();

int media_play_register_listener(MediaPlayerListener* listener);
int media_play_unregister_listener(MediaPlayerListener* listener);
void media_play_set_volume(unsigned char vol);
unsigned char media_play_get_volume();
int media_play_get_write_size();
int media_play_seek_m4a_file(int position);
bool media_play_finished();
void media_play_clear_previous_m4a();
bool media_play_error();
void media_play_reset_error();
void media_play_wait_end();

void media_record_start_record();
void media_record_stop_record();
int media_record_read_data(char *data, int size);

void media_bt_play();
void media_bt_pause();
void media_bt_forward();
void media_bt_backward();
void media_bt_volup();
void media_bt_voldown();
void media_set_bt_name(const unsigned char* name);

extern MediaPlayBuffer* g_media_play_buffer;

} // namespace duer

#endif // BAIDU_TINYDU_IOT_OS_SRC_MEDIA_PLAYER_BAIDU_MEDIA_PLAY_H
