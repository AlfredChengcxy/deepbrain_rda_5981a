#ifndef __YT_VAD_RDA_8000_INTERFACE_HEADER__
#define __YT_VAD_RDA_8000_INTERFACE_HEADER__

#define YT_VAD_8000_MEMORY_SIZE 15 *1024


#ifdef __cplusplus
 extern "C" 
 {
#endif


/*----------------------------------------------------------------------------
 |yt_vad_rda_8000_set_memory_buffer(...)
 |PURPOSE
 |	This API intends to set memory buffer for VAD engine
 |
 |INPUT
 |	pHeapBuffer: memory from heap
 |	nBufSize: buffer size in byte
 |
 |OUTPUT
 |	none
 |
 |RETURN VALUE
 |	It returns 0 if memory is enough; else it returns -1.
 *---------------------------------------------------------------------------*/
int yt_vad_rda_8000_set_memory_buffer(char *pHeapBuffer, unsigned int nBufSize);



/*----------------------------------------------------------------------------
 |yt_vad_rda_8000_get_instance(...)
 |PURPOSE
 |	This API intends to get a VAD instance
 |
 |INPUT
 |	none
 |
 |OUTPUT
 |	none
 |
 |RETURN VALUE
 |	It returns a pointer to VAD instance if normal; else it returns NULL.
 *---------------------------------------------------------------------------*/
void *yt_vad_rda_8000_get_instance(void);



/*----------------------------------------------------------------------------
 |yt_vad_rda_8000_initialize(...)
 |PURPOSE
 |	This API intends to initialize and configure a VAD instance
 |
 |INPUT
 |	pInstance: pointer to VAD instance
 |  nSamplingRate: sampling rate
 |
 |OUTPUT
 |	none
 |
 |RETURN VALUE
 |	It returns 0 if normal; else it returns -1.
 *---------------------------------------------------------------------------*/
int yt_vad_rda_8000_initialize(void *pInstance,  unsigned int nSamplingRate);




/*----------------------------------------------------------------------------
 |yt_vad_rda_8000_start_session(...)
 |PURPOSE
 |	This API intends to start a new session
 |
 |INPUT
 |	pInstance: pointer to VAD instance
 |
 |OUTPUT
 |	none
 |
 |RETURN VALUE
 |	none
 *---------------------------------------------------------------------------*/
void yt_vad_rda_8000_start_session(void *pInstance);




/*----------------------------------------------------------------------------
 |yt_vad_rda_8000_close_session(...)
 |PURPOSE
 |	This API intends to close a new session
 |
 |INPUT
 |	pInstance: pointer to VAD instance
 |
 |OUTPUT
 |	none
 |
 |RETURN VALUE
 |	none
 *---------------------------------------------------------------------------*/
void yt_vad_rda_8000_close_session(void *pInstance);



/*----------------------------------------------------------------------------
 |yt_vad_rda_8000_clear(...)
 |PURPOSE
 |	This API intends to clear variables within a session
 |
 |INPUT
 |	pInstance: pointer to VAD instance
 |
 |OUTPUT
 |	none
 |
 |RETURN VALUE
 |	none
 *---------------------------------------------------------------------------*/
void yt_vad_rda_8000_clear(void *pInstance);



/*----------------------------------------------------------------------------
 |yt_vad_rda_8000_reset_for_new_start(...)
 |PURPOSE
 |	This API intends to reset variables for a new start within a session
 |
 |INPUT
 |	pInstance: pointer to VAD instance
 |
 |OUTPUT
 |	none
 |
 |RETURN VALUE
 |	none
 *---------------------------------------------------------------------------*/
void yt_vad_rda_8000_reset_for_new_start(void *pInstance);







	/*----------------------------------------------------------------------------
	 |yt_vad_rda_8000_input_data_to_start(...)
	 |PURPOSE
	 |	This API intends to detect speech starting point 
	 |
	 |INPUT
	 |	pInstance: an instance pointer, especiall for multi-threaded applications 
	 |	pSampleBuffer: buffer for holding speech frame
	 |	nSampleNumber: sample number
	 |
	 |OUTPUT
	 |	pOutSampleBuffer: buffer for holding speech samples
	 |	pOutSampleNumber: pointer to output sample number
	 |	pFlag_VAD: pointer to VAD flag
	 |
	 |RETURN VALUE
	 |	It returns 0 if normal; else it returns a negative value
	 *---------------------------------------------------------------------------*/

	int yt_vad_rda_8000_input_data_to_start(void *pInstance, 
										  short *pSampleBuffer, 
										  int nSampleNumber,
										  short *pOutSampleBuffer,
										  int *pOutSampleNumber,
										  char *pFlag_VAD);


	/*----------------------------------------------------------------------------
	 |yt_vad_rda_8000_vad_get_starting_part(...)
	 |PURPOSE
	 |	This API intends to get the starting portion after 
	 |	speech starting point is detected...
	 |
	 |INPUT
	 |	pInstance: an instance pointer, especiall for multi-threaded applications 
	 |
	 |OUTPUT
	 |	pOutSampleNumber: pointer to number of output samples
	 |
	 |RETURN VALUE
	 |	It returns the corresponding buffer.
	 *---------------------------------------------------------------------------*/
	short *yt_vad_rda_8000_get_starting_part(void *pInstance, int *pOutSampleNumber);





	/*----------------------------------------------------------------------------
	 |yt_vad_rda_8000_vad_input_data_to_end(...)
	 |PURPOSE
	 |	This API intends to detect speech endpoint 
	 |
	 |INPUT
	 |	pInstance: an instance pointer, especiall for multi-threaded applications 
	 |	pSampleBuffer: buffer for holding speech frame
	 |	nSampleNumber: sample number
	 |
	 |OUTPUT
	 |	pOutSampleBuffer: buffer for holding speech samples. 
	 |					  In most cases, it holds the same data as the input frame
	 |	pOutSampleNumber: pointer to output sample number
	 |	pHistoryMaxSum: pointer to historic maximum sample
	 |	pUttFinalFlag: pointer to utterance flag
	 |	pCurMinSum: pointer to current minimum value
	 |  pProcessedSampleBuffer: hold the processed sample buffer
	 |
	 |RETURN VALUE
	 |	It returns 0 if normal; else it returns a negative value
	 *---------------------------------------------------------------------------*/
	int yt_vad_rda_8000_input_data_to_end(void *pInstance, 
										  short *pSampleBuffer, 
										  int nSampleNumber,
										  short *pOutSampleBuffer,
										  int *pOutSampleNumber,
										  unsigned int *pHistoryMaxSum,
										  unsigned char *pUttFinalFlag,
										  unsigned int *pCurMinSum,
										  short *pProcessedSampleBuffer);




/*----------------------------------------------------------------------------
 |yt_vad_rda_8000_set_mv_mode(...)
 |PURPOSE
 |	This API intends to set vad for magic voice function 
 |
 |INPUT
 |	pInstance: pointer to VAD instance
 |
 |OUTPUT
 |	none
 |
 |RETURN VALUE
 |	none
 *---------------------------------------------------------------------------*/
void yt_vad_rda_8000_set_mv_mode(void *pInstance);



/*----------------------------------------------------------------------------
 |yt_vad_rda_8000_set_asr_mode(...)
 |PURPOSE
 |	This API intends to set vad for asr function
 |
 |INPUT
 |	pInstance: pointer to VAD instance
 |
 |OUTPUT
 |	none
 |
 |RETURN VALUE
 |	none
 *---------------------------------------------------------------------------*/
void yt_vad_rda_8000_set_asr_mode(void *pInstance);




#ifdef __cplusplus
	}
#endif

#endif
