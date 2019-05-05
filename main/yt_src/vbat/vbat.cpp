#include "mbed.h"
#include "rtos.h"
#include "vbat.h"
#include "lightduer_log.h"
#include "YTLight.h"
#include "YTManage.h"
#include "audio.h"
#include "rda_wdt_api.h"
#include "Rda_sleep_api.h"
#include "events.h"
#include "baidu_media_play.h"
#include "yt_key.h"

//#define LOG(_fmt, ...)      printf("[DEVC] ==> %s(%d): "_fmt"\n", __FILE__, __LINE__, ##__VA_ARGS__)
#define LOG(...) DUER_LOGI(__VA_ARGS__)


/// 开发板 用这个可能会造成死机
// other 
//AnalogIn ain(ADC_PIN2);


////kaifaban
AnalogIn ain(ADC_PIN0);

//static Thread s_vbat_thread(osPriorityHigh, 1024);
static Thread s_vbat_thread(osPriorityNormal, 1024);

const unsigned int vbat_low_power_alert_times = 5*60*1000; 
const unsigned int low_power_max_cnt = 5; 

static bool bVbatRun=true;
static VBAT_SCENE _vbat_status = VBAT_FULL;

static unsigned short get_vbat_avg()
{
	unsigned short _ival[10];
	unsigned short ival_max = 0;	
	unsigned short ival_min = 0;
	unsigned int sum = 0;
	int i = 0;

	_ival[0] = ain.read_u16();
	ival_max = ival_min = _ival[0];
	sum += _ival[0];
	
	for(i = 1; i < 10; i++)
	{		
		Thread::wait(2000);
		_ival[i] = ain.read_u16();
		if(_ival[i] < ival_min)
		{
			ival_min = _ival[i];
		}

		if(_ival[i] > ival_max)
		{
			ival_max = _ival[i];
		}

		sum += _ival[i];
	}

	sum -= ival_max;
	sum -= ival_min;

	return (unsigned short)(sum / 8);
}

bool vbat_check_startup()
{
	unsigned short _ival = ain.read_u16();
	
	if(_ival < 610)//3.65v
	{
		_vbat_status = VBAT_SHUTDOWN;					
		duer::YTMediaManager::instance().play_shutdown(YT_DB_LOW_BAT,sizeof(YT_DB_LOW_BAT));
		return true;
	}

	return false;
}

bool vbat_is_shutdown()
{
	return (_vbat_status == VBAT_SHUTDOWN);
}

#if 0
/*static */void vbat_sleep()
{
	printf("[vbat]sleep\r\n");

	duer::media_play_codec_off();
	sleep_set_level(SLEEP_POWERDOWN);			
	user_sleep_allow(1);

 #if 1
    bVbatRun=false;
    s_vbat_thread.terminate();
 #endif
 
}
#endif

static void vbat_check()
{
	unsigned short _ival;
    unsigned int   _delay = 1000*30;
	unsigned int alert_times = vbat_low_power_alert_times;
	unsigned int lowPowerCnt = 0;

	while(bVbatRun)
	{
		//static unsigned int   sleep_delay = 0;
		while(true)
		{
			_ival = get_vbat_avg();			
			LOG("[vbat] ival:%d states:%d",_ival ,_vbat_status);
#if 1
			if(_ival < 610)//3.55v
			{	
				lowPowerCnt++;
				if(lowPowerCnt > low_power_max_cnt) {
					_vbat_status = VBAT_SHUTDOWN;	
					LOG("_vbat_status = VBAT_SHUTDOWN");
					duer::YTMediaManager::instance().play_shutdown(YT_DB_LOW_BAT,sizeof(YT_DB_LOW_BAT));
					duer::yt_key_clear();
				} else {
					if(_vbat_status < VBAT_LOW_POWER)
					{			
						_vbat_status = VBAT_LOW_POWER;
					}
					
					if(alert_times > vbat_low_power_alert_times)
					{
						alert_times = 0;
						duer::YTMediaManager::instance().play_data(YT_DB_LOW_BAT,sizeof(YT_DB_LOW_BAT),duer::MEDIA_FLAG_PROMPT_TONE | duer::MEDIA_FLAG_SAVE_PREVIOUS);
					}
					else 
					{
						alert_times += _delay;
					}
				}
			}
			
#elif 0		
			//LOG("vbat ival:%d",ival);
			printf("vbat val:%d\r\n",_ival);
			if(_ival < 605+10)
			{
				if(_vbat_status < VBAT_NORMAL)
				{			
					_vbat_status = VBAT_NORMAL; 		
				}
			}
			
			if(_ival < 585+10)//3.55v
			{	
				if(_vbat_status < VBAT_LOW_POWER)
				{			
					_vbat_status = VBAT_LOW_POWER;
				}
			
				if(_ival >= 575+10)
				{
					if(alert_times > vbat_low_power_alert_times)
					{
						alert_times = 0;
						duer::YTMediaManager::instance().play_data(YT_DB_LOW_BAT,sizeof(YT_DB_LOW_BAT),duer::MEDIA_FLAG_PROMPT_TONE | duer::MEDIA_FLAG_SAVE_PREVIOUS);
					}
					else 
					{
						alert_times += _delay;
					}
				}
			}
			
			if(_ival < 575+10)//3.5v
			{
				if(_vbat_status < VBAT_SHUTDOWN)
				{			
					_vbat_status = VBAT_SHUTDOWN;
				}

				if(alert_times > vbat_low_power_alert_times)
				{
					alert_times = 0;
					duer::YTMediaManager::instance().play_data(YT_DB_LOW_BAT,sizeof(YT_DB_LOW_BAT),duer::MEDIA_FLAG_PROMPT_TONE | duer::MEDIA_FLAG_SAVE_PREVIOUS);				
				}
				else 
				{
					alert_times += _delay;
				}
			}

#elif 0
			if(_ival < 610)
			{
			    lowPowerCnt += 1;
				if(lowPowerCnt == 5)
				{			
                    
					duer::YTMediaManager::instance().play_data(YT_DB_LOW_BAT,sizeof(YT_DB_LOW_BAT),duer::MEDIA_FLAG_PROMPT_TONE | duer::MEDIA_FLAG_SAVE_PREVIOUS);				
	                deepbrain::yt_key_clear();				
				}
                if(lowPowerCnt == 6)
                {
                    lowPowerCnt=5;
                }				
			}
#endif

			//sleep_delay = duer::YTMediaManager::instance().is_playing() ? 0 : sleep_delay + delay;
			//if(sleep_delay > SLEEP_DELAY_MAX)
			//	duer::YTMediaManager::instance().play_data(YT_DB_LOW_BAT,sizeof(YT_DB_LOW_BAT),duer::MEDIA_FLAG_PROMPT_TONE | duer::MEDIA_FLAG_SAVE_PREVIOUS);				
			
			Thread::wait(_delay);
		}
	}
}

void vbat_start()
{	
	//duer::event_set_handler(duer::EVT_SHUTDOWN, vbat_sleep);
	s_vbat_thread.start(vbat_check);
}

