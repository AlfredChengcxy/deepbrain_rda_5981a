#include <stdlib.h>
#include <stdio.h>

#include "userconfig.h"
#include "wifi_manage.h"
//#include "player_middleware_interface.h"
#include "cJSON.h"
#include "mbed.h"
#include "rtos.h"
#include "inet.h"
#include "WiFiStackInterface.h"
#include "rda5981_sniffer.h"
#ifdef AIRKISS
#include "rda_airkiss.h"
#else
#include "AirkissNet.h"
#endif
#include "rda_sys_wrapper.h"
#include "YTManage.h"
#include "audio.h"

#define LOG_TAG "wifi manage"

//wifi manage handle
typedef struct WIFI_MANAGE_HANDLE_t
{
	WIFI_MANAGE_STATUS_t status;
	DEVICE_WIFI_INFOS_T wifi_infos;
	DEVICE_WIFI_INFO_T curr_wifi;
	int32_t connect_index;
	uint64_t connecting_start_time;
	
	WiFiStackInterface *wifi_handler;

	//wifi 消息队列，用于接收wifi断网等消息
	void *wifi_msg_queue;
	
	//资源互斥锁
	void *mutex_lock;
}WIFI_MANAGE_HANDLE_t;

WiFiStackInterface g_wifi_handler;
static WIFI_MANAGE_HANDLE_t* g_wifi_manage_handle = NULL;


static rtos::Thread *g_thWifi = NULL;


void* get_rda_network_interface(void)
{
    return (void*)&g_wifi_handler;
}

static bool stop_wifi_airkiss_mode(void);

/**
 * update wifi connect time
 *
 * @param none
 * @return true/false
 */
static bool update_wifi_connect_time(void)
{
	if (g_wifi_manage_handle == NULL)
	{
		return false;
	}

	SEMPHR_TRY_LOCK(g_wifi_manage_handle->mutex_lock);
	g_wifi_manage_handle->connecting_start_time = get_time_of_day();
	SEMPHR_TRY_UNLOCK(g_wifi_manage_handle->mutex_lock);

	return true;
}

/**
 * update current wifi infomation
 *
 * @param [in]ssid,the name of the wifi
 * @param [in]password
 * @return true/false
 */
static bool update_current_wifi_info(
	const char *ssid, 
	const char *password)
{
	DEVICE_WIFI_INFO_T *wifi_info = NULL;
	
	if (g_wifi_manage_handle == NULL 
		|| ssid == NULL
		|| strlen(ssid) == 0
		|| password == NULL)
	{
		return false;
	}

	wifi_info = &g_wifi_manage_handle->curr_wifi;
	SEMPHR_TRY_LOCK(g_wifi_manage_handle->mutex_lock);
	snprintf(wifi_info->wifi_ssid, sizeof(wifi_info->wifi_ssid), "%s", ssid);
	snprintf(wifi_info->wifi_passwd, sizeof(wifi_info->wifi_passwd), "%s", password);
	SEMPHR_TRY_UNLOCK(g_wifi_manage_handle->mutex_lock);

	return true;
}

/**
 * set wifi status
 *
 * @param [in]status,wifi manage status
 * @return none
 */
static bool set_wifi_manage_status(WIFI_MANAGE_STATUS_t status)
{
	if (g_wifi_manage_handle == NULL)
	{
		return false;
	}

	SEMPHR_TRY_LOCK(g_wifi_manage_handle->mutex_lock);
	g_wifi_manage_handle->status = status;
	SEMPHR_TRY_UNLOCK(g_wifi_manage_handle->mutex_lock);

	return true;
}

/**
 * get wifi status
 *
 * @param none
 * @return int
 */
int get_wifi_manage_status(void)
{
	int status = -1;
	
	if (g_wifi_manage_handle == NULL)
	{
		return status;
	}

	SEMPHR_TRY_LOCK(g_wifi_manage_handle->mutex_lock);
	status = g_wifi_manage_handle->status;
	SEMPHR_TRY_UNLOCK(g_wifi_manage_handle->mutex_lock);
	//DEBUG_LOGE(LOG_TAG, "get_wifi_manage_status[%d]", status);
	
	return status;
}

/**
 * judge wheather the mode is wifi sta mode
 *
 * @param none
 * @return true/false
 */
