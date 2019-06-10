/*********************************************************************************
  *Copyright(C),http://www.youngtone.cn
  *FileName:  YTQueue.cpp
  *Author:  Àî¾ý
  *Version:  v1.0
  *Date:  2018.11.11
**********************************************************************************/
#include "stdio.h"
#include "YTQueue.h"

YTQueue YTQueue::mInstance;

YTQueue &YTQueue::Instance()
{
	return mInstance;
}

YTQueue::YTQueue()	
	:qQueue(NULL)	
	,nReadCnt(0)	
	,nWriteCnt(0)
{
	
}

YTQueue::~YTQueue()
{	
	Destory();
}

void YTQueue::Init(int _nUnitSize,int _nBlocks)
{	
	nUnitSize = _nUnitSize + 1;
	nBlocks = _nBlocks;	
	Destory();	
	qQueue = new unsigned char *[nBlocks];	
	for(int i = 0; i< nBlocks; i++)	
	{		
		qQueue[i] = new unsigned char[nUnitSize];	
	}	
}

void YTQueue::Destory()
{	
	if(qQueue)	
	{		
		for(int i = 0; i< nBlocks; i++)		
		{			
			if(qQueue[i])delete qQueue[i];			
			qQueue[i] = NULL;		
		}		
		delete qQueue;		
		qQueue = NULL;	
	}
}

unsigned char * YTQueue::GetReadPtr()
{	
	if(nReadCnt >= nWriteCnt) return NULL;	
	return qQueue[nReadCnt %nBlocks];
}

void YTQueue::ReadDone()
{	
	nReadCnt ++;
}


unsigned char * YTQueue::GetWritePtr()
{	
	return qQueue[nWriteCnt %nBlocks];
}

void YTQueue::WriteDone()
{	
	nWriteCnt ++;
}

void YTQueue::Reset()
{	
	nReadCnt = 0;	
	nWriteCnt = 0;
	for(int i = 0; i< nBlocks; i++)	
	{	
		memset(qQueue[i],0,nUnitSize);	
	}

}


