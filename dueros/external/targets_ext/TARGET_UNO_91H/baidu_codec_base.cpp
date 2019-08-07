
// Copyright 2017 Baidu Inc. All Rights Reserved.
// Author: Su Hao (suhao@baidu.com)
//
// Description: Implement MediaBase for Rda58xx

#include "baidu_codec_base.h"
#include "lightduer_log.h"
#include "rda58xx.h"
#include "baidu_media_play_type.h"

// #define TEST_MEDIA_FILE_INTEGRITY
#ifdef TEST_MEDIA_FILE_INTEGRITY
#include "baidu_media_file_storer.h"
#endif

#if defined(HI_FLYING) || defined(TARGET_UNO_81C_U04) || defined(TARGET_UNO_81C)
#define RESET_PIN   PC_9
#else
#define RESET_PIN   PC_6
#endif

rda58xx _rda58xx(PB_2, PB_1, PC_9);/// zhuxiaopi
//rda58xx _rda58xx(PD_3, PD_2, PC_9);///// 开发板 (不到朱)

namespace duer {

static const int MAX_VOLUME_VS1053 = 15;

#define CHECK_CODEC_ACK(ret) \
    do { \
        if (_rda58xx.atHandler(ret) < 0) { \
            return -1; \
        } \
    } while(0)

static int32_t atAckHandler(int32_t status) {
    int32_t ret = 0;
    if (VACK != (rda58xx_at_status) status) {
        switch ((rda58xx_at_status) status) {
        case NACK:
            DUER_LOGE("No ACK for AT command!\n");

            // reset codec and wait it ready
            _rda58xx.hardReset();
            DUER_LOGI("reset codec");
            while (!_rda58xx.isReady()) {
                Thread::wait(500);
            }
            DUER_LOGI("codec ready");
            break;
        case IACK:
            DUER_LOGE("Invalid ACK for AT command!\n");
            break;
        default:
            DUER_LOGE("Unknown ACK for AT command!\n");
            break;
        }
        ret = -1;
    }

    return ret;
}
BaiduCodecBase::BaiduCodecBase() {
    _rda58xx.setAtHandler(atAckHandler);
}

int BaiduCodecBase::on_open_power(int pin_name)
{
	rda58xx_at_status ret;
    while (!_rda58xx.isReady()) {
        Thread::wait(100);
    }

  	 ret = _rda58xx.setGPIOVal(pin_name, 1);
    CHECK_CODEC_ACK(ret);

    return 0;
}

int BaiduCodecBase::on_close_power(int pin_name)
{
	rda58xx_at_status ret;
    while (!_rda58xx.isReady()) {
        Thread::wait(100);
    }
	
  	 ret = _rda58xx.setGPIOVal(pin_name, 0);
    CHECK_CODEC_ACK(ret);
	
    return 0;
}


int BaiduCodecBase::on_SetName(const unsigned char* name)
{
    rda58xx_at_status ret;	
    ret = _rda58xx.SetName(name);
    _rda58xx.atHandler(ret);
    while (!_rda58xx.isReady()) {
        Thread::wait(100);
    }
	return 0;
}


int BaiduCodecBase::on_bt_getA2dpstatus(int *status)
{
	rda58xx_at_status ret;
	app_a2dp_state_t *a2dp_status = (app_a2dp_state_t*)status;
	if(_rda58xx.getMode() == BT_MODE){
		ret = _rda58xx.btGetA2dpStatus(a2dp_status);
	}
    CHECK_CODEC_ACK(ret);
	
    return 0;	
}

int BaiduCodecBase::on_bt_play()
{
	rda58xx_at_status ret;
	if(_rda58xx.getMode() == BT_MODE){
		ret = _rda58xx.btPlay();
	}
    CHECK_CODEC_ACK(ret);
	
    return 0;
}

int BaiduCodecBase::on_bt_pause()
{
	rda58xx_at_status ret;
	if(_rda58xx.getMode() == BT_MODE){
		ret = _rda58xx.btPause();
	}
    CHECK_CODEC_ACK(ret);
	
    return 0;
}

int BaiduCodecBase::on_bt_forward()
{
	rda58xx_at_status ret;
	if(_rda58xx.getMode() == BT_MODE){
		ret = _rda58xx.btForWard();
	}
    CHECK_CODEC_ACK(ret);
	
    return 0;
}

int BaiduCodecBase::on_bt_backward()
{
	rda58xx_at_status ret;
	if(_rda58xx.getMode() == BT_MODE){
		ret = _rda58xx.btBackWard();
	}
    CHECK_CODEC_ACK(ret);
	
    return 0;
}

int BaiduCodecBase::on_bt_volup()
{
	rda58xx_at_status ret;
	if(_rda58xx.getMode() == BT_MODE){
		ret = _rda58xx.btVolup();
	}
    CHECK_CODEC_ACK(ret);
	
    return 0;
}

int BaiduCodecBase::on_bt_voldown()
{
	rda58xx_at_status ret;
	if(_rda58xx.getMode() == BT_MODE){
		ret = _rda58xx.btVoldown();
	}
    CHECK_CODEC_ACK(ret);
	
    return 0;
}

int BaiduCodecBase::on_start_play(MediaType type, int bitrate) {
    DUER_LOGV("on_start_play begin type:%d bitrate:%d", type, bitrate);
#ifndef TEST_MEDIA_FILE_INTEGRITY
    while (!_rda58xx.isReady()) {
        Thread::wait(100);
    }

    static bool is_first_time = true;
    if (is_first_time) {
        char version[64];
        if (_rda58xx.getChipVersion(version) == VACK) {
            is_first_time = false;
        }
		_rda58xx.setGPIOMode(4); 
		_rda58xx.setGPIODir(4);
    }

    rda58xx_at_status ret = _rda58xx.stopRecord();
    CHECK_CODEC_ACK(ret);
    _rda58xx.clearBufferStatus();
    ret = _rda58xx.stopPlay(WITHOUT_ENDING);
    CHECK_CODEC_ACK(ret);

    switch (type) {
    case TYPE_MP3: {
        int buffer_size = 0;
        int start_size = 0;
        switch (bitrate) {
        case 8000:
        case 16000:
            buffer_size = 1024 * 3;
            start_size = 1024;
            break;
        case 32000:
            buffer_size = 1024 * 4;
            start_size = 1024 * 2;
            break;
        case 64000:
            buffer_size = 1024 * 6;
            start_size = 1024 * 2;
            break;
        default:
            buffer_size = 1024 * 8;
            start_size = 1024 * 2;
            break;
        }
        ret = _rda58xx.bufferReq(MCI_TYPE_DAF, buffer_size, start_size);
        break;
    }
    case TYPE_M4A:
    case TYPE_AAC:
        ret = _rda58xx.bufferReq(MCI_TYPE_AAC, 1024 * 8, 1024 * 5);
        break;
    case TYPE_WAV:
        ret = _rda58xx.bufferReq(MCI_TYPE_WAV, 1024 * 8, 1024 * 5);
        break;
    case TYPE_TS:
        ret = _rda58xx.bufferReq(MCI_TYPE_TS, 1024 * 8, 1024 * 5);
        break;
    case TYPE_AMR:
        // one frame of amr is 32 bytes
        ret = _rda58xx.bufferReq(MCI_TYPE_AMR, 1024 * 2, 320);
        break;
    default:
        DUER_LOGE("Invalid type: %d", type);
        return -1;
    }
    CHECK_CODEC_ACK(ret);
#else
    duer::MediaFileStorer::instance().open(type);
#endif
    DUER_LOGV("on_start_play end");
    return 0;
}


int BaiduCodecBase::on_write(const void* data, size_t size) {
    DUER_LOGV("on_write begin size:%d",size);
#ifndef TEST_MEDIA_FILE_INTEGRITY
    const unsigned char* value = (const unsigned char*)data;
    rda58xx_at_status ret;
    if (data == NULL || size == 0) {
        ret = _rda58xx.stopPlay(WITH_ENDING);
        CHECK_CODEC_ACK(ret);
        return 0;
    }

    int data_size = size;
    int n = 0;
    //DUER_LOGV("REQ:%d.\r\n", _rda58xx.requestData() );

    int time_start = us_ticker_read();
    while (data_size > 0) {
        n = (data_size < 1024) ? data_size : 1024;
        ret = _rda58xx.sendRawData(const_cast<unsigned char*>(value), n);
        CHECK_CODEC_ACK(ret);
        data_size -= n;
        value += n;
    }
    //debug
    //write_data_sd(data, size);

    int time_end = us_ticker_read();
    DUER_LOGV("T:%d.\r\n", time_end - time_start);

#else
    duer::MediaFileStorer::instance().write(data, size);
#endif

    DUER_LOGV("on_write end");
    return 0;
}

int BaiduCodecBase::on_voice(unsigned char vol) 
{
    DUER_LOGV("on_voice vol=%d.", vol);
    while (!_rda58xx.isReady()) {
        Thread::wait(100);
    }
    rda58xx_at_status ret = _rda58xx.volSet(vol * MAX_VOLUME_VS1053 / 100);
    CHECK_CODEC_ACK(ret);
    return 0;
}

int BaiduCodecBase::on_pause_play() {
#ifndef TEST_MEDIA_FILE_INTEGRITY
    DUER_LOGV("on_pause_play begin");
    rda58xx_at_status ret = _rda58xx.pause();
    CHECK_CODEC_ACK(ret);
    DUER_LOGV("on_pause_play end");
#endif
    return 0;
}

int BaiduCodecBase::on_stop_play() {
#ifndef TEST_MEDIA_FILE_INTEGRITY
    DUER_LOGV("on_stop_play begin");
    rda58xx_at_status ret = _rda58xx.stopPlay(WITHOUT_ENDING);
    CHECK_CODEC_ACK(ret);
    DUER_LOGV("on_stop_play end");
#else
    duer::MediaFileStorer::instance().close();
#endif
    return 0;
}

int BaiduCodecBase::on_start_record(MediaType type) 
{
    DUER_LOGV("on_start_record begin");

    rda58xx_at_status ret = _rda58xx.stopPlay(WITHOUT_ENDING);
    CHECK_CODEC_ACK(ret);

#if defined(TARGET_UNO_81C_U04) || defined(TARGET_UNO_81C)
    static bool is_first_time = true;
    if (is_first_time) {
        _rda58xx.setMicGain(11);
        is_first_time = false;
    }
#endif

	switch(type)
	{
		case TYPE_AMR:
			ret = _rda58xx.startRecord(MCI_TYPE_AMR, _rda58xx.getBufferSize() * 2 / 4, 8000);
			break;

		case TYPE_WAV:
    		ret = _rda58xx.startRecord(MCI_TYPE_WAV_DVI_ADPCM, _rda58xx.getBufferSize() * 2 / 4, 16000);
			break;
	}
    CHECK_CODEC_ACK(ret);
    DUER_LOGV("on_start_record b");
    _rda58xx.setStatus(RECORDING);

    DUER_LOGV("on_start_record end");

    return 0;
}

size_t BaiduCodecBase::on_read(void* data, size_t size) 
{
    unsigned int len = 0;

    DUER_LOGV("on_read");

    if (FULL == _rda58xx.getBufferStatus()) {
        _rda58xx.clearBufferStatus();
        len = _rda58xx.getBufferSize();
    }

    if (len > size) {
        len = size;
    }

    if (len > 0) {
        memcpy(data, _rda58xx.getBufferAddr(), len);
    }

    DUER_LOGV("on_read: len = %d, size = %d", len, size);

    return len;
}

int BaiduCodecBase::on_stop_record() {
    DUER_LOGV("on_stop_record begin");
    rda58xx_at_status ret = _rda58xx.stopRecord();
    CHECK_CODEC_ACK(ret);
    _rda58xx.clearBufferStatus();
    DUER_LOGV("on_stop_record end");
    return 0;
}

size_t BaiduCodecBase::on_read_done() 
{
	return 0;	
}

int BaiduCodecBase::on_setBtMode(void)
{
    rda58xx_at_status ret;	
    ret = _rda58xx.setMode(BT_MODE);
	CHECK_CODEC_ACK(ret);
	
	while(_rda58xx.getMode() == UART_MODE){
        Thread::wait(100);
	}
	
	CHECK_CODEC_ACK(ret);
	return 0;
}

int BaiduCodecBase::on_setUartMode(void)
{
    rda58xx_at_status ret;	
    ret = _rda58xx.setMode(UART_MODE);
    CHECK_CODEC_ACK(ret);

	while(_rda58xx.getMode() == BT_MODE){
        Thread::wait(100);
	}

	Thread::wait(1000);
	return 0;
}

bool BaiduCodecBase::is_play_stopped_internal() {
    rda58xx_status status;
    _rda58xx.getCodecStatus(&status);
    return status <= STOP;
}

} // namespace duer
