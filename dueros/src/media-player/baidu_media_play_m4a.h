// Copyright 2017 Baidu Inc. All Rights Reserved.
// Author: Chen Xihao (chenxihao@baidu.com)
//
// Description: Play m4a type file

#ifndef BAIDU_TINYDU_IOT_OS_SRC_MEDIA_PLAYER_BAIDU_MEDIA_PLAY_M4A_H
#define BAIDU_TINYDU_IOT_OS_SRC_MEDIA_PLAYER_BAIDU_MEDIA_PLAY_M4A_H

#include "lightduer_m4a_audio_specific_config.h"
#include "lightduer_m4a_unpacker.h"
#include "baidu_media_type.h"

namespace duer {

const int M4AERR_OK = 0;
const int M4AERR_MAC = -1;
const int M4AERR_ARG = -2;
const int M4AERR_VAL = -3;

class MediaAdapter;
class IBuffer;

class MediaPlayM4A {
public:
    MediaPlayM4A(MediaAdapter* media_adapter);

    ~MediaPlayM4A();

    int m4a_file_start(const unsigned char* p_buff, int buff_sz);

    int m4a_file_play(const unsigned char* p_buff, int buff_sz);

    int m4a_file_end(const unsigned char* p_buff, int buff_sz);

    int get_m4a_file_pos();

    /**
     * return real seek position
     */
    int m4a_file_seek(int file_pos);

    static const uint32_t MAX_M4A_HEADER_SIZE = 512 * 1024;

private:
    MediaPlayM4A(const MediaPlayM4A&);

    MediaPlayM4A& operator=(const MediaPlayM4A&);

    int m4a_header_save(const unsigned char* p_buff, int buff_sz);

    int m4a_header_parse();

    int m4a_frame_to_codec(const unsigned char* p_buff, int buff_sz);

    static uint32_t read_callback(void* user_data, void* buffer, uint32_t length);

    static uint32_t seek_callback(void* user_data, uint64_t position);

    static duer_m4a_file_callback_t _s_mp4cb;

    IBuffer* _buffer;
    uint32_t _buffer_position;
    int _m4a_header_size;
    int _m4a_header_size_saved;
    int _buff_data_size;
    unsigned char* _p_buff_data;
    int _m4a_file_pos;
    bool _m4a_parse_done;

    int _num_samples;
    int _track;
    int _sample_id;
    duer_m4a_audio_specific_config_t _mp4_asc;
    duer_m4a_unpacker_t _m4a_unpacker;

    MediaAdapter* _media_adapter;
};

} // namespace duer

#endif // BAIDU_TINYDU_IOT_OS_SRC_MEDIA_PLAYER_BAIDU_MEDIA_PLAY_M4A_H
