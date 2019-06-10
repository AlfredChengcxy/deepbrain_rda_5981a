// Copyright 2017 Baidu Inc. All Rights Reserved.
// Author: Chen Xihao (chenxihao@baidu.com)
//
// Description: define export type of media module

#ifndef BAIDU_TINYDU_IOT_OS_SRC_MEDIA_PLAYER_BAIDU_MEDIA_PLAY_TYPE_H
#define BAIDU_TINYDU_IOT_OS_SRC_MEDIA_PLAYER_BAIDU_MEDIA_PLAY_TYPE_H

namespace duer {

typedef void (*media_stutter_cb)(bool is_stuttuered, int flags);
typedef void (*mdm_media_play_failed_cb)(int flag);

enum MediaPlayerStatus {
    MEDIA_PLAYER_IDLE,
	MEDIA_PLAYER_RECORDING,		
	MEDIA_PLAYER_MAGIC_VOICE,	
	MEDIA_PLAYER_BT, 
    MEDIA_PLAYER_PLAYING,
    MEDIA_PLAYER_PAUSE
};

/*
 * Define flags for media play
 */
enum MediaFlag {
    MEDIA_FLAG_SPEECH               = 0x01, // this flag indicate the media type is speech
    MEDIA_FLAG_SAVE_PREVIOUS        = 0x02, // this flag indicate save the previous media info
    MEDIA_FLAG_DCS_URL              = 0x04, // this flag indicate the media url from dcs
    MEDIA_FLAG_UPNP_URL             = 0x08, // this flag indicate the media url from upnp
    MEDIA_FLAG_PROMPT_TONE          = 0x10, // this flag indicate the media type is prompt tone
    MEDIA_FLAG_SEEK                 = 0x20, // this flag indicate seek media file to play
    MEDIA_FLAG_WCHAT                = 0x40, // this flag indicate the media url from alert
    MEDIA_FLAG_LOCAL                = 0x80, // this flag indicate the media is from local
    MEDIA_FLAG_RECORD_TONE          = 0x0100, // this flag indicate the media is recorder tone
    MEDIA_FLAG_BT_TONE    			= 0x0200, // this flag indicate the media is interactive class
	MEDIA_FLAG_URL_CHAT				= 0x0400, // URL_CHAT

/// add for spi
	MEDIA_FLAG_SPI_DATA				= 0x0800,

/////add by lijun
	MEDIA_FLAG_BT_MODE = 0x100000,
	MEDIA_FLAG_WIFI_MODE=0x200000,
	MEDIA_FLAG_MAGIC_MODE=0x400000,
	MEDIA_FLAG_LOCAL_MODE=0x800000,
	

	
    //MEDIA_FLAG_WECHAT               = 0x0400, // this flag indicate the media is wechat
    // this flag indicate the media is the CA connected prompt tone
    MEDIA_FLAG_CONTINUE_PLAY        = 0x8000, // this flag indicate the play is not from start
    MEDIA_FLAG_NO_POWER_TONE    	= 0X1000, // this flag indicate the media is interactive class
    MEDIA_FLAG_MAGIC_VOICE			= 0x2000,    
	MEDIA_FLAG_RESTART				= 0x4000,	 //zt tony 20181104
};

const int FRAME_SIZE = 1024;
const int REC_FRAME_SIZE = 640/*64*/ ;

} // namespace duer

#endif // BAIDU_TINYDU_IOT_OS_SRC_MEDIA_PLAYER_BAIDU_MEDIA_PLAY_TYPE_H
