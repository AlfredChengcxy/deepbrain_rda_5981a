#ifndef RINGBUFF_H
#define RINGBUFF_H

#include "mbed.h"

#define MAX_FLAME 5
#define FLAME_SIZE 640

class Ringbuff
{
public:
    Ringbuff(int flame, int max_flame);
    ~Ringbuff(void);

	void init();

	int get_remain();
	void get_writePtr(char **writePtr);
	void get_readPtr(char **readPtr);
	
	void write_done();	
	void read_done();

	void print_bug();

private:
	int _size;
	int _max_flame;
	
	char *memory; 
	char *writePtr;
	char *readPtr;

	int remain;	
};

#endif
