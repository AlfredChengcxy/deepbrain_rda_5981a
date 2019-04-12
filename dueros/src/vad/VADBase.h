#ifndef __VAD_BASE_H__
#define __VAD_BASE_H__

#include "types.h"

/*
 *
 */
class VADBase
{
    public:
        VADBase() {}

        // Push data to internal buffer.
        // When data is sufficient, do VAD.
        // Return:
        //  -1: No sufficient data in buffer.
        //   0: No voice detected.
        //   1: Voice detected.
        virtual int PushDataAndDetect(const short* data_in, int data_size) = 0;
};

#endif
