#ifndef __FEAT_EXT_BASE_H__
#define __FEAT_EXT_BASE_H__

#include "types.h"

/*
 *  Feature extractor interface.
 *  ExtractFeature(): input is int16 PCM, output is uint8
 */
class FeatExtBase
{
    public:
        FeatExtBase() {}
        virtual void ExtractFeature(const short* data_in, int data_size, float32_type* feat_out) {}
};

#endif
