#include <time.h>
#include "memo_service.h"
#include "cJSON.h"
#include "debug_log_interface.h"
#include "userconfig.h"
#include "device_params_manage.h"
#include "auth_crypto.h"
#include "http_api.h"
#include "asr_service.h"
#include "YTManage.h"
#include "dcl_update_time.h"

#define PRINT_TAG     "MEMO CLIENT"

//attrName
#define MEMO_TIME    "备忘时间"
#define MEMO_EVENT   "备忘事件"
#define EXECUTETIME  "executeTimeParseSecond"
#define EVENTCONTENT "eventContent"
#define EVENTTYPE    "eventType"

//attrValue
#define OUTDATE      "outDateError"
#define REMINDMEMO   "remindMemo"

static MEMO_STATUS_T memo_status = MEMO_IDLE;
static MEMO_ARRAY_T *memo_array = NULL;
static MEMO_EVENT_T *cur_evt = NULL;
static rtos::Mutex s_cur_evt_mutex;

static rtos::Thread *g_thMemo = NULL;

static void audio_play_tts(char *tts)
{
	char *tts_url = (char*)memory_malloc(1024);
	int ret = get_tts_play_url(tts, tts_url, 1024);
	if (!ret)
	{
		ret = get_tts_play_url(tts, tts_url, 1024);
	}
	
	if (ret)
	{		
		duer::YTMediaManager::instance().play_url(tts_url, duer::MEDIA_FLAG_SPEECH | duer::MEDIA_FLAG_SAVE_PREVIOUS);
	}
	else
	{
		DEBUG_LOGE(LOG_TAG, "memo get_tts_play_url failed");
	}

	if(tts_url)
		memory_free(tts_url);
}

//初始化时钟
static uint32_t init_clock()
{
	uint32_t time_now = 0;
	DCL_AUTH_PARAMS_t dcl_auth_params = {0};
	get_dcl_auth_params(&dcl_auth_params);
	
	if (!get_dcl_auth_params(&dcl_auth_params))
	{
		DEBUG_LOGE(PRINT_TAG, "get_dcl_auth_params failed"); 
		task_thread_sleep(3000);
		return;
	}

	if (dcl_update_time(&dcl_auth_params, &time_now) != DCL_ERROR_CODE_OK)
	{
		DEBUG_LOGE(PRINT_TAG, "dcl_update_time failed"); 
		task_thread_sleep(3000);
		return;
	}

	return time_now;
}

void show_memo()
{
	int num = 0;
	for(num = 0; num < MEMO_EVEVT_MAX; num++) 
	{
		if(memo_array->event[num].is_valid)
			DEBUG_LOGI(PRINT_TAG, "memo[%d] str:%s", memo_array->event[num].time ,memo_array->event[num].str); 
	}
}

//执行提醒
static void memo_event_handle(void *evt)
{
	s_cur_evt_mutex.lock();
	
	if(cur_evt == NULL)
		cur_evt = (MEMO_EVENT_T*)evt;	

	s_cur_evt_mutex.unlock();
}

static void start_one_memo(MEMO_EVENT_T *event,uint32_t time_now)
{
	if(!event->timer) {
		event->timer = 
			duer_timer_acquire(memo_event_handle, event, DUER_TIMER_ONCE);	
	}
	
	if (!event->timer) {
		DEBUG_LOGE(PRINT_TAG, "fail to creat memo timer"); 
		return;
	}	

    uint64_t delay = (event->time - time_now) * 1000;
	
    int rs = duer_timer_start(event->timer, delay);
    if (rs != DUER_OK) {
        DEBUG_LOGE(PRINT_TAG,"Failed to start timer\n");
		return;
    }

	DEBUG_LOGI(PRINT_TAG, "start_one_memo[%d] success!! str:%s",delay,event->str); 
	event->is_valid = true;
	event->set_time = time_now;
}

