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

#ifndef DEVICE_PARAMS_MANAGE_H
#define DEVICE_PARAMS_MANAGE_H

#include <stdio.h>
#include <stdlib.h>
#include <ctypes_interface.h>
#include "partitions_config.h"
#include "dcl_interface.h"
#include "flash_operation_interface.h"
#include "memo_service.h"
#define MAX_WIFI_NUM 			5 
#define DEVICE_PARAMS_VERSION_1 0x20190211

//crc32 verify
typedef struct DEVICE_PARAMS_BASE_t
{
	uint32_t crc32;
}DEVICE_PARAMS_BASE_t;

//参数类型
typedef enum FLASH_CONFIG_PARAMS_T
{
	FLASH_CFG_START = 0,
	FLASH_CFG_FLASH_MUSIC_INDEX,
	FLASH_CFG_SDCARD_FOLDER_INDEX,	
	FLASH_CFG_AUTH_INFO,
	FLASH_CFG_WIFI_INFO,
	FLASH_CFG_USER_ID,
	FLASH_CFG_DEVICE_ID,
	FLASH_CFG_MEMO_PARAMS,
	FLASH_CFG_OTA_MODE,//OTA升级模式，采用正式还是测试服务器
	FLASH_CFG_ASR_MODE,
	FLASH_CFG_APP_ID,
	FLASH_CFG_ROBOT_ID,
	FLASH_CFG_DEVICE_LICENSE,
	FLASH_CFG_END
}FLASH_CONFIG_PARAMS_T;

//OTA升级模式
typedef enum OTA_UPDATE_MODE_T
{
	OTA_UPDATE_MODE_FORMAL = 0x12345678, //正式版本
	OTA_UPDATE_MODE_TEST = 0x87654321,	//测试版本
}OTA_UPDATE_MODE_T;

//total 64 bytes
typedef struct DEVICE_WIFI_INFO_T
{
	char wifi_ssid[64];		//wifi name
	char wifi_passwd[64];	//wifi password
}DEVICE_WIFI_INFO_T;

//total 644 bytes
typedef struct DEVICE_WIFI_INFOS_T
{
	uint8_t wifi_storage_index;					// 最新wifi信息存储位置
	uint8_t wifi_connect_index;					// 最新连接wifi位置
	uint8_t res[2];
	DEVICE_WIFI_INFO_T wifi_info[MAX_WIFI_NUM];
}DEVICE_WIFI_INFOS_T;

//total 512 bytes
typedef struct DEVICE_BASIC_INFO_T
{
	char device_sn[64];				//device serial num
	char bind_user_id[64];			//user id,use to bind user
	char weixin_dev_type[64];		//微信设备类型
	char weixin_dev_license[256];	//微信设备许可
	char res[64];					//res
}DEVICE_BASIC_INFO_T;

//total 256 + 1 bytes
typedef struct DEVICE_AUTH__INFO_T
{
	char flag;					//device serial num
	char license[256];			//设备许可
}DEVICE_AUTH_INFO_T;

//total size 4*1024 bytes,must 4 bytes align
typedef struct DEVICE_PARAMS_CONFIG_T
{
	//crc32校验 4字节
	DEVICE_PARAMS_BASE_t device_params_header;//CRC32校验
	
	//param version 4 bytes
	uint32_t params_version;

	DEVICE_AUTH_INFO_T auth_info;	
	//wifi info 644 bytes
	DEVICE_WIFI_INFOS_T wifi_infos;
	
	//device basic info 512 bytes
	DEVICE_BASIC_INFO_T basic_info;

	//ota update mode
	uint32_t ota_mode;

	//memo array	
#ifndef __NOT_USE_FLSAH__
	MEMO_ARRAY_T memo_array;
#endif

	//res
	//uint8_t res[2928];
}DEVICE_PARAMS_CONFIG_T;

/**
 * @brief  print wifi infos 
 *  
 * @param  [in]_wifi_infos
 * @return none
 */
void print_wifi_infos(DEVICE_WIFI_INFOS_T* _wifi_infos);

/**
 * @brief  print basic info 
 *  
 * @param  [in]_basic_info
 * @return none
 */
void print_basic_info(DEVICE_BASIC_INFO_T *_basic_info);

/**
 * @brief  print other params 
 *  
 * @param  [in]_other_info
 * @return none
 */
void print_other_params(DEVICE_PARAMS_CONFIG_T *_other_info);

/**
 * @brief  print device params 
 *  
 * @param  none
 * @return none
 */
void print_device_params(void);

/**
 * @brief  save device params
 *  
 * @param  none
 * @return device parameters errno
 */
static bool save_device_params(void);

/**
 * @brief  remove pcm queue 
 *  
 * @param  [in]_params
 * @param  [out]_value
 * @return int
 */
bool get_flash_cfg(FLASH_CONFIG_PARAMS_T _params, void *_value);

/**
 * @brief  get dcl auth params 
 *  
 * @param  [in]dcl_auth_params
 * @return true/false
 */
bool get_dcl_auth_params(DCL_AUTH_PARAMS_t *dcl_auth_params);

/**
 * @brief  set flash cfg 
 *  
 * @param  [in]_params
 * @param  [out]_value
 * @return int
 */
bool set_flash_cfg(FLASH_CONFIG_PARAMS_T _params, void *_value);

/**
 * @brief  init device params 
 *  
 * @param  [out]_config
 * @return none
 */
void init_device_params_v1(DEVICE_PARAMS_CONFIG_T *_config);

/**
 * @brief  remove pcm queue 
 * @param  none
 * @return device parameters errno
 */
bool init_device_params(void);

/**
 * @brief  init default params
 * @param  [in] void
 * @return none
 */
bool init_default_params(void);

#endif

