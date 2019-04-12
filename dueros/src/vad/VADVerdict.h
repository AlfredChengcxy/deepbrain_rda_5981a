#include "vad_constant.h"
#include "types.h"
#include "helper.h"
#include <vector>

class VADVerdict {
    public:
        enum MFEState {
            UNKNOWN = 0,
            BOOT = 1,
            IDLE = 2,
            RUN = 3,
        };

        enum VADState {
            SILENCE = 0,
            SILENCE_TO_SPEECH = 1,
            SPEECH_TO_SILENCE = 2,
            NO_SPEECH = 3,
            SPEECH_TOO_SHORT = 4,
            SPEECH_TOO_LONG = 5,
            SPEECH_PAUSE = 6,  /**< @brief For Speech Input */
            NO_DETECT = 7,  /**< @brief No Data Arrived */
        };

        enum DetectResult {
            RET_SILENCE = 0,
            RET_SILENCE_TO_SPEECH = 1,
            RET_SPEECH_TO_SILENCE = 2,
            RET_NO_SPEECH = 3,
            RET_SPEECH_TOO_SHORT = 4,
            RET_SPEECH_TOO_LONG = 5,
            RET_RESERVE1 = 6,
            RET_RESERVE2 = 7,
            RET_RESERVE3 = 8,
            RET_RESERVE4 = 9,
        };

        VADVerdict();

        int32_type VerdictOnSmoothedState(int32_type state);

    private:
        uint64_type lVADResultStartLoc;          /**< @brief VAD Result Start Loc (Frame) */
        uint64_type lVADResultCurLoc;            /**< @brief VAD Result Current Loc (Frame) */
        uint64_type lFrameCnt;                   /**< @brief Frame Counter (Last time) */
        uint64_type lFrameCntTotal;              /**< @brief For SpeechMode = 1, Total Frame Cnt */
        uint32_type nStartFrame;                 //start in vad calculation-window (160/256)
        uint32_type nEndFrame;                   //end in vad calculation-window (160/256)

        uint32_type nCurState;                   /**< @brief Current State of LSM */
        uint32_type nVADCurState;
        uint32_type nVADLastState;

        uint32_type nSpeech_End;
        uint32_type nSpeech_Mode;

        int16_type nIn_Speech_Threshold;         //how many continuous frames can be considered as the beginning of speech
        uint32_type nOffsetLength;
        uint32_type nVADInnerCnt;                /**< @brief If there is eight "1", change VADState */
        uint32_type nVADInnerZeroCnt;
        uint32_type nSpeechEndCnt;               /**< @brief Used ONLY in SpeechMode 1 */
        uint32_type nSpeechEncLength;            /**< @brief Speech Encoder Framelength: BV: 80; AMR: 320 */

        uint32_type nn_nMax_Speech_Pause;
        uint32_type nn_nMax_Wait_Duration;
        uint32_type nn_nMax_Speech_Duration;
        uint32_type nn_nMin_Speech_Duration;

        uint32_type nn_nStartBackFrame;          //how many frames should be backdated as start when a sil2speech is detected
        uint32_type nn_nEndBackFrame;            //how many frames should be backdated as end when a speech2sil or a speech2pause is detected
        uint32_type nn_nLastSpeechFrame;

        uint32_type nFindPossibleEndPoint;
        uint32_type nPossible_Speech_Pause_Init;
        uint32_type nPossible_Speech_Pause;      //a threshold, how many continuous frames(non-speech) might be a speech-pause
        uint32_type nPossible_Speech_Start;

        int32_type st_package;

        //std::vector<int32_type> seg_st;
        //std::vector<int32_type> seg_et;

        int32_type MakeSegment();
        int32_type GenerateReturnStatus();
};
