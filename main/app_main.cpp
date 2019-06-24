/*
 * Copyright 2017-2018 deepbrain.ai, Inc. or its affiliates. All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License").
 * You may not use this file except in compliance with the License.
 * A copy of the License is located at
 *
 *     http://deepbrain.ai/apache2.0/
 *
 * or in the "license" file accompanying this file. This file is distributed
 * on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either
 * express or implied. See the License for the specific language governing
 * permissions and limitations under the License.
 */

#include "mbed.h"
#include "baidu_media_manager.h"
#include "baidu_media_play.h"
#include "rtos.h"
#include "events.h"
#include "duer_rda_flash.h"
#include "Wland_flash.h"
#include "wland_rf.h"
#include "audio.h"
#include "YTLight.h"
#include "yt_key.h"
#include "YTManage.h"

#include "Lightduer_adapter.h"
#include "userconfig.h"
#include "app_config.h"
#include "app_event.h"
#include "app_framework.h"
#include "time_interface.h"
#include "memory_interface.h"
#include "task_thread_interface.h"
#include "debug_log_interface.h"

#include "uart_shell.h"
#include "device_params_manage.h"
#include "wifi_manage.h"
#include "mpush_service.h"
#include "airkiss_lan_discovery.h"
#include "asr_service.h"
#include "deepbrain_app.h"
#include "ota_service.h"
#include "authorize.h"
#include "vbat.h"
#include "Rda_sleep_api.h"
#include "SDMMCFileSystem.h"



#include "ytawake.h"

#define LOG_TAG "main"


/// ¿ª·¢°å
//SDMMCFileSystem sd(PB_9, PB_0, PB_6, PB_7, PC_0, PC_1, "sd");/// ×¢Òâ PB_6 Óëvbat adcpin0 ³åÍ»
SDMMCFileSystem sd(NC, NC, NC, NC, NC, NC, "sd");

//SDMMCFileSystem sd(GPIO_PIN9, GPIO_PIN0, GPIO_PIN3, NC, NC, NC, "sd");

APP_OBJECT_t *g_app_main = NULL;

static void task_app_main(void)
{
	APP_OBJECT_t *app_main = g_app_main;
		
	app_msg_dispatch(app_main);
	app_delete(app_main);
	task_thread_exit();
}

static APP_FRAMEWORK_ERRNO_t app_main_create(int32_t task_priority)
{
	
	PLATFORM_API_t api = {memory_malloc, memory_free, task_thread_sleep, get_time_of_day};

	//åˆ›å»ºAPP MAIN
	app_set_extern_functions(&api);
	while(1)
	{
		g_app_main = app_new(APP_NAME_MAIN);
		if (g_app_main == NULL)
		{
			DEBUG_LOGE(LOG_TAG, "new app main failed, out of memory");
			continue;
		}
		else
		{
			DEBUG_LOGI(LOG_TAG, "new app main success");
		}
		break;
	}

	if (!task_thread_create(task_app_main, 
			"task_app_main", 
			APP_NAME_MAIN_STACK_SIZE, 
			g_app_main, 
			task_priority))
	{
		DEBUG_LOGE(LOG_TAG, "task_thread_create task_app_main failed");
		return APP_FRAMEWORK_ERRNO_FAIL;
	}

	return APP_FRAMEWORK_ERRNO_OK;
}

void* get_network_interface(void)
{
    return get_rda_network_interface();
}




int main() 
{
	DEBUG_LOGI(LOG_TAG, "main");
#if defined(TARGET_UNO_91H)	
    rda5981_set_flash_size(RDA_FLASH_SIZE);
    rda5981_set_user_data_addr(RDA_SYS_DATA_ADDR, RDA_USER_DATA_ADDR, RDA_USER_DATA_LENG);

	baidu_ca_adapter_initialize();
#endif

	DEBUG_LOGI(LOG_TAG, 
		"\r\n====================================\r\n\t"
		"RDA 5981 Wifi Model\r\n"
		"====================================\r\n");


/////// add nothing

	//init global clock
	init_timer_clock();

	//init flash params
	init_device_params();
	
    duer::MediaManager::instance().initialize();
	duer::YTMediaManager::instance().init();

    duer::YTMediaManager::instance().set_volume(duer::DEFAULT_VOLUME/*10*/);	

#if 0//chenjl add 20190317
	if(vbat_check_startup())
	{
		duer::event_loop();
	}	
#endif

#if 1//add by lijun	20190311
	if(deepbrain::auto_test())
	{
		duer::event_loop();
	}
#endif

#if 1
	duer::YTMediaManager::instance().play_data(YT_DB_WELCOME, sizeof(YT_DB_WELCOME), duer::MEDIA_FLAG_PROMPT_TONE | duer::MEDIA_FLAG_SAVE_PREVIOUS);	
#else
	duer::YTMediaManager::instance().play_data(YT_DB_WELCOME_PHC, sizeof(YT_DB_WELCOME_PHC), duer::MEDIA_FLAG_PROMPT_TONE | duer::MEDIA_FLAG_SAVE_PREVIOUS);		
#endif
	while(duer::YTMediaManager::instance().is_playing())//zhutong 20190317		
	{		
		wait_ms(50);	
	}

#if 0
	duer::YTMediaManager::instance().set_volume(duer::DEFAULT_VOLUME);
#endif
		
	deepbrain::yt_dcl_init();


//memory_info();

	app_main_create(TASK_PRIORITY_1);

	//uart shell
	//uart_shell_create(TASK_PRIORITY_1);

	//wifi manage
	wifi_manage_create(TASK_PRIORITY_1);

	/// need ytawake
	//yt_awake_create(TASK_PRIORITY_1); 


	//asr service create
	asr_service_create(TASK_PRIORITY_1);
		
	//power manage
	vbat_start();

	//authorize
	authorize_service_create(TASK_PRIORITY_1);

	//mpush
	mpush_service_create(TASK_PRIORITY_1);

	//17.OTA service
	//ota_service_create(TASK_PRIORITY_2);

	//20.memo è®¾ç½®
	//memo_service_create(TASK_PRIORITY_1);

	//airkiss lan discovery
	airkiss_lan_discovery_create(TASK_PRIORITY_1);	


#if 0
memory_info();
	
	rtos::Thread::wait(100);
	airkiss_lan_discovery_delete();
	asr_service_delete();
	mpush_service_delete();  
	authorize_service_delete();
	
memory_info();	
#endif	

	DEBUG_LOGI(LOG_TAG, "----entry event_loop----");
    duer::event_loop();

	//while(1){}
}