static void add_one_memo(MEMO_EVENT_T *event)
{
	DEBUG_LOGI(PRINT_TAG, "add_one_memo ");	

#ifdef __NOT_USE_FLSAH__
	int num = 0;
	for(num = 0; num < MEMO_EVEVT_MAX; num++) 
	{
		if(memo_array->event[num].is_valid == false)
		{
			memo_array->event[num].time = event->time;
			memcpy(memo_array->event[num].str,event->str,sizeof(memo_array->event[num].str));
			start_one_memo(&memo_array->event[num], 0);
			break;
		}
	}

	if(num == MEMO_EVEVT_MAX)
	{
		DEBUG_LOGI(PRINT_TAG, "memo full!!");
	}
#else
	uint32_t time_now = init_clock();
	if(time_now == 0)
	{
		DEBUG_LOGE(PRINT_TAG, "add memo fail time_now = 0!!");
		return;
	}
	
	int num, earliest_num = 0;
	uint32_t earliest_set_time = 0xffffffff;
	for(num = 0; num < MEMO_EVEVT_MAX; num++) 
	{
		if(memo_array->event[num].is_valid == false)
		{
			memo_array->event[num].time = time_now + event->time;
			memcpy(memo_array->event[num].str,event->str,sizeof(memo_array->event[num].str));
			start_one_memo(&memo_array->event[num], time_now);
			break;
		}else {
			if(memo_array->event[num].set_time < earliest_set_time)
			{
				earliest_set_time = memo_array->event[num].set_time;
				earliest_num = num;
			}
		}
	}

	if(num == MEMO_EVEVT_MAX)
	{
		DEBUG_LOGI(PRINT_TAG, "memo full!! clean earliest_memo earliest_num:%d",earliest_num);
		duer_timer_stop(memo_array->event[earliest_num].timer);

		memo_array->event[earliest_num].time = time_now + event->time;
		memcpy(memo_array->event[earliest_num].str,event->str,sizeof(memo_array->event[earliest_num].str));
		
		start_one_memo(&memo_array->event[earliest_num], time_now);
	}
		
	set_flash_cfg(FLASH_CFG_MEMO_PARAMS, memo_array);	
#endif

	show_memo();
}

#ifndef __NOT_USE_FLSAH__
//添加提醒
static int init_memo()
{
	uint32_t time_now = init_clock();
	if(time_now == 0)
		return false;

	int num = 0;
	get_flash_cfg(FLASH_CFG_MEMO_PARAMS, memo_array);

	for(num = 0; num < MEMO_EVEVT_MAX; num++) 
	{
		//清除过期备忘
		if (memo_array->event[num].time < time_now - 30 || !memo_array->event[num].is_valid || !memo_array->event[num].timer)
		{
			memset(&memo_array->event[num], 0x00, sizeof(MEMO_EVENT_T));
			continue;
		}
		else {
			memo_array->event[num].timer = NULL;			
			memo_array->event[num].is_valid = false;
			start_one_memo(&memo_array->event[num], time_now);
		}
	}

	 set_flash_cfg(FLASH_CFG_MEMO_PARAMS, memo_array);	 
	 show_memo();
}
#endif

static void memo_event_process()
{
	if(cur_evt == NULL)
		return;

	DEBUG_LOGI(PRINT_TAG, "memo_event!");
	s_cur_evt_mutex.lock();
	if(cur_evt != NULL) {
		audio_play_tts(cur_evt->str);

		int num = 0;
		for(num = 0; num < MEMO_EVEVT_MAX; num++) 
		{
			if (memo_array->event[num].timer == cur_evt->timer)
			{
				memo_array->event[num].is_valid = false;
				break;
			}
		}

		cur_evt = NULL;
	}
	s_cur_evt_mutex.unlock();
	
	set_flash_cfg(FLASH_CFG_MEMO_PARAMS, memo_array);	
}

