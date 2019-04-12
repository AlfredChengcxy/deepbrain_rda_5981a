#include "mbed.h"
#include "rtos.h"
#include "YTLight.h"
//#define __YT_THZY_LIGHT__
//#define __YT_SJTC_LIGHT__

#if 1
static void blink(void const *n) 
{
}
static RtosTimer led_yt_flash_timer(blink, osTimerOnce,NULL);
void yt_open_head_led()
{
}
void yt_open_all_led()
{
}
void yt_open_left_led()
{
}
void yt_open_right_led()
{
}
void yt_close_left_led()
{
}

void yt_close_right_led()
{
}

void yt_flash_begin(char type)
{
}
void yt_flash_end()
{
}

void yt_button_click_led()
{
}
#endif

YTLED YTLED::_s_instance;

YTLED& YTLED::instance() {
    return _s_instance;
}

YTLED::YTLED(){
}

YTLED::~YTLED(){
}

void YTLED::open_power()
{
}

void YTLED::close_power()
{
}

void YTLED::disp_head_led()
{
}

void YTLED::close_head_led()
{
}

void YTLED::click_led_button()
{
}

void YTLED::disp_led(LED_SCENE scene)
{
	static LED_SCENE cur_scene = LED_IDLE;
	led_yt_flash_timer.stop();

	if(cur_scene == LED_SHUTDOWN)
		return;
	
	switch(scene)		
	{
		case LED_IDLE:
			yt_open_all_led();
			break;

		case LED_CONNECTING_WIFI:
			yt_flash_begin(0);
			break;

		case LED_PLAY:
			yt_flash_begin(3);
			break;

		case LED_SEND_CONTENT:
			yt_close_left_led();
			break;
			
		case LED_SENDING:
			yt_flash_begin(2);
			break;
			
		case LED_SHUTDOWN:
			break;
		default:
			break;
	}

	cur_scene = scene;
}


