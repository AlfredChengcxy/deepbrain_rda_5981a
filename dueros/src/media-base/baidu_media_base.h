// Copyright 2017 Baidu Inc. All Rights Reserved.
// Author: Chen Xihao (chenxihao@baidu.com)
//
// Description: Base class of Media

#ifndef BAIDU_TINYDU_IOT_OS_SRC_MEDIA_BASE_BAIDU_MEDIA_BASE_H
#define BAIDU_TINYDU_IOT_OS_SRC_MEDIA_BASE_BAIDU_MEDIA_BASE_H

#include "mbed.h"
#include "baidu_media_type.h"

namespace duer {

typedef struct duer_ds_audio_info_s {
	uint32_t duration;
	uint32_t block_count;
	uint32_t max_codec_bitrate;
	uint32_t min_codec_bitrate;
	uint32_t avg_codec_bitrate;
	uint32_t request_play_ts;
	uint32_t request_download_ts;
} duer_ds_audio_info_t;

extern duer_ds_audio_info_t g_audio_infos;

class MediaBase{
public:
	int open_power(int pin_name);
	int close_power(int pin_name);

	int bt_play();
	int bt_pause();
	int bt_forward();
	int bt_backward();
	int bt_volup();
	int bt_voldown();
	int bt_getA2dpstatus(int *status);
	
    int start_play(MediaType type, int bitrate);

    int write(const void* data, size_t size);

    int regulate_voice(unsigned char vol);

    int pause_play();

    int stop_play();

    bool is_play_stopped();

    int start_record(MediaType type);

    size_t read(void* data, size_t size);

    int stop_record();

	virtual int setBtMode();

	virtual int setUartMode();
	
	virtual bool is_bt_mode();
protected:
	virtual int on_open_power(int pin_name) = 0;
	virtual int on_close_power(int pin_name) = 0;

	virtual int on_bt_play() = 0;
	virtual int on_bt_pause() = 0;
	virtual int on_bt_forward() = 0;
	virtual int on_bt_backward() = 0;
	virtual int on_bt_volup() = 0;
	virtual int on_bt_voldown() = 0;
	virtual int on_bt_getA2dpstatus(int *status) = 0;

    virtual int on_start_play(MediaType type, int bitrate) = 0;

    virtual int on_write(const void* data, size_t size) = 0;

    virtual int on_voice(unsigned char vol) = 0;

    virtual int on_pause_play() = 0;

    virtual int on_stop_play() = 0;

    virtual int on_start_record(MediaType type) = 0;

    virtual size_t on_read(void* data, size_t size) = 0;

    virtual int on_stop_record() = 0;
	
	virtual int on_setBtMode() = 0;

	virtual int on_setUartMode() = 0;

    virtual bool is_play_stopped_internal() = 0;

private:
    static rtos::Mutex      _s_lock;
    static int              _s_status;
};

} // namespace duer

#endif // BAIDU_TINYDU_IOT_OS_SRC_MEDIA_BASE_BAIDU_MEDIA_BASE_H