static void memo_manage_callback(void *app_handler, APP_EVENT_MSG_t *msg)
{
	APP_OBJECT_t *app = (APP_OBJECT_t *)app_handler;

	switch (msg->event)
	{
		case APP_EVENT_DEFAULT_LOOP_TIMEOUT:
		{
			switch (memo_status)
			{
				case MEMO_IDLE:
				{					
					memo_status = MEMO_INIT;					
					break;
				}			
				case MEMO_INIT:
				{
				#ifdef __NOT_USE_FLSAH__
					memo_status = MEMO_LOOP;
				#else
					if(init_memo())				
						memo_status = MEMO_LOOP;
					else {
						memo_status = MEMO_IDLE;
						task_thread_sleep(30*1000);
					}
				#endif
					break;
				}		
				case MEMO_LOOP:
				{
					memo_event_process();
					break;
				}	
				default:
					break;
			}
			break;
		}
		case APP_EVENT_MEMO_CANCLE:
		{
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

static void task_memo_manage()
{
	APP_OBJECT_t *app = NULL;

	task_thread_sleep(8000);
	
	app = app_new(APP_NAME_MEMO_MANAGE);
	if (app == NULL)
	{
		DEBUG_LOGE(PRINT_TAG, "new app[%s] failed, out of memory", APP_NAME_MEMO_MANAGE);
		task_thread_exit();
	}
	else
	{
		app_set_loop_timeout(app, 1000, memo_manage_callback);
		app_add_event(app, APP_EVENT_MEMO_BASE, memo_manage_callback);
		DEBUG_LOGE(PRINT_TAG, "%s create success", APP_NAME_MEMO_MANAGE);
	}
	
	app_msg_dispatch(app);
	
	app_delete(app);
	
	if(memo_array)
		memory_free(memo_array);

	task_thread_exit(g_thMemo);

	DEBUG_LOGI(PRINT_TAG, "task_memo_manage app_exit");
}

int get_memo_result(char *str_body)
{
	int arry_size = 0;
	int memo_valid = 0;
	MEMO_EVENT_T tmp_event = {0};
	
	//DEBUG_LOGW(PRINT_TAG, "str_body [%s]", str_body);
	cJSON *pJson_body = cJSON_Parse(str_body);
	if (NULL == pJson_body) 
	{
		DEBUG_LOGE(PRINT_TAG, "str_body cJSON_Parse fail");
		goto get_memo_result_error;
	}

	//命令名称
	cJSON *pJson_cmd_data = cJSON_GetObjectItem(pJson_body, "commandData");
	if (NULL == pJson_cmd_data) 
	{
		DEBUG_LOGE(PRINT_TAG, "pJson_body, pJson_cmd_data not found\n");
		goto get_memo_result_error;
	}
	
	cJSON *pJson_cmd_name = cJSON_GetObjectItem(pJson_cmd_data, "commandName");
	if (NULL == pJson_cmd_name) 
	{
		DEBUG_LOGE(PRINT_TAG, "str_body_buf,commandName not found\n");
		goto get_memo_result_error;
	}			
	DEBUG_LOGE(PRINT_TAG, "pJson_cmd_name = %s", pJson_cmd_name->valuestring);

	//JSON数组
	cJSON *pJson_array = cJSON_GetObjectItem(pJson_cmd_data, "commandAttrs");
	if (NULL == pJson_array) 
	{
		DEBUG_LOGE(PRINT_TAG, "str_body_buf,pJson_array not found\n");
		goto get_memo_result_error;
	}

	//JSON数组大小
	arry_size = cJSON_GetArraySize(pJson_array);
	if (arry_size == 0)
	{
		DEBUG_LOGE(PRINT_TAG, "arry_size[%d], error", arry_size);
		goto get_memo_result_error;
	}

	int i = 0;
	for (i = 0; i < arry_size; i++)
	{
		//获取数组成员
		cJSON *pJson_array_item = cJSON_GetArrayItem(pJson_array, i);
		if (NULL == pJson_array_item) 
		{
			DEBUG_LOGE(PRINT_TAG, "pJson_array, pJson_array_item not found\n");
			goto get_memo_result_error;
		}

		//成员 attrName
		cJSON *pJson_array_attrName = cJSON_GetObjectItem(pJson_array_item, "attrName");
		if (NULL == pJson_array_attrName
			|| pJson_array_attrName->valuestring == NULL) 
		{
			DEBUG_LOGE(PRINT_TAG, "pJson_array_item, pJson_array_attrName not found\n");
			goto get_memo_result_error;
		}

		//成员 attrValue
		cJSON *pJson_array_attrValue = cJSON_GetObjectItem(pJson_array_item, "attrValue");
		if (NULL == pJson_array_attrValue
			|| pJson_array_attrValue->valuestring == NULL) 
		{
			DEBUG_LOGE(PRINT_TAG, "pJson_array_item,pJson_array_attrValue not found\n");
			goto get_memo_result_error;
		}
		
		DEBUG_LOGE(PRINT_TAG, "pJson_array_attrName = %s", pJson_array_attrName->valuestring);
		DEBUG_LOGE(PRINT_TAG, "pJson_array_attrValue = %s", pJson_array_attrValue->valuestring);
		
		if (strncmp(pJson_array_attrName->valuestring, EXECUTETIME, sizeof(EXECUTETIME)) == 0)//执行时间
		{		
			tmp_event.time = atoi(pJson_array_attrValue->valuestring) + 30;
			DEBUG_LOGE(PRINT_TAG, "Execute Time = [%ld]", tmp_event.time);
		}
		else if (strncmp(pJson_array_attrName->valuestring, EVENTTYPE, sizeof(EVENTTYPE)) == 0)//事件类型
		{		
			if(strncmp(pJson_array_attrValue->valuestring, OUTDATE, sizeof(OUTDATE)) == 0) //日期过期			
			{
				memo_valid = 0;				
			}
			else if (strncmp(pJson_array_attrValue->valuestring, REMINDMEMO, sizeof(REMINDMEMO)) == 0)//记录备忘
			{
				memo_valid = 1;				
			}
		}
		else if (strncmp(pJson_array_attrName->valuestring, EVENTCONTENT, sizeof(EVENTCONTENT)) == 0)//事件内容
		{		
			memcpy(tmp_event.str, pJson_array_attrValue->valuestring, sizeof(tmp_event.str));
		}		
		
		pJson_array_item      = NULL;
		pJson_array_attrName  = NULL;
		pJson_array_attrValue = NULL;
	}

	if (memo_valid)
	{
		//audio_play_tone_mem(FLASH_MUSIC_BEI_WANG_YI_TIAN_JIA, AUDIO_TERM_TYPE_NOW);
		task_thread_sleep(2000);
		add_one_memo(&tmp_event);
	}
	else
	{
		//audio_play_tone_mem(FLASH_MUSIC_BEI_WANG_YI_GUO_QI, AUDIO_TERM_TYPE_NOW);
		//memset(memo_struct_handle->memo_event.str, 0, sizeof(memo_struct_handle->memo_event.str));
	}
	
	if (NULL != pJson_body) 
	{
		cJSON_Delete(pJson_body);
		pJson_body = NULL;
	}

	return 0;		

get_memo_result_error:	

	if (NULL != pJson_body) 
	{
		cJSON_Delete(pJson_body);
		pJson_body = NULL;
	}
	
	return -1;
}


APP_FRAMEWORK_ERRNO_t memo_service_create(const int task_priority)
{
	memo_array = (MEMO_ARRAY_T*)memory_malloc(sizeof(MEMO_ARRAY_T));
	if(!memo_array) {
    	DEBUG_LOGE(PRINT_TAG, "memo_array out of memory");
		return APP_FRAMEWORK_ERRNO_MALLOC_FAILED;
	}
	
	memset(memo_array, 0x00, sizeof(MEMO_ARRAY_T));
    if (!(g_thMemo = (rtos::Thread *)task_thread_create(task_memo_manage,
	        "task_memo_manage",
	        APP_NAME_MEMO_MANAGE_STACK_SIZE,
	        memo_array,
	        task_priority))) {

    	DEBUG_LOGE(PRINT_TAG, "ERROR creating task_memo task! Out of memory?");
		if(memo_array)
			memory_free(memo_array);
		return APP_FRAMEWORK_ERRNO_FAIL;
    }

	DEBUG_LOGI(PRINT_TAG, "memo_service_create\r\n");
	return APP_FRAMEWORK_ERRNO_OK;
}

