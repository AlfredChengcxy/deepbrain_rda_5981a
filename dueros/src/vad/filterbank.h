#ifndef _FILTERBANK_H_
#define _FILTERBANK_H_

#ifdef __cplusplus
extern "C"
{
#endif

#define TRANSFORM_CHANSNUM_DEF 24
#define TRANSFORM_CEPSNUM_DEF 12
#define FFT_NUM 512
#define HALF_FFT_NUM 256
#define WINDOWS_SIZE 400
#define FRAME_SHIFT 160
#define FALSE 0
#define TRUE 1
#define SINCOS_Q 14
#define TABLESIZE 1024

#ifndef INT32_MAX
#define INT32_MAX 2147483647
#endif

#ifndef INT32_MIN
#define INT32_MIN (-2147483647 - 1)
#endif

typedef struct tagFFTDesc
{
	short nRealOut;
	short nImageOut;
}TFFTDesc, *PFFTDesc;


void GetFeature(short *wavBuff, short *ps16FilterBank);
#ifdef __cplusplus
}
#endif

#endif
