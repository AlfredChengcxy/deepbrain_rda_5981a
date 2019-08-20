#ifndef USER_CONFIG_H
#define USER_CONFIG_H

#include <stdlib.h>
#include "app_framework.h"
#include "Thread.h"
#include "time_interface.h"
#include "memory_interface.h"
#include "device_params_manage.h"

#define DCL_DEMO_ENV					0	//DEMO鐜
#define DCL_PRE_ENV						0	
#define DCL_YT_ENV						0	//youngtone
#define DCL_SF_ENV						0	//sf
#define DCL_KMT_ENV						1	//sf

#define PROJECT_NAME  "PJ-20180830-0001-AIXIAOBEI-AsHjXYD4CwNZDU3G"

/******************************************************************
//WIFI AP淇℃伅
*******************************************************************/
#define WIFI_SSID_DEFAULT 		 "TP-LINK_759F"
#define WIFI_PASSWD_DEFAULT 	 "12345678"
//#define WIFI_SSID_DEFAULT 		 "CMCC-tayg"
//#define WIFI_PASSWD_DEFAULT 	 "nscnas3b"

//#define WIFI_PASSWD_DEFAULT 	 "123456789"


/******************************************************************
//璁惧淇℃伅锛屽寘鎷簭鍒楀彿鍓嶇紑銆佺増鏈彿銆佽处鍙风瓑
*******************************************************************/
#define PLATFORM_NAME		"RDA5981"
#define DEVICE_SN_PREFIX	"YIYU"

/*
修改一下 ESP32_FW_VERSION 并保持该version 与 version.txt中的 version 相同
在将生成的main_ota_1001_upgrade.bin(注意修改ota3_ota_12.bat中的版本号)和新的version.txt在服务器上更新下


值得注意的是 初次烧入带boolloader的bin 的version 与ota服务器上的version不能相同
*/

#define ESP32_FW_VERSION	"V1.0.1build20190819"
//#define ESP32_FW_VERSION	"V1.0.8build20190510"

//demo鐜
#if DCL_DEMO_ENV == 1
#define DEEP_BRAIN_APP_ID 	"460180de15d811e7827e90b11c244b31"
#define DEEP_BRAIN_ROBOT_ID "0dee45b6-bc7f-11e8-86f9-90b11c244b31"
#endif

#if DCL_PRE_ENV == 1
#define DEEP_BRAIN_APP_ID 	"511de70eb86511e79430801844e30cac"
#define DEEP_BRAIN_ROBOT_ID "0c715591-d027-11e8-8148-801844e30cac"
#endif

#if DCL_YT_ENV == 1
//#define DEEP_BRAIN_APP_ID 	"A000000000000438"
//#define DEEP_BRAIN_ROBOT_ID "0c7a9d8f-1d28-11e9-8148-801844e30cac"
#define DEEP_BRAIN_APP_ID 	"A000000000000438"
#define DEEP_BRAIN_ROBOT_ID "352f6362-a6a3-11e9-8148-801844e30cac"

#endif

#if DCL_SF_ENV == 1
#define DEEP_BRAIN_APP_ID 	"A000000000000436"
#define DEEP_BRAIN_ROBOT_ID "cb8c2775-1ad4-11e9-8148-801844e30cac"
#endif


#if DCL_KMT_ENV == 1
#define DEEP_BRAIN_APP_ID 	"A000000000000468"
#define DEEP_BRAIN_ROBOT_ID "a91e9615-786c-11e9-8148-801844e30cac"
#endif


//OTA鍗囩骇鏈嶅姟鍣ㄥ湴鍧�
//#define OTA_UPDATE_SERVER_URL		"http://file.yuyiyun.com:2088/ota/PJ-20180830-0002-aixiaobei/version.txt"
//#define OTA_UPDATE_SERVER_URL		"http://192.168.1.153/firmware/version.txt"
#if DCL_SF_ENV == 1
#define OTA_UPDATE_SERVER_URL		"http://file.yuyiyun.com:2088/ota/PJ-20190222-0001-zhuxiaopi/a/version.txt"
#endif

#if DCL_KMT_ENV == 1 || DCL_YT_ENV == 1
#define OTA_UPDATE_SERVER_URL		"http://211.152.53.233/RDA_Yiyu_OTA_Bin/version.txt"
#endif



//OTA鍗囩骇鏈嶅姟鍣ㄦ祴璇曞湴鍧�
#define OTA_UPDATE_SERVER_URL_TEST	"http://file.yuyiyun.com:2088/ota/test/PJ-20180830-0002-aixiaobei/version.txt"
//deepbrain open api 鍦板潃
#define DeepBrain_TEST_URL          "http://api.deepbrain.ai:8383/open-api/service"

/******************************************************************
//浜у搧鍔熻兘妯″潡鎺у埗 APP MODULE CONTROL -> AMC
*******************************************************************/


/******************************************************************
//
*******************************************************************/
#define TASK_PRIORITY_1	0
#define TASK_PRIORITY_2	1
#define TASK_PRIORITY_3	2
#define TASK_PRIORITY_4	3
#define TASK_PRIORITY_5	4

/******************************************************************
//浜у搧纭欢IO瀹氫箟
*******************************************************************/

#endif /* __USER_CONFIG_H__ */

