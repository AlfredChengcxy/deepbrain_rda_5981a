// Copyright 2017 Baidu Inc. All Rights Reserved.
// Author: Gang Chen (chengang12@baidu.com)
//
// Description: Media play progress statistic

#include "baidu_media_play_progress.h"
#include "mbed.h"
#include "lightduer_timestamp.h"
#include "lightduer_timers.h"
#include "lightduer_event_emitter.h"
#include "lightduer_log.h"

namespace duer {

struct PlayProgress {
    int             play_time;
    int             play_stuck_count;
};

static PlayProgress s_play_progress;
static PlayProgress s_pre_play_progress;
static int s_last_record_time;
static bool s_is_stucked;
static bool s_is_buffer_empty;
static rtos::Mutex s_play_progress_mutex;
static bool s_is_play_previous;
static media_stutter_cb s_stuttuer_cb;
static int s_media_flags = 0;
static duer_timer_handler s_timer;

void mdm_media_register_stuttered_cb(media_stutter_cb cb)
{
    s_stuttuer_cb = cb;
}

int MdmMeaidaPlayerListener::on_start(int flags) {
    s_play_progress_mutex.lock();

    s_media_flags = flags;
    s_is_stucked = false;
    s_is_buffer_empty = false;
    s_last_record_time = duer_timestamp();
    if (s_is_play_previous) {
        s_is_play_previous = false;
    } else {
        s_play_progress.play_stuck_count = 0;
        s_play_progress.play_time = 0;
    }
    s_play_progress_mutex.unlock();

    return 0;
}

int MdmMeaidaPlayerListener::on_stop(int flags) {
    s_play_progress_mutex.lock();

    if (!s_is_stucked) {
        int current_time = duer_timestamp();
        s_play_progress.play_time += current_time - s_last_record_time;
        s_last_record_time = current_time;
    }

    if (s_is_buffer_empty && !s_is_stucked) {
        duer_timer_stop(s_timer);
    }

    s_is_stucked = false;
    s_is_buffer_empty = false;
    s_pre_play_progress.play_stuck_count = s_play_progress.play_stuck_count;
    s_pre_play_progress.play_time = s_play_progress.play_time;

    s_play_progress_mutex.unlock();

    return 0;
}

int MdmMeaidaPlayerListener::on_finish(int flags) {
    s_play_progress_mutex.lock();

    if (s_is_buffer_empty && !s_is_stucked) {
        duer_timer_stop(s_timer);
    }
    s_is_stucked = false;
    s_is_buffer_empty = false;

    int current_time = duer_timestamp();
    s_play_progress.play_time += current_time - s_last_record_time;
    s_last_record_time = current_time;

    s_play_progress_mutex.unlock();

    return 0;
}

void MediaPlayBufferListener::on_wait_read() {
    s_play_progress_mutex.lock();
    s_is_buffer_empty = true;
    // It's a experiential value, we assume the data in codec buffer could play 200 ms
    duer_timer_start(s_timer, 200);
    s_play_progress_mutex.unlock();
}

void MediaPlayBufferListener::on_release_read() {
    s_play_progress_mutex.lock();
    s_last_record_time = duer_timestamp();

    if (s_is_stucked && s_stuttuer_cb) {
        s_stuttuer_cb(false, s_media_flags);
    }

    if (s_is_buffer_empty && !s_is_stucked) {
        duer_timer_stop(s_timer);
    }

    s_is_buffer_empty = false;
    s_is_stucked = false;
    s_play_progress_mutex.unlock();
};

void mdm_restore_previous_play_progress()
{
    s_play_progress_mutex.lock();
    s_is_play_previous = true;
    s_play_progress.play_stuck_count = s_pre_play_progress.play_stuck_count;
    s_play_progress.play_time = s_pre_play_progress.play_time;
    s_play_progress_mutex.unlock();
}

int mdm_get_play_progress()
{
    s_play_progress_mutex.lock();

    if (!s_is_stucked) {
        int current_time = duer_timestamp();
        s_play_progress.play_time += current_time - s_last_record_time;
        s_last_record_time = current_time;
    }

    s_pre_play_progress.play_stuck_count = s_play_progress.play_stuck_count;
    s_pre_play_progress.play_time = s_play_progress.play_time;

    s_play_progress_mutex.unlock();

    return s_play_progress.play_time;
}

static void mdm_media_stucked(int what, void *object)
{
    int current_time = 0;

    s_play_progress_mutex.lock();

    if (s_is_buffer_empty) {
        s_play_progress.play_stuck_count++;
        current_time = duer_timestamp();
        s_play_progress.play_time += current_time - s_last_record_time;
        s_last_record_time = current_time;
        s_is_stucked = true;
        if (s_stuttuer_cb) {
            s_stuttuer_cb(true, s_media_flags);
        }
    }

    s_play_progress_mutex.unlock();

}

static void mdm_stuck_timer_cb(void *param)
{
    duer_emitter_emit(mdm_media_stucked, 0, NULL);
}

void mdm_media_play_progress_init(MediaPlayBuffer* g_media_play_buffer)
{
    static MediaPlayBufferListener s_media_buffer_listener;
    static MdmMeaidaPlayerListener s_media_listener;

    s_timer = duer_timer_acquire(mdm_stuck_timer_cb, NULL, DUER_TIMER_ONCE);
    if (!s_timer) {
        DUER_LOGE("Failed to create timer to detect stuck event");
        return;
    }

    media_play_register_listener(&s_media_listener);
    g_media_play_buffer->set_listener(&s_media_buffer_listener);
}
} // namespace duer
