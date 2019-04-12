#include <stdio.h>
#include <string.h>

#include <mbed.h>
#include "mbed_stats.h"
//#include "spiRAM.h"
//#include "events.h"
#include "YTDebug.h"
//#include "ES8388_interface.h"

#define LOG(_fmt, ...)      printf("[DEVC] ==> %s(%d): "_fmt"\n", __FILE__, __LINE__, ##__VA_ARGS__)

//const uint32_t ramSize = 0x3FFFFF;           // 4M x 8 bit
//SpiRAM sRam(PD_2, PD_3, PD_0, PD_1);
//uint32_t sRamIdex = 0;
static uint32_t debug_time = 0;
static uint32_t debug_time2 = 0;

void SetTime1()
{
	debug_time = us_ticker_read();
}

void PrintTime()
{
	if(debug_time)
	{
		LOG("used time:%d",(us_ticker_read() - debug_time)/1000);				
	}
}

FILE *fp = NULL;
FILE *fp2 = NULL;

void FileOpen()
{
	static int fileindex = 0;
	char filename[128];
	
	memset(filename,0x00,sizeof(filename));
	sprintf(filename,"/sd/REC/1REC%d.bin",fileindex);
	printf("filename:%s\r\n",filename);
	fileindex++;
		
	fp = fopen(filename, "w+");
	if(NULL == fp) {
		LOG("Could not open file for write!\r\n");
	}
}

void FileClose()
{	
	if(fp)
		fclose(fp);

	fp = NULL;
}

size_t FileWrite(const void *data,size_t size)
{
	size_t ret = 0;

	if(fp)
	  ret = fwrite(data, size, 1, fp);

	return ret;
}

void FileOpen2()
{
	static int fileindex = 0;
	char filename[128];
	
	memset(filename,0x00,sizeof(filename));
	sprintf(filename,"/sd/REC/2REC%d.bin",fileindex);
	LOG("filename:%s\r\n",filename);
	fileindex++;
		
	fp2 = fopen(filename, "w+");
	if(NULL == fp2) {
		LOG("Could not open file for write!\r\n");
	}
}

void FileClose2()
{	
	if(fp2)
		fclose(fp2);

	fp2 = NULL;
}

size_t FileWrite2(const void *data,size_t size)
{
	size_t ret = 0;

	if(fp2)
	  ret = fwrite(data, size, 1, fp2);

	return ret;
}

void Mchat_debug_test(char *pdata, unsigned int *size)
{
	FILE *fp = fopen("/sd/AMR/111.amr", "r");
	if(NULL == fp) {
		LOG("Mchat_debug_test open fail");
		*size = 0;
		return;
	}

	unsigned int read_bytes = 0;
	unsigned int totle_bytes = 0;
	
	do{
         read_bytes = fread(pdata + totle_bytes, 1, 1024, fp);
		 totle_bytes += read_bytes;
 	}while(read_bytes >= 1024);

	*size = totle_bytes;
	fclose(fp);
}

#if 0
void save_data_init()
{
	sRamIdex = 0;
}

int save_data(void* data, size_t size)
{	
	int i = 0;
	short data_L[160] = {0};

	LOG("save_data sRamIdex:%d size:%d",sRamIdex,size);

	for(i = 0; i < 160; i++)
	{			
		memcpy(data_L + i, (short*)data + i*2, sizeof(short));
	}
	
	sRam.writeBuffer(sRamIdex, data_L, sizeof(data_L));
	sRamIdex += sizeof(data_L);

	if(sRamIdex > 0x0000FFFF)
	{		
		return -1;
	}

	return 0;
}

void play_data()
{	
	int Idex = 0;
	int i = 0;
	short data[320] = {0};
	unsigned int tmp[320] = {0};

	LOG("play_data start sRamIdex:%d",sRamIdex);
	while(Idex < sRamIdex)
	{
		sRam.readBuffer(Idex, (void*)data, sizeof(data));
		for(i = 0; i < 320; i++)
		{
			tmp[i] = data[i]&0x000000FF;		
			tmp[i] &= 0x0000FFFF;
		}
		I2s_Send_Data((void*)tmp, 640);
		Idex += sizeof(data);
	}	

	sRamIdex = 0;
	
	LOG("play_data end");
}
#endif
