#include "change_voice.h"
#include "st_mv_inf.h"
#include "amr_codec_cns.h"
#include "duer_recorder.h"
#include "baidu_media_play.h"
#include "memory_interface.h"
#include "lightduer_log.h"


#include "yt_mv_222_interface.h"


extern bool bExitMagicData ;


namespace duer{

#define YT_MV_FRAME_SHIFT 160


#define USE_LJ_MAGIC 1
#define USE_YT_MAGIC 0

static short pOutFrame[160];

#if USE_YT_MAGIC
static short pOutFrame_Magic[YT_MV_FRAME_SHIFT];
#elif USE_LJ_MAGIC 
static short pOutFrame_Magic[(YT_MV_FRAME_SHIFT * 4)];
#endif

static unsigned int nOffset = 0;


//for VoiceChanger
static void * pVoiceChanger = NULL;
static char *GLOBAL_pBufferForVoiceChanger = NULL;//[YT_MV_222_CHANGER_SIZE_IN_BYTE];


static void *pDecoderState = NULL;

static char *armnb_data = NULL;
static int armnb_data_len = 0;


void yt_voice_set_wave_head(char *wavHeader)
{
	const char channels = 1;
	const int totalDataLen = 0xffffffff;
	const int sampleRate = 8000;
	const char bitsPerSample = 16;
	const int bytePerSecond = sampleRate * (bitsPerSample / 8) * channels;  
	const int audioDataLen = 0xffffffff;

	//ckid：4字节 RIFF 标志，大写  
	wavHeader[0]  = 'R';  
	wavHeader[1]  = 'I';  
	wavHeader[2]  = 'F';  
	wavHeader[3]  = 'F';  

	//cksize：4字节文件长度，这个长度不包括"RIFF"标志(4字节)和文件长度本身所占字节(4字节),即该长度等于整个文件长度 - 8	
	wavHeader[4]  = (totalDataLen & 0xff);  
	wavHeader[5]  = ((totalDataLen >> 8) & 0xff);  
	wavHeader[6]  = ((totalDataLen >> 16) & 0xff);  
	wavHeader[7]  = ((totalDataLen >> 24) & 0xff);  

	//fcc type：4字节 "WAVE" 类型块标识, 大写  
	wavHeader[8]  = 'W';  
	wavHeader[9]  = 'A';  
	wavHeader[10] = 'V';  
	wavHeader[11] = 'E';  

	//ckid：4字节 表示"fmt" chunk的开始,此块中包括文件内部格式信息，小写, 最后一个字符是空格  
	wavHeader[12] = 'f';  
	wavHeader[13] = 'm';  
	wavHeader[14] = 't';  
	wavHeader[15] = ' ';  

	//cksize：4字节，文件内部格式信息数据的大小，过滤字节（一般为00000010H）	
	wavHeader[16] = 0x10;  
	wavHeader[17] = 0;	
	wavHeader[18] = 0;	
	wavHeader[19] = 0;	

	//FormatTag：2字节，音频数据的编码方式，1：表示是PCM 编码  
	wavHeader[20] = 1;	
	wavHeader[21] = 0;	

	//Channels：2字节，声道数，单声道为1，双声道为2  
	wavHeader[22] = channels;  
	wavHeader[23] = 0;	

	//SamplesPerSec：4字节，采样率，如44100  
	wavHeader[24] = (sampleRate & 0xff);	
	wavHeader[25] = ((sampleRate >> 8) & 0xff);  
	wavHeader[26] = ((sampleRate >> 16) & 0xff);	
	wavHeader[27] = ((sampleRate >> 24) & 0xff);	

	//BytesPerSec：4字节，音频数据传送速率, 单位是字节。其值为采样率×每次采样大小。播放软件利用此值可以估计缓冲区的大小；	
	//bytePerSecond = sampleRate * (bitsPerSample / 8) * channels  
	wavHeader[28] = (bytePerSecond & 0xff);  
	wavHeader[29] = ((bytePerSecond >> 8) & 0xff);  
	wavHeader[30] = ((bytePerSecond >> 16) & 0xff);  
	wavHeader[31] = ((bytePerSecond >> 24) & 0xff);  

	//BlockAlign：2字节，每次采样的大小 = 采样精度*声道数/8(单位是字节); 这也是字节对齐的最小单位, 譬如 16bit 立体声在这里的值是 4 字节。  
	//播放软件需要一次处理多个该值大小的字节数据，以便将其值用于缓冲区的调整  
	wavHeader[32] = (bitsPerSample * channels / 8);  
	wavHeader[33] = 0;	

	//BitsPerSample：2字节，每个声道的采样精度; 譬如 16bit 在这里的值就是16。如果有多个声道，则每个声道的采样精度大小都一样的；  
	wavHeader[34] = bitsPerSample;  
	wavHeader[35] = 0;	

	//ckid：4字节，数据标志符（data），表示 "data" chunk的开始。此块中包含音频数据，小写；	
	wavHeader[36] = 'd';  
	wavHeader[37] = 'a';  
	wavHeader[38] = 't';  
	wavHeader[39] = 'a';  

	//cksize：音频数据的长度，4字节，audioDataLen = totalDataLen - 36 = fileLenIncludeHeader - 44  
	wavHeader[40] = (audioDataLen & 0xff);  
	wavHeader[41] = ((audioDataLen >> 8) & 0xff);  
	wavHeader[42] = ((audioDataLen >> 16) & 0xff);  
	wavHeader[43] = ((audioDataLen >> 24) & 0xff);  
}



int yt_voice_change_init(char *data, int size)
{
	duer_recorder_amr_init_decoder(&pDecoderState);
	
	if(!pDecoderState)
	{
		DUER_LOGE("pDecoderState NULL");
		return -1;
	}



#if USE_LJ_MAGIC //TERENCE---2019/03/05
	if(!pVoiceChanger)pVoiceChanger = YT_MV_InitChanger();
		
	if(!pVoiceChanger)
	{		
		if(pDecoderState)
			YT_NB_AMR_FreeDecoder(&pDecoderState);
		
		DUER_LOGE("pVoiceChanger NULL");
		return -1;
	}

	YT_MV_SetConfig(pVoiceChanger, 1.0, 1.0, 8);
#elif USE_YT_MAGIC
	if(NULL == pVoiceChanger)
	{
		if(NULL == GLOBAL_pBufferForVoiceChanger)
		{
			GLOBAL_pBufferForVoiceChanger = (char*)memory_malloc(YT_MV_222_CHANGER_SIZE_IN_BYTE);
		}
		if(NULL != GLOBAL_pBufferForVoiceChanger)
		{

			pVoiceChanger = YT_MV_222_InitChanger(GLOBAL_pBufferForVoiceChanger, YT_MV_222_CHANGER_SIZE_IN_BYTE);
			if(!pVoiceChanger)
			{		
				if(pDecoderState)
					YT_NB_AMR_FreeDecoder(&pDecoderState);
				
				DUER_LOGE("pVoiceChanger NULL");
				return -1;
			}	
		}
	}
#endif
	
	nOffset = 0;
	int nReturn = YT_NB_AMR_GetDataInfo(data, size, &nOffset);

	if(nReturn != 0)nOffset = 0;
		

	armnb_data = data;
	armnb_data_len = size;
	
	return 0;
}

 

int yt_voice_change_get_data(char **data, int *size)
{
	int nReturn = 0;
	unsigned int nSampleNumber = 0;
	int nSampleNumber_Magic = 0;
	int i = 0, nOutSampleNumber, nTemp;

	nReturn = YT_NB_AMR_DecodeFrame(pDecoderState,armnb_data,armnb_data_len,&nOffset,						   
									pOutFrame,&nSampleNumber);


	if(bExitMagicData){nSampleNumber = 0;nReturn = 0;}
	
	if(nSampleNumber > 0) 
	{ 

	    /*
		for(i = 0; i < nSampleNumber; i++)
			pOutFrame[i] /= 2;
		*/


#if USE_LJ_MAGIC	
		*size = YT_MV_ChangeFrame(pVoiceChanger,pOutFrame,nSampleNumber,pOutFrame_Magic,YT_MV_FRAME_SHIFT * 4);
		*data = (char*)&pOutFrame_Magic[0];
		//*size = (*size)*2;
		//*data = (char*)&pOutFrame[0];
		*size = (*size)*2;	
#elif USE_YT_MAGIC
		//TERENCE---2019/03/05
		YT_MV_222_ChangeFrame(pVoiceChanger, pOutFrame, YT_MV_222_FRAME_SHIFT, 0, 0, pOutFrame_Magic, &nOutSampleNumber);
        ///*
		for(i = 0; i < nOutSampleNumber; i++)
		{
			nTemp = pOutFrame_Magic[i] ;
			nTemp *= 2;
			
			if(nTemp > 32767) nTemp = 32767;
			else if(nTemp < -32768) nTemp = -32768;

			pOutFrame_Magic[i] = (short) nTemp;
		}
		//*/

		*data = (char*)&pOutFrame_Magic[0];
		*size = nOutSampleNumber*2;	
#endif

	}
	
	if(0 == nReturn) 
		return 0;
	else
		return 1;
}

void yt_voice_change_free()
{
	DUER_LOGE("yt_voice_change_free");
#if USE_LJ_MAGIC
	YT_MV_FreeChanger(pVoiceChanger);
	pVoiceChanger = NULL;

	if(pDecoderState)
		YT_NB_AMR_FreeDecoder(&pDecoderState);
#elif USE_YT_MAGIC
	if(NULL != GLOBAL_pBufferForVoiceChanger)
	{
		memory_free(GLOBAL_pBufferForVoiceChanger);
		GLOBAL_pBufferForVoiceChanger = NULL;		
	}
	
		
	 YT_MV_222_FreeChanger(pVoiceChanger);
	
	pVoiceChanger = NULL;
	
	if(pDecoderState)
		YT_NB_AMR_FreeDecoder(&pDecoderState);
#endif
}
}
