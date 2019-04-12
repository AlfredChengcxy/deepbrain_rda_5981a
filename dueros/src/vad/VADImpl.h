#ifndef __VAD_IMPL_H__
#define __VAD_IMPL_H__

#include "types.h"
#include "VADBase.h"
#include "LoopBuffer.h"
#include "FeatExtFBANK1.h"
#include "Net.h"
#include "vad_config.h"
#include "windows_detect.h"
#include "VADVerdict.h"
#include "mdnn_fileio.h"


/*
 *
 */
class VADImpl : public VADBase
{
    public:
        enum {
            VAD_INSUFFICIENT_DATA,
            VAD_SILENCE,
            VAD_SPEECH
        };

        VADImpl(const char* model_path, int model_feature_context);
        VADImpl(MDNN_FILE* fp, int model_feature_context);
        virtual ~VADImpl();
        virtual int PushDataAndDetect(const short* data_in, int data_size);
        void LoadCMVNFile(const char* cmvn_path);
        void LoadCMVNFile(MDNN_FILE* fp);

    protected:
        const static int FBANK_DIM = 24;
        const static int FBANK_DELTA_DIM = FBANK_DIM * 3;
        const static int PCM_WINDOW_LENGTH = WINDOWS_SIZE;
        const static int PCM_WINDOW_SHIFT = FRAME_SHIFT;

        void CMVNFeature(const float32_type* raw_feature, float32_type* cmvned_feature);
        float32_type Float32ToInt8(const float32_type* float32_data, int8_type* int8_data, int size);
        void CMVNFloatToUInt8(const float32_type* float32_feature, uint8_type* uint8_feature);
        void InitDetectors();

        LoopBuffer<short> _loop_buffer;
        FeatExtFBANK1 _feat_ext;
        Net _net;
        VadConfiger* _vad_configurer_ptr;
        WindowsDetector* _windows_detector_ptr;
        VADVerdict* _vad_verdict;
        short* _data_buffer;
        //D_type _feat_buffer[TRANSFORM_CHANSNUM_DEF];
        int _feat_context;
        int _feat_context_window;   // _feat_context * 2 + 1
        int _feat_frame_count;
        float32_type* _cmvn_mean_neg;
        float32_type* _cmvn_std_inv;
        float32_type* _float32_feat_window; // cmvn'ed and context'ed feature
        int8_type* _int8_feat_window;       // cmvn'ed and context'ed feature, quantized

        int32_type _last_smoothed_state;
        int32_type _last_vad_status;
};

#endif
