#ifndef YTMANAGE_H
#define YTMANAGE_H

#include "baidu_media_manager.h"

namespace duer {

enum YTMediaPlayingType {
	YTMED_NO_PLAYING,
	YTMED_PLAYING_URL,
	YTMED_PLAYING_PATH,
	YTMED_PLAYING_DATA,
};

typedef struct mediaInfo{
	const char *media;
	int media_len;
}mediaInfo;

class YTMediaManager
{
public:	
	YTMediaManager();
	virtual ~YTMediaManager();

    static YTMediaManager& instance();
	void init();
	void deinit();

	int get_media_status();
	bool is_playing();
	bool is_bt();
	void on_start();	
	void on_stop();
	void on_finish();	
	void stop();
	void stop_completely();
	
	void set_volume(unsigned char vol);	
	unsigned char get_volume();
	void volume_up_repeat();
	void volume_up();
	void volume_down();

	int get_play_progress();	
	void pause_or_resume();

	bool is_recording();
	void rec_start();
	void rec_stop();
	int rec_read_data(char *read_ptr, int size);

	int set_wchat_queue(const char *url);
	int play_wchat_queue();

	int set_music_queue(const char *url);
	int play_queue();
	int play_queue_v2(int type);
	int clear_queue();
	
	int set_previous_media(const char* url, int flags);
	int play_previous_media_continue();
	
	void play_prev();
	void play_next();
	void play_url(const char* url, int flags);
	void play_local(const char* path, int flags);
	void play_data(const char* data, size_t size, int flags);
	void play_magic_voice(char* data, size_t size, int flags);
	void play_shutdown(const char* data, size_t size);

	void start_bt();
	void switch_bt(void);
	
private:
    static YTMediaManager s_instance;	
};

}
#endif
