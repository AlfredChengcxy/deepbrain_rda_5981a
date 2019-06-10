#ifndef __YTV_ASR_TINY_GEM_INTERFACE_HEADER__
#define __YTV_ASR_TINY_GEM_INTERFACE_HEADER__


typedef void (*YTV_ASR_CB_LOCK_MUTEX)(void);
typedef void (*YTV_ASR_CB_UNLOCK_MUTEX)(void);

typedef void*  YTV_ASR_GEM_CONTAINER_POINTER;
typedef void*  YTV_ASR_GEM_ENGINE_POINTER;

#define YTV_ASR_ERROR_PCM_NOT_READY -1
#define YTV_ASR_ERROR_FRAME_PARA_NOT_READY -2

#define YTV_ASR_DATA_NORMAL 1
#define YTV_ASR_DATA_COMPLETE 0




#ifdef __cplusplus
	extern "C" {
#endif



	//TERENCE---2017/03/25
	//////////////////////////////////////////////////////////////////////////////////////////////////
	int ytv_asr_tiny_set_callback_for_write_uart(void (*pSendFunction_CB_UART)(char *pDataToSend, int nByteNumberToSend));
	int ytv_asr_tiny_set_callback_for_read_uart(void (*pReceiveFunction_CB_UART)(char *pDataReceived, int *pReceivedByteNumber));
	int ytv_asr_tiny_set_callback_for_delay_uart(void (*pDelayFunction_CB_UART)(int nDelay_MS));
	int ytv_asr_tiny_set_callback_for_read_write_uart(void (*pReadWriteFunction_CB_UART)(char *pDataToSend, int nByteNumberToSend, char *pBufferToReceive, int nByteNumberToReceive));

	//////////////////////////////////////////////////////////////////////////////////////////////////


	YTV_ASR_GEM_CONTAINER_POINTER ytv_asr_tiny_create_container(void *pReserved, char *strReservedOne,unsigned int nLangID, char *strReservedTwo);
	int ytv_asr_tiny_create_engine(YTV_ASR_GEM_CONTAINER_POINTER pContainer,char *pMemoryBufferForEngine,int nMemSizeInByte,YTV_ASR_GEM_ENGINE_POINTER* ppEngineInstance);

	int ytv_asr_tiny_put_pcm_data(YTV_ASR_GEM_ENGINE_POINTER pEngineInstance,short *pInputSample, int nSampleNumber, char *pFlag_VAD);

	int ytv_asr_tiny_run_one_step(YTV_ASR_GEM_ENGINE_POINTER pEngineInstance);

	int ytv_asr_tiny_notify_data_end(YTV_ASR_GEM_ENGINE_POINTER pEngineInstance);

	int ytv_asr_tiny_get_result(YTV_ASR_GEM_ENGINE_POINTER pEngineInstance,int N_MAX, int *pResultNumber, int *pOriginalWordIndex);

	int ytv_asr_tiny_reset_engine(YTV_ASR_GEM_ENGINE_POINTER pEngineInstance);

	int ytv_asr_tiny_free_engine(YTV_ASR_GEM_ENGINE_POINTER pEngineInstance);
	int ytv_asr_tiny_free_container(YTV_ASR_GEM_CONTAINER_POINTER pContainer);

	int ytv_asr_tiny_set_callback_lock_mutex(YTV_ASR_GEM_ENGINE_POINTER pEngineInstance,YTV_ASR_CB_LOCK_MUTEX pFuncPointer_LockMutex);
	int ytv_asr_tiny_set_callback_unlock_mutex(YTV_ASR_GEM_ENGINE_POINTER pEngineInstance,YTV_ASR_CB_UNLOCK_MUTEX pFuncPointer_UnlockMutex);

	int ytv_asr_tiny_get_reject_value(YTV_ASR_GEM_ENGINE_POINTER pEngineInstance);
	int ytv_asr_tiny_set_reject_value(YTV_ASR_GEM_ENGINE_POINTER pEngineInstance,int nNewValue);

	int ytv_asr_tiny_get_minimum_duration(YTV_ASR_GEM_ENGINE_POINTER pEngineInstance);
	int ytv_asr_tiny_set_minimum_duration(YTV_ASR_GEM_ENGINE_POINTER pEngineInstance,int nNewValue);



	int ytv_asr_tiny_set_vad_ratio(YTV_ASR_GEM_ENGINE_POINTER pEngineInstance, int nRatio);
	void ytv_asr_tiny_set_spectrum_flag(YTV_ASR_GEM_ENGINE_POINTER pEngineInstance, char cUseSpecSharpenFlag);
	int ytv_asr_tiny_set_min_average_for_start(YTV_ASR_GEM_ENGINE_POINTER pEngineInstance,int nMinAverage);
	int ytv_asr_tiny_set_maxmin_ratio_for_end(YTV_ASR_GEM_ENGINE_POINTER pEngineInstance,int nRatio_10);


	void ytv_asr_tiny_cancel_session(YTV_ASR_GEM_ENGINE_POINTER pEngineInstance);

	int ytv_asr_tiny_get_command_number(YTV_ASR_GEM_ENGINE_POINTER pEngineInstance);
	char *ytv_asr_tiny_get_one_command_gbk(YTV_ASR_GEM_ENGINE_POINTER pEngineInstance, int nCommandIndex);
	char *ytv_asr_tiny_get_one_command_utf8(YTV_ASR_GEM_ENGINE_POINTER pEngineInstance, int nCommandIndex);


	

	void ytv_asr_tiny_notify_engine_not_wakeup(YTV_ASR_GEM_ENGINE_POINTER pEngineInstance, void (*pCallback)(void));
	void ytv_asr_tiny_notify_engine_wakeup(YTV_ASR_GEM_ENGINE_POINTER pEngineInstance, void (*pCallback)(void));
	char ytv_asr_tiny_get_engine_wakeup_status(YTV_ASR_GEM_ENGINE_POINTER pEngineInstance);
	
	
	int ytv_asr_tiny_set_callback_for_debug(void (*)(char *strMessage, unsigned int *pTimeSnapshot));
	short *ytv_asr_tiny_get_starting_part_from_vad(YTV_ASR_GEM_ENGINE_POINTER pEngineInstance, int *pOutSampleNumber);


	int ytv_asr_tiny_get_session_index(YTV_ASR_GEM_ENGINE_POINTER pEngineInstance);

	char *ytv_asr_tiny_get_memory_info_for_vad(YTV_ASR_GEM_ENGINE_POINTER pEngineInstance, unsigned int *pByteNumber);

#if 1
	void ytv_asr_tiny_set_wakeup_flag(void);
	void ytv_asr_tiny_set_interrupt_flag(void);
#endif


#ifdef __cplusplus
	}
#endif


#endif
