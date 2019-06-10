/*********************************************************************************
  *Copyright(C),http://www.youngtone.cn
  *FileName:  YTQueue.h
  *Author:  Àî¾ý
  *Version:  v1.0
  *Date:  2018.11.11
**********************************************************************************/
#ifndef __YT_QUEUE_H__
#define __YT_QUEUE_H__

#include <stdlib.h>
#include <string.h>

class YTQueue
{
public:	
	static YTQueue &Instance();
	~YTQueue();	
	void Init(int _nUnitSize,int _nBlocks);	
	void Destory();	
	unsigned char * GetReadPtr();	
	void ReadDone();	
	unsigned char * GetWritePtr();	
	void WriteDone();	
	void Reset();
private:
	YTQueue();
private:	
	static YTQueue  mInstance;
	unsigned char ** qQueue;	
	int nUnitSize;	
	int nBlocks;	
	int nReadCnt;	
	int nWriteCnt;
};

#endif
