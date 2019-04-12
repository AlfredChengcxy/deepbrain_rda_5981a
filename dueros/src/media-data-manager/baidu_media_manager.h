// Copyright 2017 Baidu Inc. All Rights Reserved.
// Author: Chen Xihao (chenxihao@baidu.com)
//
// Description: Media Manager

#ifndef BAIDU_TINYDU_IOT_OS_SRC_MEDIA_DATA_MANAGER_BAIDU_MEDIA_MANAGER_H
#define BAIDU_TINYDU_IOT_OS_SRC_MEDIA_DATA_MANAGER_BAIDU_MEDIA_MANAGER_H

#include <stddef.h>
#include "baidu_media_play_listener.h"
#include "baidu_media_play_type.h"

namespace duer {

class IBuffer;

class MediaManager {
public:
    static MediaManager& instance();

    /*
     * Initialize MediaManager object.
     *
     * @Return bool, success: true, fail: false
     */
    bool initialize();

    /*
     * Initialize MediaManager object by buffer .
     *
     * @Param buffer, buffer for media data
     * @Return bool, success: true, fail: false
     */
    bool initialize(IBuffer* buffer);

	//zafter rec
	MediaPlayerStatus rec_start();
	MediaPlayerStatus rec_stop();
	
	int read_data(char *read_ptr, int size);
	
	MediaPlayerStatus bt_mode();
	MediaPlayerStatus uart_mode();


    /*
     * Play network media file
     *
     * @Param url, media file's url
     * @param flags, media play flags defined reference enum MediaFlag
     * @Return MediaPlayerStatus, the last status of media player
     */
    MediaPlayerStatus play_url(const char* url, int flags);

    /*
     * Play local media file
     *
     * @Param path, media file's path
     * @param flags, media play flags defined reference enum MediaFlag
     * @Return MediaPlayerStatus, the last status of media player
     */
    MediaPlayerStatus play_local(const char* path, int flags);

    /*
     * Play media data, only support mp3 data now
     *
     * @Param data, media data buffer, can't be freed before play end
     * @Return MediaPlayerStatus, the last status of media player
     */
    MediaPlayerStatus play_data(const char* data, size_t size, int flags);

	MediaPlayerStatus play_magic_voice(char* data, size_t size, int flags);

    /*
     * If media player's status is playing/pause, pause/resume it
     *
     * @Return MediaPlayerStatus, the last status of media player
     */
    MediaPlayerStatus pause_or_resume();

    /*
     * Stop media player
     *
     * @Return MediaPlayerStatus, the last status of media player
     */
    MediaPlayerStatus stop();

    /*
     * Stop media player completely(no connection play)
     *
     * @Return MediaPlayerStatus, the last status of media player
     */
    MediaPlayerStatus stop_completely();

    /*
     * Get status of media player
     *
     * @Return MediaPlayerStatus, the current status of media player
     */
    MediaPlayerStatus get_media_player_status();

    /*
     * Register the media player listener
     *
     * @Param listener, media player listener
     * @Return int, success: 0, fail: -1
     */
    int register_listener(MediaPlayerListener* listener);

    /*
     * Unregister the media player listener
     *
     * @Param listener, media player listener
     * @Return int, success: 0, fail: -1
     */
    int unregister_listener(MediaPlayerListener* listener);

    /*
     * Set volume of audio
     *
     * @Param vol, effective range is 0~15
     */
    void set_volume(unsigned char vol);

	void voice_up_repeat();
	void voice_up();
	void voice_down();
	

    /*
     * Get volume of audio
     *
     * @Return volume, effective range is 0~15
     */
    unsigned char get_volume();

    /*
     * Get http download progress
     *
     * PARAM:
     * @param[out] total_size: to receive the total size to be download
     *                         if chunked Transfer-Encoding is used, we cannot know the total size
     *                         untile download finished, in this case the total size will be 0
     * @param[out] recv_size:  to receive the data size have been download
     *
     * @return     none
     */
    void get_download_progress(size_t* total_size, size_t* recv_size);

    /*
     * Get play progress(ms).
     *
     * @param      none.
     *
     * @return     the play progress.
     */
    int get_play_progress();

    /*
     * Seek media file playing now
     *
     * PARAM:
     * @param[in] position, seek postion
     */
    void seek(int position);

    /*
     * Resume play the previous media
     *
     * @Return int, success: 0, no previous media found: MEDIA_ERROR_NO_PREVIOUS_URL, fail: -1
     */
    int set_previous_media(const char *url, int flags);
    int play_previous_media_continue();

    /*
     * Register a callback which will be called when media stuttuer started/finished.
     *
     * @return     none
     */
    void register_stuttered_cb(media_stutter_cb cb);

    /*
     * Register a callback which will be called when media play failed.
     *
     * @return     none
     */
    void register_play_failed_cb(mdm_media_play_failed_cb cb);

private:
    MediaManager();

    static MediaManager _s_instance;
    static bool _s_initialized;
};

} // namespace duer

#endif // BAIDU_TINYDU_IOT_OS_SRC_MEDIA_DATA_MANAGER_BAIDU_MEDIA_MANAGER_H
