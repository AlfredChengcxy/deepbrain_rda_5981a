// Copyright 2017 Baidu Inc. All Rights Reserved.
// Author: Chen Xihao (chenxihao@baidu.com)
//
// Description: Media player API

#include "baidu_media_play.h"
#include <limits.h>
#include "baidu_media_adapter.h"
#include "baidu_media_play_m4a.h"
#include "heap_monitor.h"
#include "lightduer_log.h"
#include "baidu_measure_stack.h"
#include "baidu_mp3_frame_header_parser.h"
#include "YTLight.h"
namespace duer {

struct ListenerListNode {
    MediaPlayerListener* listener;
    ListenerListNode*    next;
};

struct ListenerList {
    rtos::Mutex mutex;
    ListenerListNode* first;
    ListenerListNode* last;
};

MediaPlayBuffer* g_media_play_buffer = NULL;

static const uint32_t MEDIA_PLAY_STACK_SIZE = 1024 * 4;
static Thread s_media_player_thread(osPriorityAboveNormal, MEDIA_PLAY_STACK_SIZE);

static unsigned char s_volume = 0xff;

static const int32_t S_RESUME_SIGNAL = 0x01;
static volatile MediaPlayerStatus s_media_player_status = MEDIA_PLAYER_IDLE;

// Is Mutex needed?
// static Mutex s_status_mutex;
static ListenerList s_listeners;

static MediaAdapter s_media_adapter;
static MediaPlayM4A* s_media_m4a = NULL;

static volatile int s_write_codec_size = 0;
static MediaPlayM4A* s_previous_m4a = NULL;
static bool s_finished = false; // indicate last play finished or stopped halfway
static int s_current_media_flags = 0;
static uint32_t s_sleep_time = 0;
static rtos::Semaphore s_play_end_sem;
static int s_previous_bitrate = 0;

// if s_error < 0, error occured
static volatile int s_error = 0;

void media_play_codec_on()
{
	s_media_adapter.close_power(4);	
}

void media_play_codec_off()
{
	s_media_adapter.open_power(4);	
}

void media_play_set_5856_pin_on(int pin_name)
{
	s_media_adapter.open_power(pin_name);
}

void media_play_set_5856_pin_off(int pin_name)
{
	s_media_adapter.close_power(pin_name);
}

void media_play_set_volume(unsigned char vol) {
    if (s_volume != vol) {
        s_volume = vol;
        s_media_adapter.regulate_voice(s_volume);
    }
}

unsigned char media_play_get_volume() {
    return s_volume;
}

void media_play_set_uart_mode() 
{	
	s_media_adapter.setUartMode();	
	s_media_player_status = MEDIA_PLAYER_IDLE;
}

void media_play_set_bt_mode() 
{	
	if(s_media_player_status == MEDIA_PLAYER_IDLE)
	{
		s_media_player_status = MEDIA_PLAYER_BT;	
		media_play_codec_on();
		s_media_adapter.setBtMode();	
	}
}

static void on_media_player_start(int flags) {
    s_listeners.mutex.lock();
    ListenerListNode* node = s_listeners.first;
    while (node != NULL) {
        node->listener->on_start(flags);
        node = node->next;
    }
    s_listeners.mutex.unlock();
}

static void on_media_player_stop(int flags) {
    s_listeners.mutex.lock();
    ListenerListNode* node = s_listeners.first;
    while (node != NULL) {
        node->listener->on_stop(flags);
        node = node->next;
    }
    s_listeners.mutex.unlock();
}

static void on_media_player_finish(int flags) {
    s_listeners.mutex.lock();
    ListenerListNode* node = s_listeners.first;
    while (node != NULL) {
        node->listener->on_finish(flags);
        node = node->next;
    }
    s_listeners.mutex.unlock();
}

/* only bitrate <= 32kbps need to sleep after write data to codec */
static uint32_t get_sleep_time_by_bitrate(int bitrate) {
    // the frame size write to codec is 1024, so play one frame cost time:
    // 8kbps--1000ms, 16kbps--500ms, 32kbps--250ms
    uint32_t ms = 0;
    switch (bitrate) {
    case 8000:
        ms = 950;
        break;
    case 16000:
        ms = 450;
        break;
    case 32000:
        ms = 200;
        break;
    default:
        // do nothing
        break;
    }

    return ms;
}

static void handle_event_play() {
    static unsigned char buffer[FRAME_SIZE];
    MediaPlayerMessage media_message;
    g_media_play_buffer->read(&media_message, sizeof(media_message));
    int size = media_message.size;
	
    if (size > FRAME_SIZE) {
        s_error = -1;
        DUER_LOGE("message's buffer size is bigger than FRAME_SIZE!");
        return;
    }

    if (size > 0) {
        g_media_play_buffer->read(buffer, size);
    }

    int ret = 0;
    MediaType type = media_message.type;
    DUER_LOGV("commond:%d, type:%d, size:%d\n", media_message.cmd, type, size);
    DUER_LOGV("media player status:%d.", s_media_player_status);
    static uint32_t last_write_time = 0;

    switch (media_message.cmd) {
    case PLAYER_CMD_START:
        s_write_codec_size = 0;
        if (MEDIA_PLAYER_IDLE == s_media_player_status) {
            DUER_LOGD("start play");
            s_media_player_status = MEDIA_PLAYER_PLAYING;
            s_finished = false;
            s_sleep_time = 0;

            s_current_media_flags = media_message.flags;
            if ((media_message.flags & MEDIA_FLAG_SEEK) == 0) {
                on_media_player_start(media_message.flags);
            }
			// open codec_power
			media_play_codec_on();
			//zt add 20180702

            if (type == TYPE_M4A) {
                if ((media_message.flags & MEDIA_FLAG_CONTINUE_PLAY) == 0) {
                    media_play_clear_previous_m4a();

                    s_media_m4a = NEW(MEDIA) MediaPlayM4A(&s_media_adapter);
                    if (NULL == s_media_m4a) {
                        ret = -1;
                        //DUER_DS_LOG_AUDIO_MEMORY_OVERFLOW();
                        DUER_LOGE("No free memory.");
                        break;
                    }

                    ret = s_media_m4a->m4a_file_start(buffer, size);
                } else {
                    DUER_LOGD("Continue play m4a");
                    if (NULL == s_previous_m4a) {
                        ret = -1;
                        DUER_LOGE("s_previous_m4a is null.");
                        break;
                    }
                    s_media_m4a = s_previous_m4a;
                    s_previous_m4a = NULL;

                    ret = s_media_adapter.start_play(TYPE_M4A, 0);
                    if (ret < 0) {
                        break;
                    }

                    ret = s_media_m4a->m4a_file_play(buffer, size);
                }
            } else if (type == TYPE_MP3 || type == TYPE_AAC || type == TYPE_WAV
                    || type == TYPE_TS || type == TYPE_AMR) {
                int bitrate = 0;
                if (type == TYPE_MP3) {
                    if (media_message.flags & MEDIA_FLAG_CONTINUE_PLAY) {
                        bitrate = s_previous_bitrate;
                        DUER_LOGD("restore bitrate = %d", bitrate);
                    } else {
                        bitrate = get_mp3_frame_bitrate(buffer, size);
                        if (((media_message.flags & MEDIA_FLAG_PROMPT_TONE) == 0)
                                && ((media_message.flags & MEDIA_FLAG_DCS_URL)
                                || (media_message.flags & MEDIA_FLAG_UPNP_URL)
                                || (media_message.flags & MEDIA_FLAG_LOCAL))) {
                            s_previous_bitrate = bitrate;
                            DUER_LOGD("save bitrate = %d", bitrate);
                        }
                    }
                    s_sleep_time = get_sleep_time_by_bitrate(bitrate);
                }
                ret = s_media_adapter.start_play(type, bitrate);
                if (ret < 0) {
                    break;
                }

                ret = s_media_adapter.write(buffer, size);
                last_write_time = us_ticker_read();
            } else {
                ret = -1;
                DUER_LOGE("media type error:%d.", type);
            }
            s_write_codec_size += size;

            // TODO: log bitrate, sample_rate, bits_per_sample, channels
            // duer_ds_log_audio_info(0, 0, 0, 0);
        } else {
            ret = -1;
            DUER_LOGE("PLAYER_CMD_START status error:%d!", s_media_player_status);
        }
        break;
    case PLAYER_CMD_STOP:
        if ((MEDIA_PLAYER_PAUSE == s_media_player_status)
			|| (MEDIA_PLAYER_PLAYING== s_media_player_status))
         {
            //stop play, get queue till empty later
            if (type == TYPE_M4A) {
                if (NULL == s_media_m4a) {
                    ret = -1;
                    DUER_LOGE("cmd stop s_media_m4a null.");
                    break;
                }
                ret = s_media_m4a->m4a_file_end(buffer, size);
                delete s_media_m4a;
                s_media_m4a = NULL;
            } else if (type == TYPE_MP3 || type == TYPE_AAC || type == TYPE_WAV
                    || type == TYPE_TS || type == TYPE_AMR) {
                if (size > 0) {
                    ret = s_media_adapter.write(buffer, size);
                    if (ret < 0) {
                        break;
                    }
                }
                ret = s_media_adapter.write(NULL, 0); // stop with ending
            } else {
                ret = -1;
                DUER_LOGE("media type error:%d.", media_message.type);
            }

            s_write_codec_size += size;

            // wait till codec play end
            while (!s_media_adapter.is_play_stopped()) {
                Thread::wait(100);
            }
			// close codec_power
			media_play_codec_off();
			//zt add 20180702

            // media_play_stop maybe called while waiting
            if (s_media_player_status != MEDIA_PLAYER_IDLE) {
                s_finished = true;
                s_media_player_status = MEDIA_PLAYER_IDLE;
                on_media_player_finish(media_message.flags);
                DUER_LOGI("play end");
            }

        } else {
            DUER_LOGD("media player halfway stop!\n");

            if (s_media_m4a != NULL) {
                media_play_clear_previous_m4a();

                s_previous_m4a = s_media_m4a;
                s_media_m4a = NULL;
            }
        }

        if (!media_play_error()) {
            s_play_end_sem.release();
        }
        break;
    case PLAYER_CMD_CONTINUE:
        if ((MEDIA_PLAYER_PAUSE == s_media_player_status)
			|| (MEDIA_PLAYER_PLAYING== s_media_player_status))
		{
            //play the data
            if (type == TYPE_M4A) {
                if (NULL == s_media_m4a) {
                    ret = -1;
                    DUER_LOGE("cmd continue s_media_m4a null.");
                    break;
                }
                ret = s_media_m4a->m4a_file_play(buffer, size);
            } else if (type == TYPE_AAC || type == TYPE_WAV
                    || type == TYPE_TS || type == TYPE_AMR) {
                ret = s_media_adapter.write(buffer, size);
            } else if (type == TYPE_MP3) {
                uint32_t now = us_ticker_read();
                uint32_t delta_ms = 0;
                if (now < last_write_time) {
                    delta_ms = (UINT_MAX - last_write_time + now) / 1000;
                } else {
                    delta_ms = (now - last_write_time) / 1000;
                }

                ret = s_media_adapter.write(buffer, size);

                // reduce write codec frequency
                if (s_sleep_time > 0 && s_sleep_time > delta_ms) {
                    DUER_LOGD("s_sleep_time = %d, delta_ms = %d", s_sleep_time, delta_ms);
                    Thread::wait(s_sleep_time - delta_ms);
                }
                last_write_time = us_ticker_read();
            } else {
                ret = -1;
                DUER_LOGE("media type error:%d.", media_message.type);
            }
            s_write_codec_size += size;
        } else {
            DUER_LOGV("media player halfway stop!\n");
        }
        break;
    default:
        ret = -1;
        DUER_LOGE("!!!media commond Error!!!");
        break;
    }

    if (ret < 0) {
        s_error = -1;
    }

    return;
}

static void pause_play() {
	media_play_codec_off();
#if 1
	YTLED::instance().disp_led(LED_IDLE);
#endif	
    s_media_adapter.pause_play();

    DUER_LOGI("media play thread pause\n");
    Thread::signal_wait(S_RESUME_SIGNAL);
    DUER_LOGI("media play thread resume\n");
}

static void media_play_thread() {
    DUER_LOGD("init media_play");
    while (true) {
        if (s_media_player_status == MEDIA_PLAYER_PAUSE) {
            pause_play();
        }
        handle_event_play();
    }
}

MediaPlayerStatus media_play_pause_or_resume() {
    MediaPlayerStatus oldStatus = s_media_player_status;
    if (oldStatus == MEDIA_PLAYER_PAUSE) {		
		media_play_codec_on();	
#if 1
		YTLED::instance().disp_led(LED_PLAY);
#endif	
        s_media_player_thread.signal_set(S_RESUME_SIGNAL);
        s_media_player_status = MEDIA_PLAYER_PLAYING;
    } else if (oldStatus == MEDIA_PLAYER_PLAYING) {
        s_media_player_status = MEDIA_PLAYER_PAUSE;
    } else {
        //do nothing
    }
    return oldStatus;
}

MediaPlayerStatus media_play_stop(bool no_callback) {
    MediaPlayerStatus oldStatus = s_media_player_status;
    s_media_player_status = MEDIA_PLAYER_IDLE;
    s_media_adapter.stop_play();
    if (oldStatus == MEDIA_PLAYER_PAUSE) {
        s_media_player_thread.signal_set(S_RESUME_SIGNAL);
    }

    if (((oldStatus == MEDIA_PLAYER_PLAYING) || (oldStatus == MEDIA_PLAYER_PAUSE)) && !no_callback) {
        DUER_LOGI("play stop");
        on_media_player_stop(s_current_media_flags);
    }

    return oldStatus;
}

void start_media_play_thread() {
#ifdef BAIDU_STACK_MONITOR
    register_thread(&s_media_player_thread, "MediaPlayer");
#endif
    s_media_player_thread.start(media_play_thread);
}

MediaPlayerStatus media_play_get_status() {
    return s_media_player_status;
}

int media_play_register_listener(MediaPlayerListener* listener) {
    if (listener == NULL) {
        DUER_LOGW("listener is NULL");
        return -1;
    }

    s_listeners.mutex.lock();
    // check whether register repeat
    ListenerListNode* node = s_listeners.first;
    while (node != NULL) {
        if (node->listener == listener) {
            s_listeners.mutex.unlock();
            DUER_LOGW("listener has been registered before.");
            return -1;
        }
        node = node->next;
    }

    node = (ListenerListNode*)MALLOC(sizeof(ListenerListNode), MEDIA);
    if (node == NULL) {
        s_listeners.mutex.unlock();
        //DUER_DS_LOG_AUDIO_MEMORY_OVERFLOW();
        DUER_LOGE("No free memory!");
        return -1;
    }

    node->listener = listener;
    node->next = NULL;

    if (s_listeners.last == NULL) {
        s_listeners.first = s_listeners.last = node;
    } else {
        s_listeners.last->next = node;
        s_listeners.last = node;
    }

    s_listeners.mutex.unlock();

    return 0;
}

int media_play_unregister_listener(MediaPlayerListener* listener) {
    if (listener == NULL) {
        DUER_LOGW("listener is NULL");
        return -1;
    }

    s_listeners.mutex.lock();

    ListenerListNode* node = s_listeners.first;
    ListenerListNode* prev = NULL;
    while (node != NULL) {
        if (node->listener == listener) {
            if (s_listeners.first == s_listeners.last) { // list has one item only
                s_listeners.first = s_listeners.last = NULL;
            } else if (prev == NULL) { // item is first node
                s_listeners.first = node->next;
            } else {
                prev->next = node->next;
                if (node == s_listeners.last) { // item is last node
                    s_listeners.last = prev;
                }
            }
            FREE(node);
            break;
        }

        prev = node;
        node = node->next;
    }
    s_listeners.mutex.unlock();

    return 0;
}

int media_play_get_write_size() {
    return s_write_codec_size;
}

int media_play_seek_m4a_file(int position) {
    if (s_previous_m4a != NULL) {
        return s_previous_m4a->m4a_file_seek(position);
    }

    return 0;
}

bool media_play_finished() {
    return s_finished;
}

void media_play_clear_previous_m4a() {
    if (s_previous_m4a != NULL) {
        delete s_previous_m4a;
        s_previous_m4a = NULL;
    }
}

bool media_play_error() {
    return s_error < 0;
}

void media_play_reset_error() {
    s_error = 0;
    s_write_codec_size = 0;
}

void media_play_wait_end() {
    // if play has not started or error occurred, don't need to wait
    if (s_write_codec_size > 0 && !media_play_error()) {
        s_play_end_sem.wait();
    }
}

void media_record_start_record()
{
    if (s_media_player_status == MEDIA_PLAYER_IDLE) {
		s_media_player_status = MEDIA_PLAYER_RECORDING;
		s_media_adapter.start_record(TYPE_AMR);
    }
	
}

void media_record_stop_record()
{	
	if(s_media_player_status == MEDIA_PLAYER_RECORDING)
	{
		s_media_adapter.stop_record();		
		s_media_player_status = MEDIA_PLAYER_IDLE;
	}
}

int media_record_read_data(char *data, int size)
{
	return (s_media_player_status == MEDIA_PLAYER_RECORDING) ?  s_media_adapter.read(data, size) : 0; 
}

void media_bt_play()
{
	if(s_media_player_status == MEDIA_PLAYER_BT)
	{
		int status = 0;
		s_media_adapter.bt_getA2dpstatus(&status);
		DUER_LOGI("media_bt_play A2dpstatus:%d",status);
		if(status == 2)//A2DP_CONNECTED
		{			
			//media_play_codec_on();
			s_media_adapter.bt_play();
		}
		else
		{
			//media_play_codec_off();
			s_media_adapter.bt_pause(); 				
		}
	}
}

void media_bt_pause()
{
	if(s_media_player_status == MEDIA_PLAYER_BT)
	{
		s_media_adapter.bt_pause();		
		//media_play_codec_off();
	}
}

void media_bt_forward()
{
	if(s_media_player_status == MEDIA_PLAYER_BT)
	{
		s_media_adapter.bt_forward();		
	}
}

void media_bt_backward()
{
	if(s_media_player_status == MEDIA_PLAYER_BT)
	{
		s_media_adapter.bt_backward();		
	}
}

void media_bt_volup()
{
	if(s_media_player_status == MEDIA_PLAYER_BT)
	{
		s_media_adapter.bt_volup();		
	}
}

void media_bt_voldown()
{
	if(s_media_player_status == MEDIA_PLAYER_BT)
	{
		s_media_adapter.bt_voldown();		
	}
}

} // namespace duer
