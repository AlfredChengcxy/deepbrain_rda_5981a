/*
 * Copyright 2017-2018 deepbrain.ai, Inc. or its affiliates. All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License").
 * You may not use this file except in compliance with the License.
 * A copy of the License is located at
 *
 *     http://deepbrain.ai/apache2.0/
 *
 * or in the "license" file accompanying this file. This file is distributed
 * on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either
 * express or implied. See the License for the specific language governing
 * permissions and limitations under the License.
 */

#ifndef DCL_SERVICE_H
#define DCL_SERVICE_H

#include "dcl_interface.h"
#include "dcl_asr_api.h"

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************************
//语义处理接口
*******************************************************************/
#define NLP_SERVICE_LINK_MAX_NUM 5
	
/*语义结果类型*/
typedef enum NLP_RESULT_TYPE_T
{
	NLP_RESULT_TYPE_NONE,
	NLP_RESULT_TYPE_NO_ASR,
	NLP_RESULT_TYPE_NO_LICENSE,
	NLP_RESULT_TYPE_SHORT_AUDIO,
	NLP_RESULT_TYPE_CHAT,
	NLP_RESULT_TYPE_LINK,
	NLP_RESULT_TYPE_CMD,
	NLP_RESULT_TYPE_VOL_CMD,
	NLP_RESULT_TYPE_TRANSLATE,
	NLP_RESULT_TYPE_ERROR,
}NLP_RESULT_TYPE_T;

/*语义结果-聊天结果*/
typedef struct NLP_RESULT_CHAT_T
{
#if 1 //// ��ʱ�޸�
	char text[1024];
	char link[1024];
#else
	char text[512];
	char link[512];
#endif	
}NLP_RESULT_CHAT_T;

/*语义结果-音乐资源*/
typedef struct NLP_RESULT_LINK_T
{
	char link_name[128];
	char name_tts_url[1024];
	char link_url[128];
}NLP_RESULT_LINK_T;

/*语义结果-音乐资源集合*/
typedef struct NLP_RESULT_LINKS_T
{
	int link_size;
	NLP_RESULT_LINK_T link[NLP_SERVICE_LINK_MAX_NUM];
}NLP_RESULT_LINKS_T;

/*语义结果集合*/
typedef struct NLP_RESULT_T
{
	int request_sn;		//请求序列号
	char input_text[256];
	NLP_RESULT_TYPE_T type;
	NLP_RESULT_CHAT_T chat_result;
	NLP_RESULT_LINKS_T link_result;
}NLP_RESULT_T;


/******************************************************************
//语音识别服务接口
*******************************************************************/
/*语音识别结果*/
typedef struct ASR_RESULT_t
{
	int 				record_sn;		//语音识别序列号
	DCL_ERROR_CODE_t	error_code;		//错误码
	char				str_result[MAX_NLP_RESULT_LENGTH];//语义返回的原始字符串
}ASR_RESULT_t;

/*语音输入类型*/
typedef enum ASR_MSG_TYPE_T
{
	ASR_SERVICE_RECORD_START = 0,
	ASR_SERVICE_RECORD_READ,
	ASR_SERVICE_RECORD_STOP,
}ASR_MSG_TYPE_T;

/*语音识别引擎选择*/
typedef enum ASR_ENGINE_TYPE_t
{
	//deepbrain private asr engine
	ASR_ENGINE_TYPE_DP_ENGINE,
}ASR_ENGINE_TYPE_t;

//语音识别结果回调
typedef void (*asr_result_cb)(ASR_RESULT_t *result);

/*语音识别输入PCM数据对象*/
typedef struct ASR_PCM_OBJECT_t
{
	//tail queue entry
	TAILQ_ENTRY(ASR_PCM_OBJECT_t) next;
	
	ASR_MSG_TYPE_T 		msg_type;		//消息类型
	asr_result_cb 		asr_call_back;	//可以设置为NULL
	
	int 				record_sn;		//录音序列号
	int 				record_id;		//录音索引
	int 				record_len;		//录音数据长度
	int 				record_ms;		//录音时长
	int 				is_max_ms;		//录音是否是最大的录音时间
	uint64_t 			time_stamp;		//录音时间戳
	DCL_ASR_MODE_t 		asr_mode;		//语音识别模式
	DCL_ASR_LANG_t 		asr_lang;		//识别语言类型
	ASR_ENGINE_TYPE_t 	asr_engine;		//语音识别引擎类型
	//char record_data[RAW_PCM_LEN_MS(100, PCM_SAMPLING_RATE_16K)];//200毫秒pcm数据
	char record_data[RAW_AMR_LEN_MS];//200毫秒amr数据
}ASR_PCM_OBJECT_t;

#ifdef __cplusplus
}
#endif

#endif //DCL_SERVICE_H