static bool is_wifi_sta_mode(void)
{	
	int status = get_wifi_manage_status();

	if (status < 0)
	{
		return false;
	}
	
	if ((status&0xff00) == WIFI_MANAGE_STATUS_STA)
	{
		return true;
	}

	return false;
}

/**
 * judge wheather the mode is wifi AIRKISS mode
 *
 * @param none
 * @return true/false
 */
static bool is_wifi_airkiss_mode(void)
{	
	int status = get_wifi_manage_status();
	
	DEBUG_LOGE(LOG_TAG, "status %x ",status);

	if (status < 0)
	{
		return false;
	}
	
	if ((status&0xff00) == WIFI_MANAGE_STATUS_AIRKISS)
	{
		return true;
	}

	return false;
}

/**
 * start wifi sta mode
 *
 * @param none
 * @return true/false
 */
static bool start_wifi_sta_mode(void)
{
	if (g_wifi_manage_handle == NULL)
	{
		return false;
	}

	g_wifi_manage_handle->wifi_handler->disconnect();
		
	return true;
}


/**
 * wifi connect
 *
 * @param [in]wifi_info,device wifi infomation
 * @return true/false
 */
bool wifi_connect(const DEVICE_WIFI_INFO_T *wifi_info)
{
	if (g_wifi_manage_handle == NULL)
	{
		DEBUG_LOGE(LOG_TAG, "g_wifi_manage_handle is NULL");
		return false;
	}

	g_wifi_manage_handle->wifi_handler->disconnect();
	int ret = g_wifi_manage_handle->wifi_handler->connect(wifi_info->wifi_ssid, wifi_info->wifi_passwd, NULL, NSAPI_SECURITY_NONE);
    if (ret == 0) 
	{
        DEBUG_LOGI(LOG_TAG, "connect to %s success, ip %s", wifi_info->wifi_ssid, g_wifi_manage_handle->wifi_handler->get_ip_address());
		return true;
    } 
	else 
	{
        DEBUG_LOGE(LOG_TAG, "connect to %s failed", wifi_info->wifi_ssid);
		return false;
    }
}

/**
 * save wifi infomation
 *
 * @param [in]ssid,the name of wifi
 * @param [in]password
 * @return true/false
 */
static bool save_wifi_info(
	const char *ssid, 
	const char *password)
{
	int i = 0;
	int is_find_wifi = 0;
	int ret = WIFI_MANAGE_ERRNO_FAIL;
	DEVICE_WIFI_INFOS_T wifi_infos;
	DEVICE_WIFI_INFO_T *wifi_info = NULL;

	if (ssid == NULL 
		|| password == NULL
		|| strlen(ssid) <= 0)
	{
		DEBUG_LOGE(LOG_TAG, "save_wifi_info invalid params");
		return false;
	}
	
	memset(&wifi_infos, 0, sizeof(wifi_infos));
	bool err = get_flash_cfg(FLASH_CFG_WIFI_INFO, &wifi_infos);
	if (!err)
	{
		DEBUG_LOGE(LOG_TAG, "save_wifi_info get_flash_cfg failed");
		return WIFI_MANAGE_ERRNO_FAIL;
	}

	print_wifi_infos(&wifi_infos);

	for (i=1; i < MAX_WIFI_NUM; i++)
	{
		wifi_info = &wifi_infos.wifi_info[i];
		if (strncmp(ssid, wifi_info->wifi_ssid, sizeof(wifi_info->wifi_ssid)) == 0
			&& strncmp(password, wifi_info->wifi_passwd, sizeof(wifi_info->wifi_passwd)) == 0)
		{
			wifi_infos.wifi_connect_index = i;
			is_find_wifi = 1;
			break;
		}
	}

	if (is_find_wifi == 0)
	{
		if (wifi_infos.wifi_storage_index < MAX_WIFI_NUM)
		{
			if (wifi_infos.wifi_storage_index == (MAX_WIFI_NUM - 1))
			{
				wifi_infos.wifi_storage_index = 1;
				wifi_infos.wifi_connect_index = 1;
				wifi_info = &wifi_infos.wifi_info[1];
			}
			else
			{
				wifi_infos.wifi_storage_index++;
				wifi_infos.wifi_connect_index = wifi_infos.wifi_storage_index;
				wifi_info = &wifi_infos.wifi_info[wifi_infos.wifi_storage_index];
			}
		}
		else
		{
			wifi_infos.wifi_storage_index = 1;
			wifi_infos.wifi_connect_index = 1;
			wifi_info = &wifi_infos.wifi_info[1];
		}

		snprintf((char*)&wifi_info->wifi_ssid, sizeof(wifi_info->wifi_ssid), "%s", ssid);
		snprintf((char*)&wifi_info->wifi_passwd, sizeof(wifi_info->wifi_passwd), "%s", password);
	}
	
	print_wifi_infos(&wifi_infos);
	
	err = set_flash_cfg(FLASH_CFG_WIFI_INFO, &wifi_infos);
	if (!err)
	{
		DEBUG_LOGE(LOG_TAG, "save_wifi_info set_flash_cfg failed(%x)", ret);
		return WIFI_MANAGE_ERRNO_FAIL;
	}

	return WIFI_MANAGE_ERRNO_OK;
}

