#include "duer_recorder.h"
#include "device_vad.h"
#include "baidu_media_play_type.h"
#include "YTLight.h"
#include "audio.h"
#include "YTManage.h"
#include "lightduer_log.h"
#include "lightduer_memory.h"
#include "deepbrain_app.h"
#include "ringbuff.h"
#include "amr_codec_cns.h"
#include "yt_vad_rda_interface.h"

#include "task_thread_interface.h"


namespace deepbrain {
extern bool is_magic_voice_mode();
extern bool in_wifi_mode();
extern bool in_wechat_mode();

}



namespace duer {

#define USE_DYNAMIC_THREAD 0

#if !USE_DYNAMIC_THREAD
static rtos::Thread duer_rec_thread(osPriorityAboveNormal, DEFAULT_STACK_SIZE);
static rtos::Thread duer_amr_thread(osPriorityNormal, DEFAULT_STACK_SIZE * 1);
#else
static rtos::Thread *duer_rec_thread = NULL;
static rtos::Thread *duer_amr_thread = NULL;
#endif

enum _duer_record_state {
    DUER_REC_STOPPING,
    DUER_REC_STOPPED,
    DUER_REC_STARTING,
    DUER_REC_STARTED,
};


static Ringbuff rb(REC_FRAME_SIZE, 4);
static _duer_record_state _state = DUER_REC_STOPPED;

//amr_decoder
#ifndef DISABLE_LOCAL_VAD
static void *pDecoderState = NULL;
static void *pInstance = NULL;



#define USE_DYNAMIC_VAD 0
#define USE_DYNAMIC_AMR 1

#if !USE_DYNAMIC_AMR
static char pBufferForDecoder[YT_NB_AMR_DECODER_SIZE_IN_BYTE];
#else
static char *pBufferForDecoder=NULL;
#endif

#if !USE_DYNAMIC_VAD
static char pBufferForVAD[YT_VAD_8000_MEMORY_SIZE];
#else
static char *pBufferForVAD=NULL;
#endif

static short pOutFrame[160];
static bool vad_is_start = false;
static bool vad_is_end = false;
static bool vad_mode = false;
static bool vad_asr_mode = false;
int nSampleTotal = 0;
#endif


const char amr_head[] =  "#!AMR\n";

bool duer_recorder_is_busy()
{
#if !USE_DYNAMIC_THREAD
	return (duer_amr_thread.get_state() == Thread::Deleted) ? false : true;
#else
	if(_state != DUER_REC_STOPPED) return true;
	if(NULL == duer_amr_thread || duer_amr_thread->get_state() == Thread::Deleted) return false;
	else return true;
#endif
}

bool duer_recorder_is_start()
{
#if !USE_DYNAMIC_THREAD
	return (duer_amr_thread.get_state() == Thread::Running) ? false : true;
#else
	if(NULL == duer_amr_thread || duer_amr_thread->get_state() == Thread::Running) return false;
	else return true;
#endif
}

void duer_recorder_amr_init_decoder(void **handler)
{
#if !USE_DYNAMIC_AMR
	if(*handler !=NULL) 
	{
		DUER_LOGE("*handler != NULL");
		return ;
	}
	*handler = YT_NB_AMR_InitDecoder(pBufferForDecoder,YT_NB_AMR_DECODER_SIZE_IN_BYTE);
#else
	if(pBufferForDecoder == NULL)
	{
		pBufferForDecoder =(char *) malloc(YT_NB_AMR_DECODER_SIZE_IN_BYTE);
	}
	if(pBufferForDecoder)
	{
		*handler = YT_NB_AMR_InitDecoder(pBufferForDecoder,YT_NB_AMR_DECODER_SIZE_IN_BYTE);
	}
#endif
}


void duer_recorder_reinit()
{
#if USE_DYNAMIC_VAD
	if(pBufferForVAD != NULL)
	{
		DUER_LOGE("duer_recorder_reinit:pBufferForVAD");
		free(pBufferForVAD);
		pBufferForVAD = NULL;
	}
#endif

#if USE_DYNAMIC_AMR
	if(pBufferForDecoder != NULL)
	{
		DUER_LOGE("duer_recorder_reinit:pBufferForDecoder");
		free(pBufferForDecoder);
		pBufferForDecoder = NULL;
	}
#endif

#if USE_DYNAMIC_THREAD
	DUER_LOGE("duer_recorder_reinit:duer_amr_thread&&duer_rec_thread");
	if(duer_amr_thread)
	{
		task_thread_exit(duer_amr_thread);	
		duer_amr_thread = NULL;
	}
	if(duer_rec_thread)
	{
		task_thread_exit(duer_rec_thread);
		duer_rec_thread = NULL;
	}
#endif
}

static void duer_recorder_on_start()
{	
#ifndef DISABLE_LOCAL_VAD
		if(vad_mode)
		{				
			duer_recorder_amr_init_decoder(&pDecoderState);
			if(pDecoderState == NULL)
			{
				duer_recorder_stop();
				return;
			}
		
			DUER_LOGI("vad mode");	
		#if USE_DYNAMIC_VAD
			DUER_LOGI("pBufferForVAD : [%x]",pBufferForVAD);
			if(pBufferForVAD == NULL)
			{
				pBufferForVAD =(char *) malloc(YT_VAD_8000_MEMORY_SIZE);
			}
			if(pBufferForVAD)
			{
				DUER_LOGI("pBufferForVAD malloc success");
				yt_vad_rda_8000_set_memory_buffer(pBufferForVAD,YT_VAD_8000_MEMORY_SIZE);
			}
		#else		
			yt_vad_rda_8000_set_memory_buffer(pBufferForVAD,YT_VAD_8000_MEMORY_SIZE);
		#endif
			pInstance = yt_vad_rda_8000_get_instance();
			yt_vad_rda_8000_initialize(pInstance, 8000);
			yt_vad_rda_8000_start_session(pInstance);			
			yt_vad_rda_8000_reset_for_new_start(pInstance);
			vad_is_start = false;
			vad_is_end = false;
		}
#endif	

	deepbrain::yt_dcl_rec_on_start();
}

static void duer_recorder_on_data(char *data, int size)
{
	DUER_LOGV("duer_recorder_on_data size:%d", size);	
#ifndef DISABLE_LOCAL_VAD
	short pOutSpeechFrame[200] = {0};
	unsigned int nOffset,nSampleNumber;
	
	unsigned int nHistoryMaxSum,nCurMinSum;
	int pOutSampleNumber = 0;
	char charFlag_VAD = 0;
	
	if(vad_mode)
	{
		int nReturn = YT_NB_AMR_GetDataInfo(data, size, &nOffset);
		if(nOffset != 0)
			nOffset = 0;
		
		//nSampleTotal=0;//cjl add 20190307
		
		while(1)
		{
			nReturn = YT_NB_AMR_DecodeFrame(pDecoderState,data,size,&nOffset,						   
											pOutFrame,&nSampleNumber);					
			if(nSampleNumber > 0 && !vad_is_end) {
				if(!vad_is_start) {

                 if(!vad_asr_mode)
                 {
				 	//yt_vad_rda_8000_set_mv_mode(pInstance);
					//here insert the new vad fun:magic mode///////////////////////
					yt_vad_rda_8000_input_data_to_start(pInstance, 
													pOutFrame,
													nSampleNumber,
													pOutSpeechFrame,
													&pOutSampleNumber,
													&charFlag_VAD);

                 }
				 else
				 {
                    //here insert the new vad fun:asr mode///////////////////////
					//yt_vad_rda_8000_set_asr_mode(pInstance);
					yt_vad_rda_8000_input_data_to_start(pInstance, 
													pOutFrame,
													nSampleNumber,
													pOutSpeechFrame,
													&pOutSampleNumber,
													&charFlag_VAD);
				 }

					if(charFlag_VAD == 'S') {						
						DUER_LOGI("start vad\n");						
						vad_is_start = true;
						
					    nSampleTotal = 0;//cjl add 20190307
					}
				}	
				
				if(vad_is_start)
				{
				   if(!vad_asr_mode)
				   {
					  //nSampleTotal++;//cjl add 20190307
					  
					  //yt_vad_rda_8000_set_mv_mode(pInstance);
					  //here insert the new vad fun:magic mode///////////////////////
					  yt_vad_rda_8000_input_data_to_end(pInstance, 
												   pOutFrame,
												   nSampleNumber,
												   pOutSpeechFrame,
												   &pOutSampleNumber,
												   &nHistoryMaxSum,
												   (unsigned char*)&charFlag_VAD,
												   &nCurMinSum,
												   pOutSpeechFrame);

					  nSampleTotal += nSampleNumber;

				
				  }
				  else
				  {
					 //here insert the new vad fun:asr mode///////////////////////
					 //yt_vad_rda_8000_set_asr_mode(pInstance);
					 yt_vad_rda_8000_input_data_to_end(pInstance, 
												   pOutFrame,
												   nSampleNumber,
												   pOutSpeechFrame,
												   &pOutSampleNumber,
												   &nHistoryMaxSum,
												   (unsigned char*)&charFlag_VAD,
												   &nCurMinSum,
												   pOutSpeechFrame);	

				  nSampleTotal += nSampleNumber;
					 
				  }
				
					if(charFlag_VAD == 'Y')
					{	
						vad_is_end = true;

						
						DUER_LOGI("vad end\n");						
						duer_recorder_stop();
						nSampleTotal = 0;
						
					}
					else
					{
					 	if(nSampleTotal > (8000*7))	
							{
								vad_is_end = true; //TERENCE---2019/03/11: mandatory voice end	
								duer_recorder_stop();
								nSampleTotal = 0;
							
					 		}
							
					}

				}
			}
			if(0 == nReturn) break;
		}

		if(vad_is_start)
			deepbrain::yt_dcl_rec_on_data(data, size);
	}
	else {
		deepbrain::yt_dcl_rec_on_data(data, size);
	}
#else
	deepbrain::yt_dcl_rec_on_data(data, size);
#endif
}

static void duer_recorder_on_stop()
{	
	DUER_LOGI("duer_recorder_on_stop");
	
#ifndef DISABLE_LOCAL_VAD	
	if(vad_mode)
	{		
		YT_NB_AMR_FreeDecoder(&pDecoderState);
		yt_vad_rda_8000_close_session(pInstance);
	}

	#if USE_DYNAMIC_VAD
		if(pBufferForVAD != NULL)
		{
			DUER_LOGI("pBufferForVAD Free");
			free(pBufferForVAD);
			pBufferForVAD = NULL;
		}
	#endif 
	#if USE_DYNAMIC_AMR
		if(pBufferForDecoder != NULL)
		{
			DUER_LOGI("pBufferForDecoder Free");
			free(pBufferForDecoder);
			pBufferForDecoder = NULL;
		}
	#endif
#endif
	
	deepbrain::yt_dcl_rec_on_stop();
}

static void duer_amr_thread_main()
{	
	unsigned int nOffset,nSampleNumber;
	char *data = NULL;
	unsigned int size = 0;
	DUER_LOGI("duer_amr_thread_main start");
	duer_recorder_on_start();
	
#if !USE_DYNAMIC_THREAD
	duer_amr_thread.signal_wait(0x01);
#else
	duer_amr_thread->signal_wait(0x01);
#endif
	DUER_LOGI("duer_amr_thread_main start1");

	while(_state == DUER_REC_STARTED)
	{
		rb.get_readPtr(&data);
		if(data) {
			duer_recorder_on_data(data, REC_FRAME_SIZE);
			rb.read_done();
		}
	}	

	while(rb.get_remain() > 0)
	{
		rb.get_readPtr(&data);
		if(data) {
			duer_recorder_on_data(data, REC_FRAME_SIZE);
			rb.read_done();
		}
	}
#if !USE_DYNAMIC_THREAD
	while(duer_rec_thread.get_state() != Thread::Deleted);
#else
	while(duer_rec_thread->get_state() != Thread::Deleted);
#endif
	duer_recorder_on_stop();
}

static void duer_rec_thread_main() 
{
	char* data = NULL;
    char* p_cur = NULL;
    int rs = 0;
	
	DUER_LOGI("duer_rec_thread_main start");
	
	_state = DUER_REC_STARTED;
	rb.init();
	rb.get_writePtr(&data);
    p_cur = data;
#if !USE_DYNAMIC_THREAD
	duer_amr_thread.signal_set(0x01);
#else
	duer_amr_thread->signal_set(0x01);
#endif

	DUER_LOGI("duer_rec_thread_main start1");

	//memcpy(p_cur, amr_head, strlen(amr_head));
	//p_cur += strlen(amr_head);
	
    while (_state == DUER_REC_STARTED) {
        if (p_cur == data + REC_FRAME_SIZE) {
			rb.write_done();			
			rb.get_writePtr(&data);
            //duer_recorder_on_data(data, p_cur - data);
            p_cur = data;
        }
        size_t size = data + REC_FRAME_SIZE - p_cur;
        rs = YTMediaManager::instance().rec_read_data(p_cur, size);
        // if value of rs is bigger than size, _plugin is not RECORDING
        if (rs > size) {
            break;
        }
        p_cur += rs;
    }
exit:
	DUER_LOGI("duer_rec_thread_main stop0");
	YTMediaManager::instance().rec_stop();
	_state = DUER_REC_STOPPED;
	DUER_LOGI("duer_rec_thread_main stop1");
}

void duer_recorder_set_vad(bool need_vad)
{
	vad_mode = need_vad;
}

void duer_recorder_set_vad_asr(bool asr_mode)
{
	vad_asr_mode = asr_mode;
}

void duer_recorder_stop()
{
    if (_state > DUER_REC_STOPPED) {
        _state = DUER_REC_STOPPING;
    }		
	DUER_LOGI("duer_recorder_stop");
}





void duer_recorder_start()
{		
////add
	if(!deepbrain::is_magic_voice_mode() && !deepbrain::in_wifi_mode() && !deepbrain::in_wechat_mode())return;

	DUER_LOGI("duer_recorder_start");
	_state = DUER_REC_STARTING;	
	
	YTMediaManager::instance().rec_start();

#if USE_DYNAMIC_THREAD
	if(NULL == duer_amr_thread)
	{
		duer_amr_thread =(rtos::Thread *) task_thread_create(duer_amr_thread_main,
							"",
							DEFAULT_STACK_SIZE,
							NULL,
							osPriorityNormal);
		DUER_LOGI("duer_amr_thread:[%x]",duer_amr_thread);
	}
	else
	{
		DUER_LOGI("duer_amr_thread:[%x]",duer_amr_thread);
		duer_amr_thread->start(duer_amr_thread_main);
	}
	if(NULL == duer_rec_thread)
	{
		duer_rec_thread =(rtos::Thread *) task_thread_create(duer_rec_thread_main,
						"",
						DEFAULT_STACK_SIZE,
						NULL,
						osPriorityAboveNormal);
		DUER_LOGI("duer_rec_thread:[%x]",duer_rec_thread);
	}
	else
	{
		DUER_LOGI("duer_rec_thread:[%x]",duer_rec_thread);
		duer_rec_thread->start(duer_rec_thread_main);
	}
#else	
	duer_amr_thread.start(&duer_amr_thread_main);
	duer_rec_thread.start(&duer_rec_thread_main);
#endif
}

}
