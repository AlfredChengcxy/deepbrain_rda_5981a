
#ifndef _AMR_CODEC_CNS_HEADER_
#define _AMR_CODEC_CNS_HEADER_


#define YT_NB_AMR_DECODER_SIZE_IN_BYTE 6*1024


#ifndef ETSI
	#ifndef IF2
		#include <string.h>
		#define AMR_MAGIC_NUMBER "#!AMR\n"
	#endif
#endif




#ifdef __cplusplus
extern "C" {
#endif



/*----------------------------------------------------------------------------
 |YT_NB_AMR_InitDecoder(...)
 |PURPOSE
 |	This API intends to initialize an NB-AMR decoder
 |
 |INPUT
 |	pBufferForDecoder: buffer for holding the decoder
 |	BUF_SIZE_IN_BYTE: length of pBufferForDecoder in byte,
 |					  say YT_NB_AMR_DECODER_SIZE_IN_BYTE in header file
 |
 |OUTPUT
 |	none
 |
 |RETURN VALUE
 |	if normal, it returns pointer to Decoder; otherwise it returns NULL
 |
 |CODING
 | YoungTone Inc.
 *---------------------------------------------------------------------------*/
void *YT_NB_AMR_InitDecoder(char *pBufferForDecoder, unsigned int BUF_SIZE_IN_BYTE);



/*----------------------------------------------------------------------------
 |YT_NB_AMR_GetDataInfo(...)
 |PURPOSE
 |	This API intends to get information about NB-AMR stream
 |
 |INPUT
 |	pDataBitStream: buffer for holding AMR-encoded bitstream
 |	nDatalenInByte: length of pDataBitStream in byte,
 |OUTPUT
 |	pHeaderOffset: pointer to the offset for header
 |
 |RETURN VALUE
 |	if normal, it returns 0; otherwise it returns -1
 |
 |CODING
 | YoungTone Inc.
 *---------------------------------------------------------------------------*/
int YT_NB_AMR_GetDataInfo(char *pDataBitStream, unsigned int nDatalenInByte, unsigned int *pHeaderOffset);



/*----------------------------------------------------------------------------
 |YT_NB_AMR_DecodeFrame(...)
 |PURPOSE
 |	This API intends to decode an AMR frame
 |
 |INPUT
 |	pDecoderState: pointer to decoder created by YT_NB_AMR_InitDecoder(...)
 |	pDataBitStream: buffer for holding AMR-encoded bitstream
 |	nDatalenInByte: length of pDataBitStream in byte,
 |INOUT
 |	pOffsetInByte: pointer to the offset in pDataBitStream
 |
 |OUT
 |	pOutFrame: buffer for holding speech sample
 |	pSampleNumber: pointer to decoded speech sample number. It is 160 in most cases.
 |
 |RETURN VALUE
 |	if complete, it returns 0; otherwise it returns 1
 |
 |CODING
 | YoungTone Inc.
 *---------------------------------------------------------------------------*/
int YT_NB_AMR_DecodeFrame(void *pDecoderState,char *pDataBitStream,unsigned int nDataLenInByte,						   
						  unsigned int *pOffsetInByte, 
						  short *pOutFrame,unsigned int *pSampleNumber);





/*----------------------------------------------------------------------------
 |YT_NB_AMR_FreeDecoder(...)
 |PURPOSE
 |	This API intends to free the decoder
 |
 |INOUT
 |	ppDecoderState: pointer to the decoder pointer
 |OUT
 |	none
 |
 |RETURN VALUE
 |	it returns 0
 |
 |CODING
 | YoungTone Inc.
 *---------------------------------------------------------------------------*/
int YT_NB_AMR_FreeDecoder(void **ppDecoderState);










#ifdef __cplusplus
}
#endif	/* _cpp */



#endif