/**
 * get wifi infomation
 *
 * @param [out]handle,wifi manage handle
 * @return int
 */
static int get_wifi_info(WIFI_MANAGE_HANDLE_t *handle)
{
	int i = 0;
	static int connect_count = 0;
	int ret = WIFI_MANAGE_ERRNO_FAIL;
	DEVICE_WIFI_INFO_T *wifi_info = NULL;

	if (handle == NULL)
	{
		DEBUG_LOGE(LOG_TAG, "get_wifi_info invalid params");
		return WIFI_MANAGE_ERRNO_FAIL;
	}
	
	memset(&handle->wifi_infos, 0, sizeof(handle->wifi_infos));
	bool err = get_flash_cfg(FLASH_CFG_WIFI_INFO, &handle->wifi_infos);
	if (!err)
	{
		DEBUG_LOGE(LOG_TAG, "get_flash_cfg FLASH_CFG_WIFI_INFO failed");
		return WIFI_MANAGE_ERRNO_FAIL;
	}

	print_wifi_infos(&handle->wifi_infos);

	for (i=0; i<MAX_WIFI_NUM; i++)
	{
		if (handle->connect_index == -1)
		{
			handle->connect_index = handle->wifi_infos.wifi_connect_index;
			if (handle->connect_index < 0 
				|| handle->connect_index > MAX_WIFI_NUM)
			{
				handle->connect_index = 0;
			}
			connect_count = 0;
		}
		else
		{
			handle->connect_index++;
			if (handle->connect_index >= MAX_WIFI_NUM)
			{
				handle->connect_index = 0;
			}
		}

		connect_count++;
		if (connect_count > MAX_WIFI_NUM)
		{
			connect_count = 0;
			return WIFI_MANAGE_ERRNO_FAIL;
		}
		
		if (handle->connect_index >= 0 
			&& handle->connect_index < MAX_WIFI_NUM)
		{
			wifi_info = &handle->wifi_infos.wifi_info[handle->connect_index];
			if (strlen(wifi_info->wifi_ssid) == 0)
			{
				continue;
			}

			update_current_wifi_info(wifi_info->wifi_ssid, wifi_info->wifi_passwd);
			break;
		}
	}
	
	return WIFI_MANAGE_ERRNO_OK;
}

/**
 * wifi event process
 *
 * @param [out]handle,wifi manage handle
 * @return none
 */

bool bIsConnectedOnce = false;

bool is_wifi_connected()
{
	return (g_wifi_manage_handle->status == WIFI_MANAGE_STATUS_STA_CONNECTED);
}

#define AIRKISS_TIMEOUT 1000*51

static int nCount = 0;
const int nCountTotal = 3;

namespace deepbrain{
extern void yt_dcl_start();
extern void RegistWifi();
extern void RegistRec();
void net_connected(bool b_connect);
}
void AirkissTimeOut(void const *argument)
{
	DEBUG_LOGI(LOG_TAG, "AirkissTimeOut");	
#ifdef AIRKISS		
	rda5981_stop_airkiss();	
#else
	Airkiss::Instance().AirkissStop();
#endif
	{
		DEBUG_LOGI(LOG_TAG, "YT_DB_WIFI_AIRKISS_NOT_COMALETE");
		duer::YTMediaManager::instance().play_data(YT_DB_WIFI_AIRKISS_NOT_COMALETE, sizeof(YT_DB_WIFI_AIRKISS_NOT_COMALETE), duer::MEDIA_FLAG_PROMPT_TONE | duer::MEDIA_FLAG_SAVE_PREVIOUS);
		set_wifi_manage_status(WIFI_MANAGE_STATUS_IDLE);
		deepbrain::yt_dcl_start();
		
		task_thread_sleep(5000);
		deepbrain::net_connected(false);

		return ;	
	}
}

