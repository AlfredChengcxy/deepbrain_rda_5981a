#ifndef __YT_RECORDER_H__
#define __YT_RECORDER_H__



namespace duer {
	//#define DISABLE_LOCAL_VAD
	void duer_recorder_amr_init_decoder(void **handler);

	bool duer_recorder_is_busy();
	void duer_recorder_set_vad(bool need_vad);

	void duer_recorder_stop();
	void duer_recorder_start();
	void duer_recorder_set_vad_asr(bool asr_mode);
}

#endif
