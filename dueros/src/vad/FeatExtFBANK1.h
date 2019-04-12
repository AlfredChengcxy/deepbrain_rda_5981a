#ifndef __FEAT_EXT_FBANK1_H__
#define __FEAT_EXT_FBANK1_H__

#include "types.h"
#include "filterbank.h"
#include "FeatExtBase.h"

/*
 *  Feature extractor using Sunjue's FBANK routines.
 */
class FeatExtFBANK1 : public FeatExtBase
{
    public:
        FeatExtFBANK1();
        virtual void ExtractFeature(const short* data_in, int data_size, float32_type* feat_out);

    private:
        short _feat_array[TRANSFORM_CHANSNUM_DEF];
};

#endif
