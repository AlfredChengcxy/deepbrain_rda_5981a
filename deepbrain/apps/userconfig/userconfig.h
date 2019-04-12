#ifndef USER_CONFIG_H
#define USER_CONFIG_H

#include <stdlib.h>
#include "app_framework.h"
#include "Thread.h"
#include "time_interface.h"
#include "memory_interface.h"
#include "device_params_manage.h"

#define DCL_DEMO_ENV					0	//DEMO环境
#define DCL_PRE_ENV						0	//预发布环境
#define DCL_YT_ENV						0	//youngtone
#define DCL_SF_ENV						1	//sf

#define PROJECT_NAME  "PJ-20180830-0001-AIXIAOBEI-AsHjXYD4CwNZDU3G"

/******************************************************************
//WIFI AP信息
*******************************************************************/
#define WIFI_SSID_DEFAULT 		 "TP-LINK_759F"
#define WIFI_PASSWD_DEFAULT 	 "12345678"
//#define WIFI_SSID_DEFAULT 		 "CMCC-tayg"
//#define WIFI_PASSWD_DEFAULT 	 "nscnas3b"

//#define WIFI_PASSWD_DEFAULT 	 "123456789"


/******************************************************************
//设备信息，包括序列号前缀、版本号、账号等
*******************************************************************/
#define PLATFORM_NAME		"RDA5981"
#define DEVICE_SN_PREFIX	"YIYU"
#define ESP32_FW_VERSION	"V1.0.0build20181129"

//demo环境
#if DCL_DEMO_ENV == 1
#define DEEP_BRAIN_APP_ID 	"460180de15d811e7827e90b11c244b31"
#define DEEP_BRAIN_ROBOT_ID "0dee45b6-bc7f-11e8-86f9-90b11c244b31"
#endif

#if DCL_PRE_ENV == 1
#define DEEP_BRAIN_APP_ID 	"511de70eb86511e79430801844e30cac"
#define DEEP_BRAIN_ROBOT_ID "0c715591-d027-11e8-8148-801844e30cac"
#endif

#if DCL_YT_ENV == 1
#define DEEP_BRAIN_APP_ID 	"A000000000000438"
#define DEEP_BRAIN_ROBOT_ID "0c7a9d8f-1d28-11e9-8148-801844e30cac"
#endif

#if DCL_SF_ENV == 1
#define DEEP_BRAIN_APP_ID 	"A000000000000436"
#define DEEP_BRAIN_ROBOT_ID "cb8c2775-1ad4-11e9-8148-801844e30cac"
#endif

//OTA升级服务器地址
//#define OTA_UPDATE_SERVER_URL		"http://file.yuyiyun.com:2088/ota/PJ-20180830-0002-aixiaobei/version.txt"
//#define OTA_UPDATE_SERVER_URL		"http://192.168.1.153/firmware/version.txt"
#define OTA_UPDATE_SERVER_URL		"http://file.yuyiyun.com:2088/ota/PJ-20190222-0001-zhuxiaopi/version.txt"

//OTA升级服务器测试地址
#define OTA_UPDATE_SERVER_URL_TEST	"http://file.yuyiyun.com:2088/ota/test/PJ-20180830-0002-aixiaobei/version.txt"
//deepbrain open api 地址
#define DeepBrain_TEST_URL          "http://api.deepbrain.ai:8383/open-api/service"

/******************************************************************
//产品功能模块控制 APP MODULE CONTROL -> AMC
*******************************************************************/


/******************************************************************
//任务优先级定义,数字越大，优先级越高
*******************************************************************/
#define TASK_PRIORITY_1	0
#define TASK_PRIORITY_2	1
#define TASK_PRIORITY_3	2
#define TASK_PRIORITY_4	3
#define TASK_PRIORITY_5	4

/******************************************************************
//产品硬件IO定义
*******************************************************************/

#endif /* __USER_CONFIG_H__ */

