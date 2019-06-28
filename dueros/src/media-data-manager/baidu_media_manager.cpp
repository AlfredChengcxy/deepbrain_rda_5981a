// Copyright 2017 Baidu Inc. All Rights Reserved.
// Author: Chen Xihao (chenxihao@baidu.com)
//
// Description: Media Manager

#include "baidu_media_manager.h"
#include "baidu_media_data_manager.h"
#include "baidu_media_play.h"
#include "baidu_media_play_progress.h"
#include "baidu_common_buffer.h"
#include "lightduer_log.h"
#include "heap_monitor.h"

extern int Image$$ARM_LIB_STACK$$Base;
extern int Image$$RW_IRAM1$$ZI$$Limit;

namespace duer {

MediaManager MediaManager::_s_instance;
bool MediaManager::_s_initialized = false;

static unsigned char VOLUME = duer::DEFAULT_VOLUME;

MediaManager::MediaManager() {
}

MediaManager& MediaManager::instance() {
    return _s_instance;
}

bool MediaManager::initialize() {
    if (_s_initialized) {
        DUER_LOGI("MediaManager has initialized!");
        return false;
    }

    void* default_buffer = NULL;
    size_t size = 0;
    bool free = false;


////Image$$ARM_LIB_STACK$$Base = 0x11f358, Image$$RW_IRAM1$$ZI$$Limit = 0x11f358, size = 0

#ifdef TARGET_UNO_91H
    size = (size_t)&Image$$ARM_LIB_STACK$$Base - (size_t)&Image$$RW_IRAM1$$ZI$$Limit;
    default_buffer = &Image$$RW_IRAM1$$ZI$$Limit;
    DUER_LOGI("Image$$ARM_LIB_STACK$$Base = 0x%0x, Image$$RW_IRAM1$$ZI$$Limit = 0x%0x, size = %d",
        &Image$$ARM_LIB_STACK$$Base, &Image$$RW_IRAM1$$ZI$$Limit, size);
#endif

    if (size < MediaPlayBuffer::DEFAULT_BUFFER_SIZE) {
        size = MediaPlayBuffer::DEFAULT_BUFFER_SIZE;
        default_buffer = MALLOC(size, MEDIA);
        if (default_buffer == NULL) {
            //DUER_DS_LOG_AUDIO_MEMORY_OVERFLOW();
            DUER_LOGE("No free memory!");
            return false;
        }
        free = true;
    }

    IBuffer* buffer = NEW(MEDIA) CommonBuffer(default_buffer, size, free);
    if (buffer == NULL) {
        //DUER_DS_LOG_AUDIO_MEMORY_OVERFLOW();
        DUER_LOGE("No free memory!");
        return false;
    }

    bool ret = initialize(buffer);
    if (!ret) {
        delete buffer;
    }

    return ret;
}

bool MediaManager::initialize(IBuffer* buffer) {
    if (_s_initialized) {
        DUER_LOGI("MediaManager has initialized!");
        return false;
    }

    if (buffer == NULL) {
        DUER_LOGE("initialize failed: invalid argument!");
        return false;
    }

    if (buffer->size() < MediaPlayBuffer::MIN_BUFFER_SIZE) {
        DUER_LOGE("Buffer size must be %d at least!", MediaPlayBuffer::MIN_BUFFER_SIZE);
        return false;
    }

    g_media_play_buffer = NEW(MEDIA) MediaPlayBuffer(buffer);
    if (g_media_play_buffer == NULL) {
        //DUER_DS_LOG_AUDIO_MEMORY_OVERFLOW();
        DUER_LOGE("No free memory!");
        return false;
    }

    mdm_media_play_progress_init(g_media_play_buffer);

    start_media_data_mgr_thread();
    start_media_play_thread();
	
    _s_initialized = true;

    return true;
}

MediaPlayerStatus MediaManager::rec_start()
{
    MediaPlayerStatus status = get_media_player_status();
	if(status == MEDIA_PLAYER_BT)
		return status;

	media_record_start_record();
	
    return status;
}

MediaPlayerStatus MediaManager::rec_stop()
{
    MediaPlayerStatus status = get_media_player_status();
	if(status == MEDIA_PLAYER_BT)
		return status;

	media_record_stop_record();
	
    return status;
}

int MediaManager::read_data(char *read_ptr, int size)
{
    return media_record_read_data(read_ptr, size);
}

MediaPlayerStatus MediaManager::play_url(const char* url, int flags) {
    MediaPlayerStatus status = get_media_player_status();
	if((status == MEDIA_PLAYER_BT) || (status == MEDIA_PLAYER_RECORDING))
		return status;

	media_play_set_volume(VOLUME);
    mdm_send_media_url(url, flags);
    return status;
}

MediaPlayerStatus MediaManager::play_local(const char* path, int flags) {
    MediaPlayerStatus status = get_media_player_status();
	if((status == MEDIA_PLAYER_BT) || (status == MEDIA_PLAYER_RECORDING))
		return status;
	
    media_play_set_volume(VOLUME);
    mdm_send_media_file_path(path, flags);
    return status;
}

MediaPlayerStatus MediaManager::play_data(const char* data, size_t size, int flags) {
    MediaPlayerStatus status = get_media_player_status();
	if((status == MEDIA_PLAYER_BT) || (status == MEDIA_PLAYER_RECORDING))
		return status;

    media_play_set_volume(VOLUME);
    mdm_send_media_data(data, size, flags);
    return status;
}

MediaPlayerStatus MediaManager::play_magic_voice(char* data, size_t size, int flags) {
    MediaPlayerStatus status = get_media_player_status();
	if((status == MEDIA_PLAYER_BT) || (status == MEDIA_PLAYER_RECORDING))
		return status;

    media_play_set_volume(VOLUME);
    mdm_send_magic_voice(data, size, flags);
    return status;
}


MediaPlayerStatus MediaManager::pause_or_resume() {
    MediaPlayerStatus status = get_media_player_status();
	if((status == MEDIA_PLAYER_BT) || (status == MEDIA_PLAYER_RECORDING))
		return status;
    return mdm_pause_or_resume();
}

MediaPlayerStatus MediaManager::stop() {
    MediaPlayerStatus status = get_media_player_status();
	if((status == MEDIA_PLAYER_BT) || (status == MEDIA_PLAYER_RECORDING))
		return status;
	
    return mdm_notify_to_stop();
}

MediaPlayerStatus MediaManager::stop_completely() {
    MediaPlayerStatus status = get_media_player_status();
	if((status == MEDIA_PLAYER_BT) || (status == MEDIA_PLAYER_RECORDING))
		return status;
	
    return mdm_notify_to_stop_completely();
}

MediaPlayerStatus MediaManager::get_media_player_status() {
    return media_play_get_status();
}

int MediaManager::register_listener(MediaPlayerListener* listener) {
    return media_play_register_listener(listener);
}

int MediaManager::unregister_listener(MediaPlayerListener* listener) {
    return media_play_unregister_listener(listener);
}

void MediaManager::set_volume(unsigned char vol) {
	VOLUME = vol;
	
    MediaPlayerStatus status = get_media_player_status();
	if((status != MEDIA_PLAYER_BT) && (status != MEDIA_PLAYER_RECORDING)) {		
		media_play_set_volume(VOLUME);
	}
}

unsigned char MediaManager::get_volume() {
    return media_play_get_volume();
}

void MediaManager::voice_up_repeat()
{
	unsigned char s_volume = get_volume();

	DUER_LOGI("s_volume0: %d",s_volume);
	
	if(s_volume < duer::MAX_VOLUME)
		voice_up();
	else
		set_volume(duer::MIN_VOLUME);
}

void MediaManager::voice_up()
{
	unsigned char s_volume = get_volume();
	
#if 1
    if (s_volume < duer::MAX_VOLUME) {
        s_volume = s_volume + duer::UNIT_VOLUME > duer::MAX_VOLUME ?
                   duer::MAX_VOLUME : s_volume + duer::UNIT_VOLUME;
        set_volume(s_volume);
    }
#else
	if (s_volume >= duer::MIN_VOLUME && s_volume <= duer::DEFAULT_VOLUME ) 
	{
		s_volume = s_volume + UNIT_VOLUME > duer::MAX_VOLUME ?
				   duer::MAX_VOLUME : s_volume + UNIT_VOLUME;
		set_volume(s_volume);
	}
#endif	
	DUER_LOGI("s_volume1: %d",s_volume);
}





void MediaManager::voice_down()
{
	unsigned char s_volume = get_volume();
	
    if (s_volume > duer::MIN_VOLUME) {
        s_volume = s_volume - duer::UNIT_VOLUME < duer::MIN_VOLUME ?
                   duer::MIN_VOLUME : s_volume - duer::UNIT_VOLUME;
        set_volume(s_volume);
    }
	DUER_LOGI("s_volume1: %d",s_volume);
}

MediaPlayerStatus MediaManager::bt_mode() {
    MediaPlayerStatus status = get_media_player_status();	
    media_play_set_bt_mode();
	return status;
}


void MediaManager::set_bt_name(const unsigned char * name)
{
	media_set_bt_name(name);
}


MediaPlayerStatus MediaManager::uart_mode() {
    MediaPlayerStatus status = get_media_player_status();	
    media_play_set_uart_mode();
	return status;
}

void MediaManager::get_download_progress(size_t* total_size, size_t* recv_size) {
    mdm_get_download_progress(total_size, recv_size);
}

int MediaManager::get_play_progress() {
    return mdm_get_play_progress();
}

void MediaManager::register_stuttered_cb(media_stutter_cb cb) {
    mdm_media_register_stuttered_cb(cb);
}

void MediaManager::register_play_failed_cb(mdm_media_play_failed_cb cb) {
    mdm_reg_play_failed_cb(cb);
}

void MediaManager::seek(int position) {
    mdm_seek_media(position);
}

int MediaManager::set_previous_media(const char *url, int flags) {	
    return mdm_set_previous_media(url, flags);
}

int MediaManager::play_previous_media_continue() {
    MediaPlayerStatus status = get_media_player_status();
	if((status == MEDIA_PLAYER_BT) || (status == MEDIA_PLAYER_RECORDING))
		return status;
	
    return mdm_play_previous_media_continue();
}

} // namespace duer
