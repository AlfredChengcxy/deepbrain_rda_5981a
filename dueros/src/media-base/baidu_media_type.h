// Copyright 2017 Baidu Inc. All Rights Reserved.
// Author: Chen Xihao (chenxihao@baidu.com)
//
// Description: Media type

#ifndef BAIDU_TINYDU_IOT_OS_SRC_MEDIA_BASE_BAIDU_MEDIA_TYPE_H
#define BAIDU_TINYDU_IOT_OS_SRC_MEDIA_BASE_BAIDU_MEDIA_TYPE_H

namespace duer {

enum MediaType {
    TYPE_MP3,
    TYPE_AAC,
    TYPE_M4A,
    TYPE_WAV,
    TYPE_SPEECH, // low bit rate mp3
    TYPE_TS,
    TYPE_HLS,    // Http live stream(.m3u8)
    TYPE_AMR,
    TYPE_NULL
};

// internal error code, pass it to duer_ds_log_audio_player_error
enum MediaError {
    MEDIA_ERROR_NONE                        = 0,
    MEDIA_ERROR_FAILED                      = -1,
    MEDIA_ERROR_PLAY_M4A_NO_SD              = -2,
    MEDIA_ERROR_NO_PREVIOUS_HEADER          = -3,
    MEDIA_ERROR_NO_PREVIOUS_M4A             = -4,
    MEDIA_ERROR_M4A_OPEN_FILE_FAILED        = -5,
    MEDIA_ERROR_M4A_WRITE_BUFF_ERROR        = -6,
    MEDIA_ERROR_NO_PREVIOUS_URL             = -7,
};

} // namespace duer

#endif // BAIDU_TINYDU_IOT_OS_SRC_MEDIA_BASE_BAIDU_MEDIA_TYPE_H
