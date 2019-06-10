#ifndef __SPI_LOCAL_H__
#define __SPI_LOCAL_H__

#ifdef __clpusplus
extern "C"{
#endif

void spi_local_init();
void spi_local_getnext(unsigned int *paddr,int *nlen);/// 音频地址 和 长度
void spi_local_getpre(unsigned int *paddr,int *nlen);/// 音频地址 和 长度
void spi_local_get_frame(unsigned int addr,char *pdata,int nlen);/// 实际音频数据 和每帧长度

#ifdef __clpusplus
}
#endif

#endif