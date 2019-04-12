// Copyright (2016) Baidu Inc. All rights reserved.

#ifndef BAIDU_IOT_TINYDU_DEMO_EVENTS_H
#define BAIDU_IOT_TINYDU_DEMO_EVENTS_H

#include <mbed.h>

namespace duer {

enum Events {
	EVT_SHUTDOWN,		
	EVT_USB_CHANGE,
	EVT_VBAT_CHANGE,
	
    EVT_KEY_REC_PRESS,
    EVT_KEY_MULTI_PRESS,
    EVT_KEY_REC_RELEASE,
	
    EVT_KEY_MCHAT_PRESS,
    EVT_KEY_MCHAT_RELEASE,
    EVT_KEY_MCHAT_ALERT,
	EVT_KEY_MCHAT_PLAY,

    EVT_KEY_MAGIC_VOICE_PRESS,

	EVT_KEY_FAVORITE_PRESS,
	EVT_KEY_PIG_PRESS,
	EVT_KEY_MODE_PRESS,
	EVT_KEY_VOLUME_PRESS,

    EVT_KEY_EXTRA_LONGPRESS,
    EVT_KEY_EXTRA_RELEASE,
    EVT_KEY_PAUSE,
    EVT_RESTART,
    EVT_STOP,
    EVT_KEY_TEST,

	EVT_KEY_PLAY_MUSIC,
    EVT_KEY_VOICE_UP,
    EVT_KEY_VOICE_DOWN,
    EVT_KEY_START_RECORD,
	EVT_KEY_STOP_RECORD,

    EVT_KEY_START_VAD,
    EVT_KEY_START_MCHAT,
	EVT_KEY_ENTER_BT,
	EVT_KEY_BT_SWITCH, 
    EVT_KEY_SWITCH_MODE,
#ifdef ENABLE_INTERACTIVE_CLASS
    EVT_KEY_INTERACTIVE_CLASS,
#endif
	EVT_KEY_CLOSE_LIGHT,
	EVT_KEY_PLAY_PREV,
	EVT_KEY_PLAY_NEXT,

	EVT_KEY_FACTORY_KEY,
	EVT_KEY_FACTORY1_KEY,
	EVT_KEY_FACTORY2_KEY,
	EVT_KEY_FACTORY3_KEY,
	EVT_KEY_FACTORY4_KEY,
	EVT_KEY_FACTORY5_KEY,

    EVT_RESUME_PREVIOUS_MEDIA,
    EVT_RESET_WIFI,

	EVT_KEY_POWER_PRESS,
	EVT_KEY_POWER_RELEASE,



    EVT_TOTAL
};

#ifdef MBED_HEAP_STATS_ENABLED
extern void memory_statistics(const char* tag);
#define MEMORY_STATISTICS(...)      memory_statistics(__VA_ARGS__)
#else
#define MEMORY_STATISTICS(...)
#endif

typedef mbed::Callback<void ()> event_handle_func;

void event_set_handler(uint32_t evt_id, event_handle_func handler);

template<typename T, typename M>
void event_set_handler(uint32_t evt_id, T* obj, M method) {
    event_set_handler(evt_id, event_handle_func(obj, method));
}

void event_trigger(uint32_t evt_id);

void event_loop();

} // namespace duer

#endif // BAIDU_IOT_TINYDU_DEMO_EVENTS_H
