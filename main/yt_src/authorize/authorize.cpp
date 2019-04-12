//
//  authorize.c
//  Protocol V5.1
//
//  Created by ustudy on 19/1/15.
//  
//
#include "authorize.h"
#include "dataProcess.h"
#include "socket_interface.h"
#include "device_params_manage.h"
#include "device_info_interface.h"
#include "Thread.h"
#include "debug_log_interface.h"

#define LOG_TAG "auth" 

static bool is_authorized = true;
static sock_t server_sock = INVALID_SOCK;

static AUTH_STATUS_t auth_state = AUTH_STAUTS_IDLE;

#define CUSTOMER_NAME  "ZhuXP001"
#define PRODUCT_NAME  "ZXPGSJ01"

#define MAX_RETRY_SEND_TIMES 5
#define MAX_RETRY_RECV_TIMES 2

static const char *dest_port =  "5000";//target port number
static const char *dest_host =  "om.ustudy100.com";

static rtos::Thread *g_thAuthorized = NULL;

bool youngtone_is_authorized()
{
	if(!is_authorized)
		DEBUG_LOGE(LOG_TAG, "youngtone_authorize fail!!");
	
	return 	is_authorized;
}

static int youngtone_auth_send_request(char *pResPacket, int pPacketLen)
{
	if (server_sock != INVALID_SOCK)
	{
		sock_close(server_sock);
		server_sock = INVALID_SOCK;
	}
		
	server_sock = sock_connect(dest_host, dest_port);
	if (server_sock == INVALID_SOCK)
	{
		DEBUG_LOGE(LOG_TAG, "auth_connect %s:%s failed", 
			dest_host, dest_port);
		return -1;
	}
	else
	{
		DEBUG_LOGI(LOG_TAG, "auth_connect %s:%s success", 
			dest_host, dest_port);
	}
	sock_set_nonblocking(server_sock);

	int ret = sock_writen_with_timeout(server_sock, pResPacket, pPacketLen, 6000);
	if (pPacketLen != ret)
	{
		DEBUG_LOGE(LOG_TAG, "auth_send failed");
		return -1;
	}

	return 0;
}

static int youngtone_authorize_close()
{
	if (server_sock != INVALID_SOCK)
	{
		sock_close(server_sock);
		server_sock = INVALID_SOCK;
	}
	
	return 0;
}

static int youngtone_authorize_recv()
{	
	if (server_sock == INVALID_SOCK)
	{
		return -1;
	}
	
	char pInPacket[PACKET_BYTE_NUM] = {0};
	char pOutPacket[PACKET_BYTE_NUM] = {0};
	int nOutPacketLen = 0;
	int ret = sock_readn_with_timeout(server_sock, pInPacket, PACKET_BYTE_NUM, 5000);
	if(ret != PACKET_BYTE_NUM) {
		return -1;
	}

	ret = you_333_KeepPacket_001(pInPacket, PACKET_BYTE_NUM, pOutPacket, &nOutPacketLen);
	if(ret < 0 || (nOutPacketLen != PACKET_BYTE_NUM)) {
		return -1;
	}

	DEVICE_AUTH_INFO_T auth_info;
	memset(&auth_info, 0, sizeof(auth_info));
		
	if (!get_flash_cfg(FLASH_CFG_AUTH_INFO, &auth_info))
	{
		DEBUG_LOGE(LOG_TAG, "get_flash_cfg failed");
		return -1;
	}
	
	ret = you_333_KeepData_001(auth_info.license, sizeof(auth_info.license), pOutPacket, nOutPacketLen);
	if(ret < 0) {
		DEBUG_LOGE(LOG_TAG,"you_333_KeepData_001 fail");
		return -1;
	}

	auth_info.flag = 1;
    //it is possible to save data wrongly
	ret = set_flash_cfg(FLASH_CFG_AUTH_INFO, &auth_info);	
	if (!ret)
	if(ret < 0) {
		DEBUG_LOGE(LOG_TAG,"save FLASH_CFG_AUTH_INFO fail");
		return -1;
	}

	memset(pOutPacket, 0x00, PACKET_BYTE_NUM);
	you_333_GeneratePacket_002(auth_info.license, '1', pOutPacket, &nOutPacketLen);

	int i = 0;
	for(i = 0; i < MAX_RETRY_SEND_TIMES; i++) {
		if(youngtone_auth_send_request(pOutPacket, nOutPacketLen) == 0) {					
			break;
		}
		else {
			task_thread_sleep(1000);
		}
	}
	
	DEBUG_LOGI(LOG_TAG,"<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<");
	DEBUG_LOGI(LOG_TAG,"youngtone_authorize succeed!");
	DEBUG_LOGI(LOG_TAG,"<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<\n");
	
	return 0;
}