rtos::RtosTimer rtAirkiss(AirkissTimeOut,osTimerOnce,NULL);

#ifndef AIRKISS
class AirkissEvent:public Airkiss::IOnEvent
{
	virtual int on_airkiss_wait()
	{
	
	}
	virtual int on_airkiss_connect()
	{
	
	}
	virtual int on_airkiss_finish(char * strSSID,char * strPWD)
	{
		DEBUG_LOGI(LOG_TAG, "start_wifi_airkiss_mode success");	
		memcpy(g_wifi_manage_handle->curr_wifi.wifi_ssid,strSSID,strlen(strSSID));
		memcpy(g_wifi_manage_handle->curr_wifi.wifi_passwd,strPWD,strlen(strPWD));
		duer::YTMediaManager::instance().stop();
		rtAirkiss.stop();
		set_wifi_manage_status(WIFI_MANAGE_STATUS_AIRKISS_CONNECTING);
	}
};

AirkissEvent *pEvent =NULL;

#endif


static void wifi_event_process(WIFI_MANAGE_HANDLE_t *handle)
{
	if (handle == NULL)
	{
		return;
	}

	switch(handle->status)
    {
    	case WIFI_MANAGE_STATUS_IDLE:
    	{
			break;
    	}
		//联网模式
		case WIFI_MANAGE_STATUS_STA_ON:
		{					
			//audio_play_tone_mem(FLASH_MUSIC_NETWORK_CONNECTING, AUDIO_TERM_TYPE_DONE);
			if (start_wifi_sta_mode())
			{
				DEBUG_LOGI(LOG_TAG, "start wifi sta mode success");
				set_wifi_manage_status(WIFI_MANAGE_STATUS_STA_CONNECTING);
				handle->connect_index = -1;
			}
			else
			{
				DEBUG_LOGE(LOG_TAG, "start wifi sta mode failed");
				task_thread_sleep(5000);
			}
            break;
        }
        case WIFI_MANAGE_STATUS_STA_CONNECTING:
        {		
			//没有wifi信息，则获取wifi配置信息
			if (get_wifi_info(handle) == WIFI_MANAGE_ERRNO_FAIL)
			{
				set_wifi_manage_status(WIFI_MANAGE_STATUS_AIRKISS_ON);
				break;
			}
			
			//连接wifi
			duer::YTMediaManager::instance().play_data(YT_DB_WIFI_CONNECTING_TONE, sizeof(YT_DB_WIFI_CONNECTING_TONE), duer::MEDIA_FLAG_PROMPT_TONE);	
            if (wifi_connect(&handle->curr_wifi))
            {	        
            	handle->connecting_start_time = get_time_of_day();
				set_wifi_manage_status(WIFI_MANAGE_STATUS_STA_CONNECT_SUCCESS);
				DEBUG_LOGI(LOG_TAG, "connecting wifi [%s]:[%s]", 
					handle->curr_wifi.wifi_ssid, handle->curr_wifi.wifi_passwd);
            }
			else
			{	
				DEBUG_LOGI(LOG_TAG, "connecting wifi [%s]:[%s] failed", 
					handle->curr_wifi.wifi_ssid, handle->curr_wifi.wifi_passwd);
			}
            break;
        }
		case WIFI_MANAGE_STATUS_STA_CONNECT_WAIT:
		{
			if (labs(get_time_of_day() - handle->connecting_start_time) >= 8000)
			{
				set_wifi_manage_status(WIFI_MANAGE_STATUS_STA_CONNECT_FAIL);
			}
			break;
		}
    	case WIFI_MANAGE_STATUS_STA_CONNECT_SUCCESS:
        {  
			app_send_message(APP_NAME_WIFI_MANAGE, APP_MSG_TO_ALL, APP_EVENT_WIFI_CONNECTED, NULL, 0);
			save_wifi_info(handle->curr_wifi.wifi_ssid, handle->curr_wifi.wifi_passwd);
			set_wifi_manage_status(WIFI_MANAGE_STATUS_STA_CONNECTED);
			//audio_play_tone_mem(FLASH_MUSIC_NETWORK_CONNECT_SUCCESS, AUDIO_TERM_TYPE_DONE);
			duer::YTMediaManager::instance().play_data(YT_DB_WIFI_SUCCESS, sizeof(YT_DB_WIFI_SUCCESS), duer::MEDIA_FLAG_PROMPT_TONE | duer::MEDIA_FLAG_SAVE_PREVIOUS);	
			handle->connect_index = -1;
			DEBUG_LOGI(LOG_TAG, "WIFI_MANAGE_STATUS_STA_CONNECT_SUCCESS");
			bIsConnectedOnce = true;
			rtAirkiss.stop();

			deepbrain::net_connected(true);
            break;
        }
		case WIFI_MANAGE_STATUS_STA_CONNECT_FAIL:
		{
		#if 0
			duer::YTMediaManager::instance().play_data(YT_DB_WIFI_FAIL, sizeof(YT_DB_WIFI_FAIL), duer::MEDIA_FLAG_PROMPT_TONE | duer::MEDIA_FLAG_SAVE_PREVIOUS);	
		#endif
			//audio_play_tone_mem(FLASH_MUSIC_NETWORK_CONNEC_FAILURE, TERMINATION_TYPE_NOW);	
			//set_wifi_manage_status(WIFI_MANAGE_STATUS_STA_CONNECTING);
			set_wifi_manage_status(WIFI_MANAGE_STATUS_AIRKISS_ON);
			break;
		}
		case WIFI_MANAGE_STATUS_STA_CONNECTED:
		{				
			unsigned int msg;
			rda_msg_get(handle->wifi_msg_queue, &msg, 100);
			switch(msg)
			{
				case MAIN_RECONNECT:
					DEBUG_LOGI(LOG_TAG, "rda_msg_get reconnect");
					g_wifi_manage_handle->wifi_handler->disconnect();
					set_wifi_manage_status(WIFI_MANAGE_STATUS_STA_DISCONNECTED);
					break;
			
				default:
					//printf("unknown msg\r\n");
					break;
			}
			break;
		}
		case WIFI_MANAGE_STATUS_STA_DISCONNECTED:
		{
			duer::YTMediaManager::instance().play_data(YT_DB_WIFI_DISCONNECT, sizeof(YT_DB_WIFI_DISCONNECT), duer::MEDIA_FLAG_PROMPT_TONE | duer::MEDIA_FLAG_SAVE_PREVIOUS);				
			task_thread_sleep(3000);
			//audio_play_tone_mem(FLASH_MUSIC_NETWORK_DISCONNECTED, AUDIO_TERM_TYPE_NOW);
			set_wifi_manage_status(WIFI_MANAGE_STATUS_STA_ON);
			break;
		}
		//AIRKISS配网
		case WIFI_MANAGE_STATUS_AIRKISS_ON:
		{

#if 0
			duer::YTMediaManager::instance().play_data(YT_DB_WIFI_AIRKISS, sizeof(YT_DB_WIFI_AIRKISS), duer::MEDIA_FLAG_PROMPT_TONE | duer::MEDIA_FLAG_SAVE_PREVIOUS);	
			g_wifi_manage_handle->wifi_handler->disconnect();
			app_send_message(APP_NAME_WIFI_MANAGE, APP_MSG_TO_ALL, APP_EVENT_WIFI_DISCONNECTED, NULL, 0);
			task_thread_sleep(5000);			
			duer::YTMediaManager::instance().play_data(YT_DB_WIFI_CONNECTING_TONE_LONG, sizeof(YT_DB_WIFI_CONNECTING_TONE_LONG), duer::MEDIA_FLAG_PROMPT_TONE);	
#else		
			app_send_message(APP_NAME_WIFI_MANAGE, APP_MSG_TO_ALL, APP_EVENT_WIFI_DISCONNECTED, NULL, 0);
			task_thread_sleep(1000);
			DEBUG_LOGI(LOG_TAG, "wifi_handler:disconnect");
			g_wifi_manage_handle->wifi_handler->disconnect();			
			duer::YTMediaManager::instance().play_data(YT_DB_WIFI_CONNECTING_TONE_LONG, sizeof(YT_DB_WIFI_CONNECTING_TONE_LONG), duer::MEDIA_FLAG_PROMPT_TONE);	
#endif

			memset(&g_wifi_manage_handle->curr_wifi, 0x00, sizeof(DEVICE_WIFI_INFO_T));

			rtAirkiss.stop();
			task_thread_sleep(100);
			DEBUG_LOGI(LOG_TAG, "get_ssid_pw_from_airkiss");
			rtAirkiss.start(AIRKISS_TIMEOUT);

			deepbrain::RegistWifi();
			deepbrain::RegistRec();
		#ifdef AIRKISS	
			if (get_ssid_pw_from_airkiss(g_wifi_manage_handle->curr_wifi.wifi_ssid,g_wifi_manage_handle->curr_wifi.wifi_passwd) == 0)
			{				
				DEBUG_LOGI(LOG_TAG, "start_wifi_airkiss_mode success");	
				duer::YTMediaManager::instance().stop();
				rtAirkiss.stop();
				
				set_wifi_manage_status(WIFI_MANAGE_STATUS_AIRKISS_CONNECTING);
			}
			else
			{				
				DEBUG_LOGE(LOG_TAG, "start_wifi_airkiss_mode failed");

				rtAirkiss.stop();
				if(WIFI_MANAGE_STATUS_AIRKISS_ON == get_wifi_manage_status())
				set_wifi_manage_status(WIFI_MANAGE_STATUS_IDLE);
				break;
				set_wifi_manage_status(WIFI_MANAGE_STATUS_IDLE);
			}
		#else
			Airkiss::Instance().AirkissStart();
		#endif
		
			break;
		}
		case WIFI_MANAGE_STATUS_AIRKISS_CONNECTING:
		{
			//连接wifi
            if ((strlen(handle->curr_wifi.wifi_ssid) > 0) &&
				wifi_connect(&handle->curr_wifi))
            {	        
            	handle->connecting_start_time = get_time_of_day();	
			#ifdef AIRKISS	
				airkiss_sendrsp_to_host(g_wifi_manage_handle->wifi_handler);
			#else
				Airkiss::Instance().AirkissFinish();
			#endif
				set_wifi_manage_status(WIFI_MANAGE_STATUS_AIRKISS_CONNECT_SUCCESS);
				DEBUG_LOGI(LOG_TAG, "connecting wifi [%s]:[%s]", 
					handle->curr_wifi.wifi_ssid, handle->curr_wifi.wifi_passwd);
            }
			else
			{	
				set_wifi_manage_status(WIFI_MANAGE_STATUS_AIRKISS_CONNECT_FAIL);
				DEBUG_LOGI(LOG_TAG, "connecting wifi [%s]:[%s] failed", 
					handle->curr_wifi.wifi_ssid, handle->curr_wifi.wifi_passwd);
			}
			break;
		}
		case WIFI_MANAGE_STATUS_AIRKISS_CONNECT_WAIT:
		{
			if (labs(get_time_of_day() - handle->connecting_start_time) >= 5000)
			{
				set_wifi_manage_status(WIFI_MANAGE_STATUS_AIRKISS_CONNECT_FAIL);
			}
			break;
		}
		case WIFI_MANAGE_STATUS_AIRKISS_CONNECT_SUCCESS:
		{	    
			app_send_message(APP_NAME_WIFI_MANAGE, APP_MSG_TO_ALL, APP_EVENT_WIFI_CONNECTED, NULL, 0);
			save_wifi_info(handle->curr_wifi.wifi_ssid, handle->curr_wifi.wifi_passwd);
			set_wifi_manage_status(WIFI_MANAGE_STATUS_STA_CONNECTED);

			bIsConnectedOnce = true;
			rtAirkiss.stop();

			duer::YTMediaManager::instance().play_data(YT_DB_WIFI_SUCCESS, sizeof(YT_DB_WIFI_SUCCESS), duer::MEDIA_FLAG_PROMPT_TONE | duer::MEDIA_FLAG_SAVE_PREVIOUS);	
			//audio_play_tone_mem(FLASH_MUSIC_NETWORK_CONNECT_SUCCESS, AUDIO_TERM_TYPE_DONE);

			deepbrain::net_connected(true);
            break;
        }
		case WIFI_MANAGE_STATUS_AIRKISS_CONNECT_FAIL:
		{
			duer::YTMediaManager::instance().play_data(YT_DB_WIFI_FAIL, sizeof(YT_DB_WIFI_FAIL), duer::MEDIA_FLAG_PROMPT_TONE | duer::MEDIA_FLAG_SAVE_PREVIOUS);	
			
			//audio_play_tone_mem(FLASH_MUSIC_NETWORK_CONNECT_FAILURE, AUDIO_TERM_TYPE_NOW);
			task_thread_sleep(2000);
			set_wifi_manage_status(WIFI_MANAGE_STATUS_AIRKISS_ON);
			break;
		}
    	default:
        	break;

    }	
}

