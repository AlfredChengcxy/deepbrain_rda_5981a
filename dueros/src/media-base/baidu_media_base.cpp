// Copyright 2017 Baidu Inc. All Rights Reserved.
// Author: Chen Xihao (chenxihao@baidu.com)
//
// Description: Base class of Media

#include "baidu_media_base.h"

namespace duer {

enum {
    IDLE,
	BT,
    PLAYING,
    PAUSE,
    RECORDING,
    STOPPING,
};

rtos::Mutex MediaBase::_s_lock;
int MediaBase::_s_status = IDLE;

static mbed::Timer s_timer;
static uint32_t s_total_size = 0;
static uint32_t s_start_time = 0;
static uint32_t s_last_size = 0;
static uint32_t s_last_time = 0;


duer_ds_audio_info_t g_audio_infos;

static void on_write_end() {
    g_audio_infos.avg_codec_bitrate = s_total_size * 8 * 1000LL / (s_timer.read_ms() - s_start_time);
    s_timer.stop();

    if (g_audio_infos.max_codec_bitrate == 0) {
        g_audio_infos.max_codec_bitrate = g_audio_infos.avg_codec_bitrate;
        g_audio_infos.min_codec_bitrate = g_audio_infos.avg_codec_bitrate;
    }

    if (g_audio_infos.min_codec_bitrate > g_audio_infos.avg_codec_bitrate) {
        g_audio_infos.min_codec_bitrate = g_audio_infos.avg_codec_bitrate;
    }
}

int MediaBase::open_power(int pin_name) {
    int rs = 0;
    _s_lock.lock(osWaitForever);
    rs = on_open_power(pin_name);
    _s_lock.unlock();
    return rs;
}

int MediaBase::close_power(int pin_name) {
    int rs = 0;
    _s_lock.lock(osWaitForever);
    rs = on_close_power(pin_name);
    _s_lock.unlock();
    return rs;
}

int MediaBase::bt_play() {
    int rs = 0;
    _s_lock.lock(osWaitForever);
    rs = on_bt_play();
    _s_lock.unlock();
    return rs;
}

int MediaBase::bt_pause() {
    int rs = 0;
    _s_lock.lock(osWaitForever);
    rs = on_bt_pause();
    _s_lock.unlock();
    return rs;
}

int MediaBase::bt_backward() {
    int rs = 0;
    _s_lock.lock(osWaitForever);
    rs = on_bt_backward();
    _s_lock.unlock();
    return rs;
}

int MediaBase::bt_forward() {
    int rs = 0;
    _s_lock.lock(osWaitForever);
    rs = on_bt_forward();
    _s_lock.unlock();
    return rs;
}
int MediaBase::bt_volup() {
    int rs = 0;
    _s_lock.lock(osWaitForever);
    rs = on_bt_volup();
    _s_lock.unlock();
    return rs;
}
int MediaBase::bt_voldown() {
    int rs = 0;
    _s_lock.lock(osWaitForever);
    rs = on_bt_voldown();
    _s_lock.unlock();
    return rs;
}

int MediaBase::bt_getA2dpstatus(int *status){
    int rs = 0;
    _s_lock.lock(osWaitForever);
    rs = on_bt_getA2dpstatus(status);
    _s_lock.unlock();
    return rs;

}

int MediaBase::start_play(MediaType type, int bitrate) {
    int rs = 0;
    _s_lock.lock(osWaitForever);
    if (_s_status != PLAYING) {
        s_timer.reset();
        s_timer.start();
        s_total_size = 0;
        s_start_time = s_timer.read_ms();
        s_last_size = 0;
        s_last_time = s_start_time;

        _s_status = PLAYING;
        rs = on_start_play(type, bitrate);
    }
    _s_lock.unlock();
    return rs;
}

int MediaBase::write(const void* data, size_t size) {
    int rs = 0;
    _s_lock.lock(osWaitForever);
    if (_s_status == PLAYING || _s_status == PAUSE) {
        if (_s_status == PAUSE) {
            s_timer.start();
        }
        uint32_t ms = s_timer.read_ms() - s_last_time;
        s_total_size += size;

        if (ms >= 1000) { // calculate bitrate per second
            uint32_t bitrate = (s_total_size - s_last_size) * 8 * 1000LL / ms;
            if (bitrate > g_audio_infos.max_codec_bitrate) {
                g_audio_infos.max_codec_bitrate = bitrate;
            }

            if (bitrate < g_audio_infos.min_codec_bitrate
                    || g_audio_infos.min_codec_bitrate == 0) {
                g_audio_infos.min_codec_bitrate = bitrate;
            }

            s_last_size = s_total_size;
            s_last_time += ms;
        }

        rs = on_write(data, size);
        if (data == NULL || size == 0) {
            on_write_end();
            _s_status = STOPPING;
        } else {
            _s_status = PLAYING;
        }
    }
    _s_lock.unlock();
    return rs;
}

int MediaBase::regulate_voice(unsigned char vol) {
    int rs = 0;
    _s_lock.lock(osWaitForever);	
    if (_s_status == IDLE || _s_status == PLAYING || _s_status == PAUSE) {
    	rs = on_voice(vol);
    }
    _s_lock.unlock();
    return rs;
}

int MediaBase::pause_play() {
    int rs = 0;
    _s_lock.lock(osWaitForever);
    if (_s_status == PLAYING) {
        s_timer.stop();
        rs = on_pause_play();
        _s_status = PAUSE;
    }
    _s_lock.unlock();
    return rs;
}

int MediaBase::stop_play() {
    int rs = 0;
    _s_lock.lock(osWaitForever);
    if (_s_status == PLAYING || _s_status == PAUSE || _s_status == STOPPING) {
        if (_s_status != STOPPING) {
            on_write_end();
        }

        rs = on_stop_play();
        _s_status = IDLE;
    }
    _s_lock.unlock();
    return rs;
}

int MediaBase::start_record(MediaType type) {
    int rs = 0;
    _s_lock.lock(osWaitForever);
    if (_s_status != RECORDING) {
        _s_status = RECORDING;
        rs = on_start_record(type);
    }
    _s_lock.unlock();
    return rs;
}

size_t MediaBase::read(void* data, size_t size) {
    size_t rs = 0;
	//printf("====>read lock\r\n");
    _s_lock.lock(osWaitForever);
    if (_s_status == RECORDING) {
        rs = on_read(data, size);
    } else {
        rs = size + 1;
    }
	
    _s_lock.unlock();	
	//printf("===>unlock\r\n");
	
    return rs;
}

int MediaBase::stop_record() {
    int rs = 0;
    _s_lock.lock(osWaitForever);
    if (_s_status == RECORDING) {
        rs = on_stop_record();
        _s_status = IDLE;
    }
    _s_lock.unlock();
    return rs;
}

int MediaBase::setBtMode(void)
{	
    int rs = 0;
    _s_lock.lock(osWaitForever);
    if (_s_status == IDLE) {
        rs = on_setBtMode();
        _s_status = BT;
    }
    _s_lock.unlock();
    return rs;
}

int MediaBase::setUartMode(void)
{	
    int rs = 0;
    _s_lock.lock(osWaitForever);
    if(_s_status == BT) 
	{
		rs = on_setUartMode();		
		_s_status = IDLE;
    }
    _s_lock.unlock();
	return rs;
}

bool MediaBase::is_bt_mode()	{
	return (_s_status == BT)?true:false;
}

bool MediaBase::is_play_stopped() {
    bool rs = true;
    _s_lock.lock(osWaitForever);
    if (_s_status == PLAYING || _s_status == PAUSE) {
        rs = false;
    } else if (_s_status == STOPPING) {
        rs = is_play_stopped_internal();
        if (rs) {
            _s_status = IDLE;
        }
    } else {
        // do nothing
    }
    _s_lock.unlock();
    return rs;
}

} // namespace duer
