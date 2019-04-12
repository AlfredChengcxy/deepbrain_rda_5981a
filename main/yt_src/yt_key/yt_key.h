#ifndef __YT_KEY_H__
#define __YT_KEY_H__

#include "GpadcKey.h"
#include "Callback.h"

namespace duer {


enum _yt_long_key_enum {
    YT_LONG_KEY_ONCE,
    YT_LONG_KEY_PERIODIC,
    YT_LONG_KEY_WITH_RISE,
    YT_LONG_KEY_TOTAL
};
	

class YTGpadcKey 
{
public:
    YTGpadcKey(mbed::KeyName key);
    virtual ~YTGpadcKey();

	void rise(Callback<void()> func);

	template<typename T, typename M>
	void rise(T *obj, M method) {
		rise(Callback<void()>(obj, method));
	}

	void fall(Callback<void()> func);

	template<typename T, typename M>
	void fall(T *obj, M method) {
		fall(Callback<void()>(obj, method));
	}

	void longpress(Callback<void()> func, uint32_t time, uint32_t type);

	template<typename T, typename M>
	void longpress(T *obj, M method, uint32_t time, uint32_t type) {
		 longpress(Callback<void()>(obj, method), time,type);
	}

	void key_fall();	
	void key_rise();
	
    static uint32_t _key_fall_ts_map[PAD_KEY_NUM];
	static uint32_t _key_longpress_time_map[PAD_KEY_NUM];
	static uint32_t _key_longpress_type[PAD_KEY_NUM];
	static	bool	_isLongPress[PAD_KEY_NUM];

	static Callback<void()> _fall[PAD_KEY_NUM];
	static Callback<void()> _rise[PAD_KEY_NUM];
	static Callback<void()> _longpress[PAD_KEY_NUM];
	
private:

	mbed::GpadcKey  _key;
	uint8_t   _idx;
};

void yt_key_clear();
void yt_key_init();

}
#endif