static void wifi_event_callback(
	void *app_handler, APP_EVENT_MSG_t *msg)
{
	APP_OBJECT_t *app = (APP_OBJECT_t *)app_handler;

	int err = 0;
	WIFI_MANAGE_HANDLE_t *handle = g_wifi_manage_handle;

	switch (msg->event)
	{
		case APP_EVENT_WIFI_CONNECTED:
		{
			set_wifi_manage_status(WIFI_MANAGE_STATUS_STA_CONNECT_SUCCESS);			
			app_send_message(APP_NAME_WIFI_MANAGE, APP_MSG_TO_ALL, APP_EVENT_WIFI_CONNECTED, NULL, 0);
			break;
		}
		case APP_EVENT_WIFI_CONNECT_FAIL:
		{
			set_wifi_manage_status(WIFI_MANAGE_STATUS_STA_CONNECT_FAIL);
			app_send_message(APP_NAME_WIFI_MANAGE, APP_MSG_TO_ALL, APP_EVENT_WIFI_CONNECT_FAIL, NULL, 0);
			break;
		}
		case APP_EVENT_WIFI_DISCONNECTED:
		{
			if (get_wifi_manage_status() == WIFI_MANAGE_STATUS_STA_CONNECTED)
			{
				set_wifi_manage_status(WIFI_MANAGE_STATUS_STA_DISCONNECTED);
			}
			else if (get_wifi_manage_status() == WIFI_MANAGE_STATUS_STA_CONNECT_WAIT)
			{
				set_wifi_manage_status(WIFI_MANAGE_STATUS_STA_CONNECT_FAIL);
			}
			
			app_send_message(APP_NAME_WIFI_MANAGE, APP_MSG_TO_ALL, APP_EVENT_WIFI_DISCONNECTED, NULL, 0);
			break;
		}
		case APP_EVENT_DEFAULT_LOOP_TIMEOUT:
		{			
			wifi_event_process(handle);
			break;
		}
		case APP_EVENT_DEFAULT_EXIT:
		{
			app_exit(app);
			break;
		}
		default:
			break;
	}

}

