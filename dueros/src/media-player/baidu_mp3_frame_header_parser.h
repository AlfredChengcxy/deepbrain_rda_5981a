// Copyright 2017 Baidu Inc. All Rights Reserved.
// Author: Chen Xihao (chenxihao@baidu.com)
//
// Description: parse mp3 frame header

#ifndef BAIDU_IOT_TINYDU_IOT_OS_SRC_MEDIA_PLAYER_BAIDU_MP3_FRAME_HEADER_PARSER_H
#define BAIDU_IOT_TINYDU_IOT_OS_SRC_MEDIA_PLAYER_BAIDU_MP3_FRAME_HEADER_PARSER_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Get the bitrate of mp3 frame
 *
 * @Param buf, mp3 data included one frame header at least
 * @Param size, the size of the buf
 * @Return success: the bitrate, fail: 0
 */
int get_mp3_frame_bitrate(const uint8_t *buf, uint32_t size);

#ifdef __cplusplus
}
#endif

#endif // BAIDU_IOT_TINYDU_IOT_OS_SRC_MEDIA_PLAYER_BAIDU_MP3_FRAME_HEADER_PARSER_H
