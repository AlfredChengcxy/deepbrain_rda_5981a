#ifndef __YT_DEBUG_H__
#define __YT_DEBUG_H__

void SetTime1();
void PrintTime();

void FileOpen();
void FileClose();
size_t FileWrite(const void *data,size_t size);


void FileOpen2();
void FileClose2();
size_t FileWrite2(const void *data,size_t size);

void Mchat_debug_test(char *pdata, unsigned int *size);

#if 0
void save_data_init();
int save_data(void* data, size_t size);
void play_data();
#endif
#endif