static void task_wifi_manage(void)
{	
	APP_OBJECT_t *app = NULL;
	
	app = app_new(APP_NAME_WIFI_MANAGE);
	if (app == NULL)
	{
		DEBUG_LOGE(LOG_TAG, "new app[%s] failed, out of memory", APP_NAME_WIFI_MANAGE);
	}
	else
	{
		app_set_loop_timeout(app, 200, wifi_event_callback);
		app_add_event(app, APP_EVENT_DEFAULT_BASE, wifi_event_callback);
		app_add_event(app, APP_EVENT_AUDIO_PLAYER_BASE, wifi_event_callback);
		app_add_event(app, APP_EVENT_WIFI_BASE, wifi_event_callback);
		app_add_event(app, APP_EVENT_POWER_BASE, wifi_event_callback);
		DEBUG_LOGI(LOG_TAG, "%s create success", APP_NAME_WIFI_MANAGE);
	}
	
	app_msg_dispatch(app);
	
	app_delete(app);

	wifi_manage_destory();

	task_thread_exit();

	DEBUG_LOGI(LOG_TAG, "task_wifi_manage app_exit");
}

APP_FRAMEWORK_ERRNO_t wifi_manage_create(const int task_priority)
{
	if (g_wifi_manage_handle != NULL)
	{
		DEBUG_LOGE(LOG_TAG, "g_wifi_manage_handle already exist");
		return APP_FRAMEWORK_ERRNO_FAIL;
	}

	g_wifi_manage_handle = (WIFI_MANAGE_HANDLE_t*)memory_malloc(sizeof(WIFI_MANAGE_HANDLE_t));
	if (g_wifi_manage_handle == NULL)
	{
		DEBUG_LOGE(LOG_TAG, "mpush_service_create failed");
		return APP_FRAMEWORK_ERRNO_MALLOC_FAILED;
	}
	
	memset(g_wifi_manage_handle, 0, sizeof(WIFI_MANAGE_HANDLE_t));
	g_wifi_manage_handle->status = WIFI_MANAGE_STATUS_STA_ON;
	g_wifi_manage_handle->connect_index = -1;
	g_wifi_manage_handle->wifi_msg_queue = rda_msgQ_create(5);
	g_wifi_manage_handle->wifi_handler = &g_wifi_handler;


	g_wifi_manage_handle->wifi_handler->init();
	g_wifi_manage_handle->wifi_handler->set_msg_queue(g_wifi_manage_handle->wifi_msg_queue);

#ifndef AIRKISS
	Airkiss::Instance().Init(g_wifi_manage_handle->wifi_handler);
	if(NULL == pEvent)pEvent = new AirkissEvent;
	Airkiss::Instance().SetEventListener(pEvent);
#endif

	SEMPHR_CREATE_LOCK(g_wifi_manage_handle->mutex_lock);

	if (!( g_thWifi =(rtos::Thread *) task_thread_create(task_wifi_manage,
                    "task_wifi_manage",
                    APP_NAME_WIFI_MANAGE_STACK_SIZE,
                    g_wifi_manage_handle,
                    task_priority))) 
    {
        DEBUG_LOGE(LOG_TAG, "ERROR creating task_wifi_manage task! Out of memory?");

		SEMPHR_DELETE_LOCK(g_wifi_manage_handle->mutex_lock);
		memory_free(g_wifi_manage_handle);
		g_wifi_manage_handle = NULL;
		
		return APP_FRAMEWORK_ERRNO_FAIL;
    }

	return APP_FRAMEWORK_ERRNO_OK;
}

