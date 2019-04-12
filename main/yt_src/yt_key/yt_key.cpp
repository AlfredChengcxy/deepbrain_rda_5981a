#include "mbed.h"
#include "yt_key.h"
#include "lightduer_timers.h"

namespace duer {


enum KeyEvent {
	KEY_NONE,
	KEY_PRESS,
	KEY_LONGPRESS,	
	KEY_RELEASE,
	KEY_TOTAL
};
	
struct GpadcKeyMessage {
	KeyEvent evt;
	uint8_t idx;
};

uint32_t YTGpadcKey::_key_fall_ts_map[PAD_KEY_NUM] = {0};
uint32_t YTGpadcKey::_key_longpress_time_map[PAD_KEY_NUM] = {0};
uint32_t YTGpadcKey::_key_longpress_type[PAD_KEY_NUM] = {0};
bool	 YTGpadcKey::_isLongPress[PAD_KEY_NUM] = {0};

Callback<void()> YTGpadcKey::_fall[PAD_KEY_NUM] = {0};
Callback<void()> YTGpadcKey::_rise[PAD_KEY_NUM] = {0};
Callback<void()> YTGpadcKey::_longpress[PAD_KEY_NUM] = {0};

static const unsigned int LONG_PRESS_THRESHOLD = 500; // unit: ms

static rtos::Mutex s_key_msg_mutex;
static rtos::Semaphore s_key_msg_sem(0);

static GpadcKeyMessage s_key_msg;
static GpadcKeyMessage key_msg;
static duer_timer_handler key_timer;

static Thread s_key_thread(osPriorityHigh, 2048);

static void yt_key_fall(uint8_t idx)
{
	if(YTGpadcKey::_fall[idx])
		YTGpadcKey::_fall[idx].call();

	YTGpadcKey::_isLongPress[idx] = false;
}

static void yt_key_rise(uint8_t idx)
{
	if(YTGpadcKey::_rise[idx] 
		&& (!YTGpadcKey::_isLongPress[idx]
		|| (YTGpadcKey::_key_longpress_type[key_msg.idx] == YT_LONG_KEY_WITH_RISE)))
		YTGpadcKey::_rise[idx].call();
	
	YTGpadcKey::_isLongPress[idx] = false;
}

static void yt_key_longpress(void *param)
{
	if(YTGpadcKey::_longpress[key_msg.idx])
	{		
		YTGpadcKey::_isLongPress[key_msg.idx] = true;
		YTGpadcKey::_longpress[key_msg.idx].call();
		if(YTGpadcKey::_key_longpress_type[key_msg.idx] == YT_LONG_KEY_PERIODIC)
			duer_timer_start(key_timer,YTGpadcKey::_key_longpress_time_map[key_msg.idx]);
	}
}

static void yt_key_thread() 
{
	while(true)
	{
		while (key_msg.evt == KEY_NONE) {
			rtos::Thread::signal_wait(0x01);
			s_key_msg_mutex.lock();
			key_msg.evt = s_key_msg.evt;
			key_msg.idx = s_key_msg.idx;
			s_key_msg.evt = KEY_NONE;
			s_key_msg_mutex.unlock();
		}
		
		switch (key_msg.evt) 
		{
			case KEY_PRESS:
				if(YTGpadcKey::_key_longpress_time_map[key_msg.idx] > 0)
				{
					duer_timer_start(key_timer, YTGpadcKey::_key_longpress_time_map[key_msg.idx]);
				}

				yt_key_fall(key_msg.idx);
				break;

			case KEY_RELEASE:
				if(YTGpadcKey::_key_longpress_time_map[key_msg.idx] > 0)
				{
					if(duer_timer_stop(key_timer) == 0)
					{
						yt_key_rise(key_msg.idx);
					}
				}
				else
				{
					yt_key_rise(key_msg.idx);
				}
				break;

			default:
				break;
		}

		key_msg.evt = KEY_NONE;
	}
}

YTGpadcKey::YTGpadcKey(mbed::KeyName key)
	:_key(key)
{
    _idx = (uint8_t)((uint8_t)key & ((0x01U << KEY_SHIFT) - 0x01U));
	_key.fall(this,&YTGpadcKey::key_fall);
	_key.rise(this,&YTGpadcKey::key_rise);
}

YTGpadcKey::~YTGpadcKey()
{
}

void YTGpadcKey::fall(Callback<void()> func)
{
	YTGpadcKey::_fall[_idx].attach(func);
}

void YTGpadcKey::rise(Callback<void()> func)
{	
	YTGpadcKey::_rise[_idx].attach(func);
}

void YTGpadcKey::longpress(Callback<void()> func,uint32_t time, uint32_t type)
{
	YTGpadcKey::_longpress[_idx].attach(func);
	YTGpadcKey::_key_longpress_time_map[_idx] = time;
	YTGpadcKey::_key_longpress_type[_idx] = type;
}

void YTGpadcKey::key_fall()
{	
    s_key_msg_mutex.lock();
    s_key_msg.evt = KEY_PRESS;
	s_key_msg.idx = _idx;
    s_key_msg_mutex.unlock();
	s_key_thread.signal_set(0x01);
}

void YTGpadcKey::key_rise()
{
    s_key_msg_mutex.lock();
    s_key_msg.evt = KEY_RELEASE;
	s_key_msg.idx = _idx;
    s_key_msg_mutex.unlock();
	s_key_thread.signal_set(0x01);
}

void yt_key_clear()
{	
	int i = 0;

	for(i = 0; i < PAD_KEY_NUM; i++)	
	{		
		YTGpadcKey::_fall[i].attach(NULL);
		YTGpadcKey::_rise[i].attach(NULL);
		YTGpadcKey::_longpress[i].attach(NULL);

		YTGpadcKey::_key_longpress_time_map[i] = 0;
		YTGpadcKey::_key_longpress_type[i] = -1;
	}
}

void yt_key_init()
{	
	key_timer = duer_timer_acquire(yt_key_longpress, NULL, DUER_TIMER_ONCE);	
    s_key_thread.start(yt_key_thread);
}
}
