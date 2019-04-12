// Copyright 2017 Baidu Inc. All Rights Reserved.
// Author: Li Chang (changli@baidu.com)
//
// Description: Media Manager adpter for C caller


#include "baidu_media_manager_adapter.h"
#include "mbed.h"
#include "baidu_media_manager.h"

static mbed::Timer s_timer;

void timer_start() {
	s_timer.start();
}

void timer_stop() {
	s_timer.stop();
}

void timer_reset() {
	s_timer.reset();
}

unsigned int timer_read() {
	return s_timer.read();
}

void set_volume_adapter(unsigned char vol) {
	duer::MediaManager::instance().set_volume(vol);
}

unsigned char get_volume_adapter() {
	return duer::MediaManager::instance().get_volume();
}

void get_download_progress_adapter(unsigned int* total_size, unsigned int* recv_size) {
    duer::MediaManager::instance().get_download_progress(reinterpret_cast<size_t*>(total_size),
            reinterpret_cast<size_t*>(recv_size));
}

void media_play_url(const char* url) {
	duer::MediaManager::instance().play_url(url, duer::MEDIA_FLAG_UPNP_URL);
}

void media_pause_resume_url() {
	duer::MediaManager::instance().pause_or_resume();
}

void media_stop_url() {
	duer::MediaManager::instance().stop();
}

void media_seek(int pos) {
    duer::MediaManager::instance().seek(pos);
}

int get_player_status_adapter() {
    return duer::MediaManager::instance().get_media_player_status();
}


