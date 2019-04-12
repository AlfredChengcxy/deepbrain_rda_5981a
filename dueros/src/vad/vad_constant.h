#ifndef VAD_CONSTANT_H_
#define VAD_CONSTANT_H_

#include <stdio.h>
#include <time.h>

enum State {
    UNINIT = 1, IDLE = 2, BUSY = 3
};

class AudioChangeState {
public:
    const static int SPEECH2SPEECH = 0;
    const static int SPEECH2SIL = 1;
    const static int SIL2SIL = 2;
    const static int SIL2SPEECH = 3;
    const static int NOTBEGIN = 4;
    const static int INVALID = 5;
};

/*
*  µ¥Ö¡×´Ì¬
*
*/
class SingleFrameState {
public:
    const static int SPEECH = 1;
    const static int SIL = 0;
};

#endif
