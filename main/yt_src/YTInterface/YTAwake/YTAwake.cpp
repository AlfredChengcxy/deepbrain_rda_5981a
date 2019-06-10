/*********************************************************************************
  *Copyright(C),http://www.youngtone.cn
  *FileName:  YTAwake.cpp
  *Author:  Àî¾ý
  *Version:  v1.0
  *Date:  2018.11.11
**********************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "YTAwake.h"
#include "YTAwakeEngine.h"
#include "debug_log_interface.h"
#include "memory_interface.h"
#include "semaphore_lock_interface.h"
#include "App_config.h"

#include "Thread.h"
#include "rt5670.h"
#include "ringbuff.h"
#include "events.h"
enum VADState {
	VAD_IDLE,
	VAD_START,
	VAD_END
};


typedef struct YT_AWAKE_HANDLE_t
{
	void *	mutex_lock;
	void * 	pEngine;
	void * 	pMemory;
	void * 	pContainer;
	bool 	bIsInit;
	bool	bIsStart;
	Rt5670* pRt5670;
	Ringbuff *rb;
}YT_AWAKE_HANDLE_t;

static YT_AWAKE_HANDLE_t * g_awake_service_handle = NULL;
static void *g_thAwake = NULL;
static void *g_thRt5670 = NULL;

#define FILE_SAVER 0
#define YTASR_MEM_SIZE (65*1024)
#define YTASR_BUF_SIZE 640
#define LOG_TAG "awake"

static int pOriginalWordIndex[1] = {-1};
static int nResultNumber = 0;
static char * szResultUTF8 = NULL;
static const int nMaxCmd = 1;
static char cFlag_VAD;
static int nFlame = 0;
static int nFlameSample = 160;
///// for run thread
static bool bCanRunOne = false;
static bool bVadStart = false;
static bool bVadEnd = false;


static bool Init()
{
	if(g_awake_service_handle->bIsInit)return true;
////////////////////////////////////////	
	DEBUG_LOGI(LOG_TAG,"Entry Engine Init");	
	g_awake_service_handle->bIsInit = false;	
 	g_awake_service_handle->pContainer = ytv_asr_tiny_create_container(NULL,NULL,3,NULL);
	g_awake_service_handle->bIsInit = 
		(ytv_asr_tiny_create_engine
		(g_awake_service_handle->pContainer,
		(char*)(g_awake_service_handle->pMemory),
		YTASR_MEM_SIZE,
		&(g_awake_service_handle->pEngine)) == 0
		)? true:false;
	if(g_awake_service_handle->bIsInit) {DEBUG_LOGI(LOG_TAG,"Engine Init Success");}
	else {DEBUG_LOGI(LOG_TAG,"Engine Init Fail");}
	DEBUG_LOGI(LOG_TAG,"Leave Engine Init");
	return g_awake_service_handle->bIsInit;
}


APP_FRAMEWORK_ERRNO_t yt_awake_start()
{
	if(!g_awake_service_handle || !g_awake_service_handle->bIsInit)return;
	DEBUG_LOGI(LOG_TAG,"Awake Start");
	SEMPHR_TRY_LOCK(g_awake_service_handle->mutex_lock);
	bCanRunOne = true;
	bVadEnd = false;
	bVadStart = false;
	SEMPHR_TRY_UNLOCK(g_awake_service_handle->mutex_lock);
}

APP_FRAMEWORK_ERRNO_t yt_awake_stop()
{
	if(!g_awake_service_handle || !g_awake_service_handle->bIsInit)return;
	DEBUG_LOGI(LOG_TAG,"Awake Stop");
	SEMPHR_TRY_LOCK(g_awake_service_handle->mutex_lock);
	bCanRunOne = false;
	bVadEnd = false;
	bVadStart = false;
	SEMPHR_TRY_UNLOCK(g_awake_service_handle->mutex_lock);
}

static void PutVadDataForAwake(unsigned char *ppData,int nSize,int nFlameSample)
{
	nSize/=sizeof(short);
	nFlame = nSize/nFlameSample;

	//DEBUG_LOGI(LOG_TAG,"PutVadDataForAwake");
		
	for(int i = 0; i < nFlame; i++)
	{	
		ytv_asr_tiny_put_pcm_data(g_awake_service_handle->pEngine,(short *)(ppData + i*nFlameSample*sizeof(short)),nFlameSample,&cFlag_VAD);
		if(cFlag_VAD == 'S')
		{	
			SEMPHR_TRY_LOCK(g_awake_service_handle->mutex_lock);
			bVadEnd = false;
			bVadStart = true;
			SEMPHR_TRY_UNLOCK(g_awake_service_handle->mutex_lock);
			DEBUG_LOGI(LOG_TAG,"VadStart"); 
		}
		else if(cFlag_VAD == 'Y' && !bVadEnd) 
		{	
			SEMPHR_TRY_LOCK(g_awake_service_handle->mutex_lock);
			bVadStart = false;
			bVadEnd = true;
			SEMPHR_TRY_UNLOCK(g_awake_service_handle->mutex_lock);
			DEBUG_LOGI(LOG_TAG,"VadEnd");
			break;			
		}
	}
}


static void OnData(unsigned char * buf, int len)
{
	if(!g_awake_service_handle->bIsInit)return;		
	if(!bVadEnd)
	PutVadDataForAwake(buf,len,nFlameSample);
}



static void GetResult()
{
	ytv_asr_tiny_notify_data_end(g_awake_service_handle->pEngine);			
	ytv_asr_tiny_get_result(g_awake_service_handle->pEngine,nMaxCmd, &nResultNumber,pOriginalWordIndex);
	if(nResultNumber)szResultUTF8 = ytv_asr_tiny_get_one_command_utf8(g_awake_service_handle->pEngine,pOriginalWordIndex[0]);	
	DEBUG_LOGI(LOG_TAG,"Result:%s",szResultUTF8?szResultUTF8:"null");	
	ytv_asr_tiny_reset_engine(g_awake_service_handle->pEngine); 			
	szResultUTF8 = NULL;
}



static void awake_client_process()
{
	if(bCanRunOne)
	{
		int nRunOneResult = -1;
		if(bVadStart)
		{	
			//static int j =1;
			//DEBUG_LOGI(LOG_TAG,"j:[%d]",j++);
			nRunOneResult = ytv_asr_tiny_run_one_step(g_awake_service_handle->pEngine); 
		}
		if(bVadEnd)
		{
			SEMPHR_TRY_LOCK(g_awake_service_handle->mutex_lock);
			bVadEnd = false;
			SEMPHR_TRY_UNLOCK(g_awake_service_handle->mutex_lock);
			GetResult();	
			if(nResultNumber)duer::event_trigger(duer::EVT_KEY_REC_PRESS);
			//// add your process	
		}	
	}
}

static void awake_event_callback(	
	void *app, 
	APP_EVENT_MSG_t *msg)
{

#if FILE_SAVER
	static FILE * fp = NULL;
#endif	

	switch (msg->event)
	{
		case APP_EVENT_DEFAULT_LOOP_TIMEOUT:
		{
			char *data = NULL;
			g_awake_service_handle->rb->get_readPtr(&data);
			
			if(data) {
				OnData((unsigned char *)data, YTASR_BUF_SIZE);
				g_awake_service_handle->rb->read_done();
			#if FILE_SAVER
				if(fp)
				fwrite(data,YTASR_BUF_SIZE,1,fp);
			#endif
				//static int i = 1;
				//DEBUG_LOGI(LOG_TAG,"i:[%d]",i++);
			}

			awake_client_process();
			break;
		}
		case APP_EVENT_WIFI_CONNECTED:
		{	
		#if FILE_SAVER
			fp = fopen("/sd/test_5670_pcm.pcm","wb");
		#endif		
			yt_awake_start();
			g_awake_service_handle->bIsStart = true;
			((rtos::Thread *)g_thRt5670)->signal_set(0x01);
			break;
		}
		case APP_EVENT_DEFAULT_EXIT:
		{
			g_awake_service_handle->bIsStart = false;
			app_exit((APP_OBJECT_t*)app);
			break;
		}
		default:
			break;	
	}
}


static void task_awake_record()
{
	((rtos::Thread *)g_thRt5670)->signal_wait(0x01);
	DEBUG_LOGI(LOG_TAG,"entry task_awake_record");
	char* data = NULL;
	g_awake_service_handle->pRt5670->start_record();
	while(g_awake_service_handle->bIsStart)
	{
		g_awake_service_handle->rb->get_writePtr(&data);
		g_awake_service_handle->pRt5670->read_data(data, YTASR_BUF_SIZE);
		g_awake_service_handle->pRt5670->read_done();
		g_awake_service_handle->rb->write_done();	
	}
	g_awake_service_handle->pRt5670->stop_record();	
	DEBUG_LOGI(LOG_TAG,"leave task_awake_record");
}


static void task_awake_service()
{
	APP_OBJECT_t *app = NULL;
	app = app_new(APP_NAME_AWAKE_SERVICE);
	if (app == NULL)
	{
		DEBUG_LOGE(LOG_TAG, "new app[%s] failed, out of memory", APP_NAME_AWAKE_SERVICE);
	}
	else
	{
		app_set_loop_timeout(app, 2, awake_event_callback);
		app_add_event(app, APP_EVENT_DEFAULT_BASE, awake_event_callback);
		app_add_event(app, APP_EVENT_WIFI_BASE, awake_event_callback);
		DEBUG_LOGI(LOG_TAG, "%s create success", APP_NAME_AWAKE_SERVICE);
	}	
	app_msg_dispatch(app);	
	app_delete(app);
	task_thread_exit(g_thRt5670);
	task_thread_exit();
	yt_awake_destory();
	DEBUG_LOGI(LOG_TAG, "task_awake_service app_exit");
}


APP_FRAMEWORK_ERRNO_t yt_awake_create(const int task_priority)
{
	if (g_awake_service_handle != NULL)
	{
		DEBUG_LOGE(LOG_TAG, "g_awake_service_handle already exist");
		return APP_FRAMEWORK_ERRNO_FAIL;
	}

//// create
	g_awake_service_handle = (YT_AWAKE_HANDLE_t*)memory_malloc(sizeof(YT_AWAKE_HANDLE_t));
	if (g_awake_service_handle == NULL)
	{
		DEBUG_LOGE(LOG_TAG, "yt_awake_create failed");
		return APP_FRAMEWORK_ERRNO_MALLOC_FAILED;
	}
	memset(g_awake_service_handle, 0, sizeof(YT_AWAKE_HANDLE_t));

	SEMPHR_CREATE_LOCK(g_awake_service_handle->mutex_lock);
	g_awake_service_handle->pMemory = (char *)memory_malloc(YTASR_MEM_SIZE);
	if (g_awake_service_handle->pMemory == NULL)
	{
		DEBUG_LOGE(LOG_TAG, "yt_awake_memory_create failed");
		return APP_FRAMEWORK_ERRNO_MALLOC_FAILED;
	}	
	memset(g_awake_service_handle->pMemory,0,YTASR_MEM_SIZE);
	g_awake_service_handle->pRt5670 = new Rt5670;
	g_awake_service_handle->pRt5670->init();
	Init();
	g_awake_service_handle->rb = new Ringbuff(YTASR_BUF_SIZE,4);
	g_awake_service_handle->rb->init();

//// create thread	
	if (!( g_thAwake=/*(rtos::Thread *)*/ task_thread_create(task_awake_service,"task_awake_service",APP_NAME_AWAKE_SERVICE_STACK_SIZE,g_awake_service_handle,task_priority))) 
	{
		DEBUG_LOGE(LOG_TAG, "ERROR creating yt_awake_create task! Out of memory?");
		SEMPHR_DELETE_LOCK(g_awake_service_handle->mutex_lock);
		if(g_awake_service_handle->pContainer)memory_free(g_awake_service_handle->pContainer);
		g_awake_service_handle->pContainer = NULL;
		if(g_awake_service_handle->pEngine)memory_free(g_awake_service_handle->pEngine);
		g_awake_service_handle->pEngine = NULL;
		if(g_awake_service_handle->pMemory)memory_free(g_awake_service_handle->pMemory);
		g_awake_service_handle->pMemory = NULL;
		if(g_awake_service_handle->rb)delete g_awake_service_handle->rb;
		g_awake_service_handle->rb = NULL;	
		g_awake_service_handle->bIsInit = false;
		if(g_awake_service_handle->pRt5670)delete g_awake_service_handle->pRt5670;
		g_awake_service_handle->pRt5670 = NULL;
		memory_free(g_awake_service_handle);
		g_awake_service_handle = NULL;
		return APP_FRAMEWORK_ERRNO_FAIL;
	}	

	g_thRt5670=/*(rtos::Thread *)*/ task_thread_create(task_awake_record,"task_awake_record",APP_NAME_AWAKE_RECORD_STACK_SIZE,g_awake_service_handle,task_priority);
		
	return APP_FRAMEWORK_ERRNO_OK;

}

