#ifndef __ST_MV_INF_H__
#define __ST_MV_INF_H__

#ifdef __cplusplus
extern "C"{
#endif

 

void* YT_MV_InitChanger();
void YT_MV_SetConfig(void *pVoiceChanger,
					 double dRateChange,
					 double dTempo,
					 int nPitchSemiTone);
int YT_MV_ChangeFrame(void *pVoiceChanger, 
					  short *pInFrame, unsigned int nInSampleNumber, 
					  short *pOutFrame, int pOutSampleNumber);
void YT_MV_ResetChanger(void *pVoiceChanger);
int YT_MV_FreeChanger(void *pVoiceChanger);

#ifdef __cplusplus
}
#endif

#endif