APP_FRAMEWORK_ERRNO_t wifi_manage_delete(void)
{
	//return app_send_message(APP_NAME_WIFI_MANAGE, APP_NAME_WIFI_MANAGE, APP_EVENT_DEFAULT_EXIT, NULL, 0);
	app_send_message(APP_NAME_WIFI_MANAGE, APP_NAME_WIFI_MANAGE, APP_EVENT_DEFAULT_EXIT, NULL, 0);
	while(g_thWifi->get_state() != rtos::Thread::Deleted){rtos::Thread::wait(10);}
	task_thread_exit(g_thWifi);

}

APP_FRAMEWORK_ERRNO_t wifi_manage_destory(void)
{
	SEMPHR_DELETE_LOCK(g_wifi_manage_handle->mutex_lock);
	memory_free(g_wifi_manage_handle);
	g_wifi_manage_handle = NULL;	
}

APP_FRAMEWORK_ERRNO_t wifi_manage_start_airkiss(void)
{
	printf("wifi_manage_start_airkiss\r\n");
	
	if (is_wifi_airkiss_mode())
	{
		DEBUG_LOGI(TAG_LOG,"is_wifi_airkiss_mode\r\n");
	#ifdef AIRKISS	
		rda5981_stop_airkiss();	
	#else
		Airkiss::Instance().AirkissStop();
	#endif
		//duer::YTMediaManager::instance().play_data(YT_AIRKISS_OUT, sizeof(YT_AIRKISS_OUT), duer::MEDIA_FLAG_PROMPT_TONE | duer::MEDIA_FLAG_SAVE_PREVIOUS);	
		
		duer::YTMediaManager::instance().play_data(YT_CONNTING_NET, sizeof(YT_CONNTING_NET), duer::MEDIA_FLAG_PROMPT_TONE | duer::MEDIA_FLAG_SAVE_PREVIOUS);	

		set_wifi_manage_status(WIFI_MANAGE_STATUS_STA_CONNECTING);	
	}
	else
	{		
		DEBUG_LOGI(TAG_LOG,"wifi_manage_start_airkiss\r\n");
		set_wifi_manage_status(WIFI_MANAGE_STATUS_AIRKISS_ON);
	}
}