APP_FRAMEWORK_ERRNO_t yt_awake_delete(void)
{
	if(g_awake_service_handle &&g_thAwake)
	{
		app_send_message(APP_NAME_AWAKE_SERVICE, APP_NAME_AWAKE_SERVICE, APP_EVENT_DEFAULT_EXIT, NULL, 0);
		//while(g_thIot->get_state() != rtos::Thread::Deleted){rtos::Thread::wait(10);}
		
		task_thread_exit(g_thAwake);
	}
}

APP_FRAMEWORK_ERRNO_t yt_awake_destory(void)
{
	if(g_awake_service_handle)
	{
		//memory_info();
		
		SEMPHR_DELETE_LOCK(g_awake_service_handle->mutex_lock);
		if(g_awake_service_handle->pContainer)memory_free(g_awake_service_handle->pContainer);
		g_awake_service_handle->pContainer = NULL;
		if(g_awake_service_handle->pEngine)memory_free(g_awake_service_handle->pEngine);
		g_awake_service_handle->pEngine = NULL;
		if(g_awake_service_handle->pMemory)memory_free(g_awake_service_handle->pMemory);
		g_awake_service_handle->pMemory = NULL;
		if(g_awake_service_handle->rb)delete g_awake_service_handle->rb;
		g_awake_service_handle->rb = NULL;	
		g_awake_service_handle->bIsInit = false;
		if(g_awake_service_handle->pRt5670)delete g_awake_service_handle->pRt5670;
		g_awake_service_handle->pRt5670 = NULL;
		memory_free(g_awake_service_handle);
		
		//memory_info();
	}
	g_awake_service_handle = NULL;
}

