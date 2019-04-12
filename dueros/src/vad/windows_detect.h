#ifndef WINDOWS_DETECT_H_
#define WINDOWS_DETECT_H_

#include <vector>

#include "vad_constant.h"
#include "vad_config.h"

using namespace std;

class WindowsDetector {
public:
private:
    int win_smooth_size;
    int cur_win_smooth_pos;
    vector<vector<float> > winProbs;
    vector<vector<double> > winSmoothedProbs;
    vector<float> winProbs_sum;
    vector<double> win_max_smoothed_probs;
    int curwinSize;
    float speech_confidence_threshold;
    float sil_confidence_threshold;

    int curWinPos;
    int winSize;
    int preFrameState;
public:
    WindowsDetector(const VadConfiger *vadConfiger);
    ~WindowsDetector();

    int SmoothDetectorOneFrame(float *in, int len, int frameIdx);
    void reset();
};

#endif