static int youngtone_authorize_start()
{
	DEVICE_AUTH_INFO_T auth_info;

	char pOutPacket[PACKET_BYTE_NUM] = {0};
	int pPacketLen = 0;
	
	memset(&auth_info, 0, sizeof(auth_info));

	if (!get_flash_cfg(FLASH_CFG_AUTH_INFO, &auth_info))
	{
		DEBUG_LOGE(LOG_TAG, "get_flash_cfg failed");
		return -1;
	}

	char strMac[6] = {0};
	get_device_id(strMac, sizeof(strMac));

	if(auth_info.flag == 0)
	{
		you_333_GeneratePacket_001(CUSTOMER_NAME, PRODUCT_NAME, strMac, pOutPacket, &pPacketLen);
	}
	else
	{
		you_333_GeneratePacket_002(auth_info.license, '0', pOutPacket, &pPacketLen);
	}

	return youngtone_auth_send_request(pOutPacket, pPacketLen);
}

static void youngtone_authorize_process()
{	
	static char retry_recv = 0;
	int i = 0;
	
	switch (auth_state)
	{
		case AUTH_STAUTS_IDLE:
			break;

		case AUTH_STAUTS_INIT:
			for(i = 0; i < MAX_RETRY_SEND_TIMES; i++) {
				if(youngtone_authorize_start() == 0) {					
					auth_state = AUTH_STAUTS_WAIT;
					break;
				}
				else {
					task_thread_sleep(6000);
				}
			}

			if(i == MAX_RETRY_SEND_TIMES) {
				youngtone_authorize_close();
				is_authorized = false;
				auth_state = AUTH_STAUTS_IDLE;
			}
			break;

		case AUTH_STAUTS_WAIT:
			if(youngtone_authorize_recv() == 0)
			{	
				youngtone_authorize_close();
				retry_recv = 0;
				is_authorized = true;
				auth_state = AUTH_STAUTS_SUCCESS;
			}else if(retry_recv < MAX_RETRY_RECV_TIMES) {		
				youngtone_authorize_close();
				retry_recv++;				
				task_thread_sleep(8000);
				auth_state = AUTH_STAUTS_INIT;
			}
			else {				
				youngtone_authorize_close();
				retry_recv = 0;
				is_authorized = false;
				auth_state = AUTH_STAUTS_IDLE;
			}
			break;
			
		case AUTH_STAUTS_SUCCESS:				
			break;
			
		default:
			break;
	}
}

static void auth_event_callback(
	void *app_handler, 
	APP_EVENT_MSG_t *msg)
{
	APP_OBJECT_t *app = (APP_OBJECT_t *)app_handler;

	switch (msg->event)
	{
		case APP_EVENT_DEFAULT_LOOP_TIMEOUT:
		{
			youngtone_authorize_process();
			break;
		}
		case APP_EVENT_WIFI_CONNECTED:
		{
			if(auth_state == AUTH_STAUTS_IDLE)
				auth_state = AUTH_STAUTS_INIT;
			break;
		}
		case APP_EVENT_WIFI_DISCONNECTED:
		{			
			break;
		}
		case APP_EVENT_DEFAULT_EXIT:
		{
			app_exit((APP_OBJECT_t *)app);
			break;
		}
		default:
			break;
	}
}

static void task_authorize_service(void)
{	
	APP_OBJECT_t *app = NULL;
	
	app = app_new(APP_NAME_AUTH_SERVICE);
	if (app == NULL)
	{
		DEBUG_LOGE(LOG_TAG, "new app[%s] failed, out of memory", APP_NAME_AUTH_SERVICE);
	}
	else
	{
		app_set_loop_timeout(app, 500, auth_event_callback);		
		app_add_event(app, APP_EVENT_DEFAULT_BASE, auth_event_callback);		
		app_add_event(app, APP_EVENT_WIFI_BASE, auth_event_callback);
		DEBUG_LOGI(LOG_TAG, "%s create success", APP_NAME_AUTH_SERVICE);
	}
	
	app_msg_dispatch(app);
	
	app_delete(app);

	youngtone_authorize_close();
	
	task_thread_exit();

	DEBUG_LOGI(LOG_TAG, "task_authorize_service app_exit");
}

APP_FRAMEWORK_ERRNO_t authorize_service_create(const int task_priority)
{
	if (! (g_thAuthorized =(rtos::Thread *)task_thread_create(task_authorize_service,
                    APP_NAME_AUTH_SERVICE,
                    APP_NAME_AUTH_SERVICE_STACK_SIZE,
                    NULL,
                    task_priority))) 
    {
        DEBUG_LOGE(LOG_TAG, "ERROR creating task_auth_service task! Out of memory?");
		return APP_FRAMEWORK_ERRNO_FAIL;
    }

	return APP_FRAMEWORK_ERRNO_OK;
}

APP_FRAMEWORK_ERRNO_t authorize_service_delete(void)
{
	//return app_send_message(APP_NAME_AUTH_SERVICE, APP_NAME_AUTH_SERVICE, APP_EVENT_DEFAULT_EXIT, NULL, 0);
	
	app_send_message(APP_NAME_AUTH_SERVICE, APP_NAME_AUTH_SERVICE, APP_EVENT_DEFAULT_EXIT, NULL, 0);
	while(g_thAuthorized->get_state() != rtos::Thread::Deleted){rtos::Thread::wait(10);}
	task_thread_exit(g_thAuthorized);

}


