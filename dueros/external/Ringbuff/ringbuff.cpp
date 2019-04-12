#include "ringbuff.h"

Ringbuff::Ringbuff(int flame_size, int max_flame)
	:_size(flame_size),
	_max_flame(max_flame)
{
	memory = (char*)malloc(_size*_max_flame);
}

Ringbuff::~Ringbuff(void)
{
	free(memory);
}

void Ringbuff::init(void)
{	
	memset(memory,0x00,_size*_max_flame);
	
	writePtr = memory;
	readPtr = memory;
	remain = 0;
}

void Ringbuff::get_writePtr(char **m_writePtr)
{
	//if(remain == _size*_max_flame)
	//{
		//printf("[RB]full\r\n",writePtr);
	//}
	
	*m_writePtr = writePtr;	
	//printf("[RB]get_writePtr:%p\r\n",writePtr);
}

int Ringbuff::get_remain()
{
	return remain;
}

void Ringbuff::get_readPtr(char **m_readPtr)
{
	if(remain == 0)
	{
		*m_readPtr = NULL;
		return;
	}
	
	*m_readPtr = readPtr;	
	//printf("[RB]get_readPtr:%p\r\n",readPtr);
}

void Ringbuff::write_done()
{
	remain += _size;
	if(remain == _size*_max_flame)
	{
		//printf("[RB]full\r\n",writePtr);
		read_done();
	}
	
	if(writePtr + _size >= memory + _size*_max_flame)
		writePtr = memory;
	else
		writePtr +=  _size;
}

void Ringbuff::read_done()
{
	remain -= _size;
	
	if(readPtr + _size >= memory + _size*_max_flame)
		readPtr = memory;
	else
		readPtr +=  _size;
}

void Ringbuff::print_bug()
{
	int i = 0;
	short *pInputSample = (short*)memory;
	int nSampleNumber = (writePtr - memory)/sizeof(short);
	
	for(i = 0;i < nSampleNumber; i+=2)
	{
		printf("%d,",pInputSample[i]);
	}
}

