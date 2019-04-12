/** file rda58xx.h
 * Headers for interfacing with the codec chip.
 */

#ifndef Rt5670_H
#define Rt5670_H

#include "mbed.h"

class Rt5670
{
public:
    Rt5670();
    ~Rt5670(void);
	void init(void);
	void start_record();
	void stop_record();
	int stop_play();
	int write_data(void* data, int size);
	int read_data(void* data, int size);
	int read_done();
	int write_done();
};

#endif

