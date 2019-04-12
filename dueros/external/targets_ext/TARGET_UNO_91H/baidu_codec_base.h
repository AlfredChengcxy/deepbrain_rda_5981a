// Copyright 2017 Baidu Inc. All Rights Reserved.
// Author: zafter
//
// Description: Implement MediaBase for Codec

#ifndef BAIDU_EXTERNAL_TARGETS_EXT_TARGET_UNO_91H_Codec_BAIDU_RDA58XX_BASE_H
#define BAIDU_EXTERNAL_TARGETS_EXT_TARGET_UNO_91H_Codec_BAIDU_RDA58XX_BASE_H

#include "baidu_media_base.h"

namespace duer {

class BaiduCodecBase : public MediaBase {
public:
    BaiduCodecBase();

protected:
	virtual int on_open_power(int pin_name);
	virtual int on_close_power(int pin_name);

	virtual int on_bt_play();
	virtual int on_bt_pause();
	virtual int on_bt_forward();
	virtual int on_bt_backward();
	virtual int on_bt_volup();
	virtual int on_bt_voldown();
	virtual int on_bt_getA2dpstatus(int *status);

    virtual int on_start_play(MediaType type, int bitrate);

    virtual int on_write(const void* data, size_t size);

    virtual int on_voice(unsigned char vol);

    virtual int on_pause_play();

    virtual int on_stop_play();

    virtual int on_start_record(MediaType type);

    virtual size_t on_read(void* data, size_t size);

    virtual size_t on_read_done();

    virtual int on_stop_record();
	
	virtual int on_setBtMode();

	virtual int on_setUartMode();
	
    virtual bool is_play_stopped_internal();	
};

} // namespace duer

#endif // BAIDU_EXTERNAL_TARGETS_EXT_TARGET_UNO_91H_RDA58XX_BAIDU_RDA58XX_BASE_H
