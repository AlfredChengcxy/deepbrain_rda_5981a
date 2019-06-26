#ifndef __DEEPBRAIN_H__
#define __DEEPBRAIN_H__
#include "ctypes_interface.h"
#include "dcl_interface.h"

namespace deepbrain {

typedef enum
{
	DEEPBRAIN_MODE_ASR,
	DEEPBRAIN_MODE_WECHAT,	
	DEEPBRAIN_MODE_MAGIC_VOICE,
	//add for kmt
	DEEPBRAIN_MODE_PLAY_LOCAL,
	DEEPBRAIN_MODE_BT,
	DEEPBRAIN_MODE_AIRKISS,
	DEEPBRAIN_MODE_MAX
}DEEPBRAIN_MODE_t;

void yt_dcl_process_result();

void yt_dcl_rec_on_data(char *data, int size);

void yt_dcl_rec_on_start();

void yt_dcl_rec_on_stop();

void yt_dcl_start();

void yt_dcl_stop();

void yt_dcl_init();

void yt_key_clear();

int auto_test();

}

#endif
