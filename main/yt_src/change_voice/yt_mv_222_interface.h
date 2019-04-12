#ifndef __YT_MV_222_INTERFACE_HEADER__
#define __YT_MV_222_INTERFACE_HEADER__


#define YT_MV_222_CHANGER_SIZE_IN_BYTE   28*1024
#define YT_MV_222_FRAME_SHIFT 160


#ifdef __cplusplus
extern "C" {
#endif


	/*----------------------------------------------------------------------------
	 |YT_MV_222_InitChanger(...)
	 |PURPOSE
	 |	This API intends to initialize a voice changer for magic voice(voice morphing)
	 |
	 |INPUT
	 |	pBufferForChanger: buffer for holding the changer
	 |	nBufSizeForChanger: length of pBufferForChanger in byte,
	 |					    say YT_MV_222_CHANGER_SIZE_IN_BYTE in header file
	 |
	 |OUTPUT
	 |	none
	 |
	 |RETURN VALUE
	 |	if successful, it returns pointer to the changer; otherwise it returns NULL
	 |
	 |CODING
	 | YoungTone Inc.
	 *---------------------------------------------------------------------------*/
	void *YT_MV_222_InitChanger(char *pBufferForChanger, unsigned int nBufSizeForChanger);




	/*----------------------------------------------------------------------------
	 |YT_MV_222_ChangeFrame(...)
	 |PURPOSE
	 |	This API intends to change voice frame with specified type and parameter
	 |
	 |INPUT
	 |	pVoiceChanger: pointer to voice changer
	 |	pInFrame: buffer for input speech frame in PCM format
	 |	nInSampleNumber: speech sample number of input frame, 
	 |					 say YT_MV_222_FRAME_SHIFT in header file
	 |	SOUND_EFFECT_TYPE:  sound effect type, default value is 0
	 |						
	 |	SOUND_EFFECT_PAR:   parameter for sound effect type, such as pitch level.
	 |					    This argument depends on sound effect type.
	 |						Default value is 0
	 |					  
	 |OUTPUT
	 |	pOutFrame:    buffer for holding changed voice frame.
	 |				  Application layer needs to allocate memory in advance.
	 |				  Its size(in short) refers to YT_MV_222_FRAME_SHIFT in header file
	 |	pOutSampleNumber£º pointer to output sample number, which can be 0 or value less than YT_MV_222_FRAME_SHIFT
	 |
	 |
	 |RETURN VALUE
	 |	if successful, it returns 0; otherwise returns -1.
	 |
	 |CODING
	 | YoungTone Inc.
	 *---------------------------------------------------------------------------*/
	int YT_MV_222_ChangeFrame(void *pVoiceChanger, short *pInFrame, unsigned int nInSampleNumber, int SOUND_EFFECT_TYPE, int SOUND_EFFECT_PAR, short *pOutFrame, int *pOutSampleNumber);





	/*----------------------------------------------------------------------------
	 |YT_MV_222_NotifySessionEnd(...)
	 |PURPOSE
	 |	This API intends to notify the voice changer session ending.
	 |	For instance, we have processed one utterance.
	 |
	 |INPUT
	 |	pVoiceChanger: pointer to voice changer
	 |
	 |OUTPUT
	 |	none
	 |
	 |RETURN VALUE
	 |	It returns 0 if normal; otherwise returns -1
	 |
	 |CODING
	 | YoungTone Inc.
	 *---------------------------------------------------------------------------*/
	int YT_MV_222_NotifySessionEnd(void *pVoiceChanger);



	/*----------------------------------------------------------------------------
	 |YT_MV_222_GetLastPartForSession(...)
	 |PURPOSE
	 |	This API intends to get the last part(remianing part) for one session
	 |
	 |INPUT
	 |	pVoiceChanger: pointer to voice changer
	 |					  
	 |OUTPUT
	 |	pOutFrame:    buffer for holding changed voice frame.
	 |				  Application layer needs to allocate memory in advance.
	 |				  Its size(in short) refers to YT_MV_222_FRAME_SHIFT in header file
	 |	pOutSampleNumber£º pointer to output sample number, which can be 0 or value less than YT_MV_222_FRAME_SHIFT
	 |
	 |
	 |RETURN VALUE
	 |	if successful, it returns 0; otherwise returns -1.
	 |
	 |CODING
	 | YoungTone Inc.
	 *---------------------------------------------------------------------------*/
	int YT_MV_222_GetLastPartForSession(void *pVoiceChanger,short *pOutFrame, int *pOutSampleNumber);




	/*----------------------------------------------------------------------------
	 |YT_MV_222_ResetChanger(...)
	 |PURPOSE
	 |	This API intends to reset a voice changer.
	 |
	 |INPUT
	 |	pVoiceChanger: pointer to voice changer
	 |
	 |OUTPUT
	 |	none
	 |
	 |RETURN VALUE
	 |	none
	 |
	 |CODING
	 | YoungTone Inc.
	 *---------------------------------------------------------------------------*/
	void YT_MV_222_ResetChanger(void *pVoiceChanger);





	/*----------------------------------------------------------------------------
	 |YT_MV_222_FreeChanger(...)
	 |PURPOSE
	 |	This API intends to free a voice changer. Actually, it does NOTHING!!!
	 |	The application layer needs to manage memory allocated for the changer.
	 |
	 |INPUT
	 |	pVoiceChanger: pointer to voice changer
	 |
	 |OUTPUT
	 |	none
	 |
	 |RETURN VALUE
	 |	It returns -1 if pVoiceChanger is NULL; otherwise returns 0
	 |
	 |CODING
	 | YoungTone Inc.
	 *---------------------------------------------------------------------------*/
	int YT_MV_222_FreeChanger(void *pVoiceChanger);



#ifdef __cplusplus
}
#endif	//for cpp compatibility







#endif
