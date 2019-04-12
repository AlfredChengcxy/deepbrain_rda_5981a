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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "device_params_manage.h"
#include "debug_log_interface.h"
#include "userconfig.h"
//#include "asr_service.h"
#include "crc_interface.h"
#include "memo_service.h"
#include <memory_interface.h>

#define LOG_TAG "DEVICE PARAMS"

static DEVICE_PARAMS_CONFIG_T *g_device_params = NULL;
static void* g_lock_flash_config = NULL;

uint32_t get_device_params_crc(DEVICE_PARAMS_CONFIG_T *device_params)
{
	uint32_t crc = crypto_crc32(0xffffffff, (uint8_t*)&device_params->params_version, sizeof(DEVICE_PARAMS_CONFIG_T) - sizeof(DEVICE_PARAMS_BASE_t));
	return crc;
}

void print_wifi_infos(DEVICE_WIFI_INFOS_T* _wifi_infos)
{
	int i = 0;
	
	if (_wifi_infos == NULL)
	{
		DEBUG_LOGI(LOG_TAG, "_wifi_infos== NULL");
		return;
	}

	DEBUG_LOGI(LOG_TAG, ">>>>>>>>wifi info>>>>>>>>");
	DEBUG_LOGI(LOG_TAG, "connect_index[%d],storage_index[%d]",
		_wifi_infos->wifi_connect_index+1, _wifi_infos->wifi_storage_index+1);
	
	for (i=0; i < MAX_WIFI_NUM; i++)
	{
		DEBUG_LOGI(LOG_TAG, "[%02d]:ssid[%s]:password[%s]", 
			i+1, _wifi_infos->wifi_info[i].wifi_ssid, _wifi_infos->wifi_info[i].wifi_passwd);
	}
	DEBUG_LOGI(LOG_TAG, "<<<<<<<<<<<<<<<<<<<<<<<<<");
}

void print_basic_info(DEVICE_BASIC_INFO_T *_basic_info)
{
	if (_basic_info == NULL)
	{
		return;
	}

	DEBUG_LOGI(LOG_TAG, ">>>>>>>>basic info>>>>>>>");
	DEBUG_LOGI(LOG_TAG, "bind user id:%s", _basic_info->bind_user_id);
	DEBUG_LOGI(LOG_TAG, "device sn:%s", _basic_info->device_sn);
	DEBUG_LOGI(LOG_TAG, "weixin device type:%s", _basic_info->weixin_dev_type);
	DEBUG_LOGI(LOG_TAG, "weixin device license:%s", _basic_info->weixin_dev_license);
	DEBUG_LOGI(LOG_TAG, "<<<<<<<<<<<<<<<<<<<<<<<<<");
}

void print_other_params(DEVICE_PARAMS_CONFIG_T *_other_info)
{
	if (_other_info == NULL)
	{
		return;
	}
	
	DEBUG_LOGI(LOG_TAG, ">>>>>>>>other info>>>>>>>");
	if (_other_info->ota_mode == OTA_UPDATE_MODE_FORMAL)
	{
		DEBUG_LOGI(LOG_TAG, "ota mode:正式地址[0x%x]", _other_info->ota_mode);
	}
	else if (_other_info->ota_mode == OTA_UPDATE_MODE_TEST)
	{
		DEBUG_LOGI(LOG_TAG, "ota mode:测试地址[0x%x]", _other_info->ota_mode);
	}
	else
	{
		DEBUG_LOGI(LOG_TAG, "ota mode:正式地址[0x%x]", _other_info->ota_mode);
	}
	DEBUG_LOGI(LOG_TAG, ">>>>>>>>other info>>>>>>>");
	//DEBUG_LOGI(LOG_TAG, "asr mode:%d", g_asr_mode);
	DEBUG_LOGI(LOG_TAG, "<<<<<<<<<<<<<<<<<<<<<<<<<");
}

void print_device_params(void)
{
	SEMPHR_TRY_LOCK(g_lock_flash_config);
	DEBUG_LOGI(LOG_TAG, ">>>>>>params version>>>>>>");	
	DEBUG_LOGI(LOG_TAG, "param size:%d", sizeof(DEVICE_PARAMS_CONFIG_T));
	DEBUG_LOGI(LOG_TAG, "param version:%08x", g_device_params->params_version);
	DEBUG_LOGI(LOG_TAG, "sdk version:%s", ESP32_FW_VERSION);
	DEBUG_LOGI(LOG_TAG, "<<<<<<<<<<<<<<<<<<<<<<<<<");

	print_wifi_infos(&g_device_params->wifi_infos);
	print_basic_info(&g_device_params->basic_info);
	print_other_params(g_device_params);
	
	SEMPHR_TRY_UNLOCK(g_lock_flash_config);
}

//保存参数
static bool save_device_params(void)
{	
	g_device_params->device_params_header.crc32 = get_device_params_crc(g_device_params);
	if (flash_op_write((uint8_t *)g_device_params, sizeof(DEVICE_PARAMS_CONFIG_T)))
	{
		return true;
	}
	else
	{
		DEBUG_LOGE(LOG_TAG, "write_flash_params g_device_params failed");
		return false;
	}
}

bool get_flash_cfg(FLASH_CONFIG_PARAMS_T _params, void *_value)
{
	SEMPHR_TRY_LOCK(g_lock_flash_config);
	switch (_params)
	{
		case FLASH_CFG_START:
		{
			break;
		}
		case FLASH_CFG_FLASH_MUSIC_INDEX:
		{
			break;
		}
		case FLASH_CFG_SDCARD_FOLDER_INDEX:
		{
			break;
		}
		case FLASH_CFG_AUTH_INFO:
		{
			memcpy((char*)_value, (char*)&g_device_params->auth_info, sizeof(g_device_params->auth_info));
			break;
		}
		case FLASH_CFG_WIFI_INFO:
		{
			memcpy((char*)_value, (char*)&g_device_params->wifi_infos, sizeof(g_device_params->wifi_infos));
			break;
		}
		case FLASH_CFG_USER_ID:
		{
			snprintf((char*)_value, sizeof(g_device_params->basic_info.bind_user_id), 
				"%s", g_device_params->basic_info.bind_user_id);
			break;
		}
		case FLASH_CFG_DEVICE_ID:
		{
			snprintf((char*)_value, sizeof(g_device_params->basic_info.device_sn), 
				"%s", g_device_params->basic_info.device_sn);
			break;
		}
		case FLASH_CFG_MEMO_PARAMS:
		{
		#ifndef __NOT_USE_FLSAH__
			memcpy((MEMO_ARRAY_T *)_value, (MEMO_ARRAY_T *)&g_device_params->memo_array, sizeof(MEMO_ARRAY_T));
		#endif
			break;
		}
		case FLASH_CFG_OTA_MODE:
		{
			if (g_device_params->ota_mode == OTA_UPDATE_MODE_FORMAL)
			{
				*(uint32_t*)_value = OTA_UPDATE_MODE_FORMAL;
			}
			else if (g_device_params->ota_mode == OTA_UPDATE_MODE_TEST)
			{
				*(uint32_t*)_value = OTA_UPDATE_MODE_TEST;
			}
			else
			{
				*(uint32_t*)_value = OTA_UPDATE_MODE_FORMAL;
			}
			break;
		}
		case FLASH_CFG_ASR_MODE:
		{
			//*(uint32_t*)_value = g_asr_mode;
			break;
		}
		case FLASH_CFG_APP_ID:
		{
			strcpy((char*)_value, DEEP_BRAIN_APP_ID);
			break;
		}
		case FLASH_CFG_ROBOT_ID:
		{
			strcpy((char*)_value, DEEP_BRAIN_ROBOT_ID);
			break;
		}
		case FLASH_CFG_DEVICE_LICENSE:
		{
			memcpy(_value, &g_device_params->basic_info, sizeof(g_device_params->basic_info));
			break;
		}
		case FLASH_CFG_END:
		{
			break;
		}
		default:
			break;
	}
	SEMPHR_TRY_UNLOCK(g_lock_flash_config);

	return true;
}

bool get_dcl_auth_params(DCL_AUTH_PARAMS_t *dcl_auth_params)
{
	if (dcl_auth_params == NULL)
	{
		return false;
	}

	snprintf(dcl_auth_params->str_app_id, sizeof(dcl_auth_params->str_app_id), "%s", DEEP_BRAIN_APP_ID);
	snprintf(dcl_auth_params->str_robot_id, sizeof(dcl_auth_params->str_robot_id), "%s", DEEP_BRAIN_ROBOT_ID);
	get_flash_cfg(FLASH_CFG_USER_ID, dcl_auth_params->str_user_id);
	get_flash_cfg(FLASH_CFG_DEVICE_ID, dcl_auth_params->str_device_id);
	snprintf(dcl_auth_params->str_city_name, sizeof(dcl_auth_params->str_city_name), " ");

	return true;
}

bool set_flash_cfg(FLASH_CONFIG_PARAMS_T _params, void *_value)
{
	bool ret = false;

	SEMPHR_TRY_LOCK(g_lock_flash_config);
	switch (_params)
	{
		case FLASH_CFG_START:
		{
			break;
		}
		case FLASH_CFG_FLASH_MUSIC_INDEX:
		{
			break;
		}
		case FLASH_CFG_SDCARD_FOLDER_INDEX:
		{
			break;
		}
		case FLASH_CFG_AUTH_INFO:
		{
			memcpy((char*)&g_device_params->auth_info, (char*)_value, sizeof(g_device_params->auth_info));
			ret = save_device_params();			
			if (!ret)
			{
				DEBUG_LOGE(LOG_TAG, "save_device_params FLASH_CFG_AUTH_INFO fail");
			}			
			break;
		}
		case FLASH_CFG_WIFI_INFO:
		{
			if (memcmp((char*)&g_device_params->wifi_infos, (char*)_value, sizeof(g_device_params->wifi_infos)) != 0)
			{
				memcpy((char*)&g_device_params->wifi_infos, (char*)_value, sizeof(g_device_params->wifi_infos));
				ret = save_device_params();
				if (!ret)
				{
					DEBUG_LOGE(LOG_TAG, "save_device_params FLASH_CFG_WIFI_INFO fail");
				}
			}
			break;
		}
		case FLASH_CFG_USER_ID:
		{
			if (strcmp(g_device_params->basic_info.bind_user_id, (char*)_value) != 0)
			{
				snprintf((char*)&g_device_params->basic_info.bind_user_id, sizeof(g_device_params->basic_info.bind_user_id),
					"%s", (char*)_value);
				ret = save_device_params();
				if (!ret)
				{
					DEBUG_LOGI(LOG_TAG, "save_device_params FLASH_CFG_USER_ID fail[0x%x]", ret);
				}
			}
			break;
		}
		case FLASH_CFG_DEVICE_ID:
		{
			if (strcmp(g_device_params->basic_info.device_sn, (char*)_value) != 0)
			{
				snprintf((char*)&g_device_params->basic_info.device_sn, sizeof(g_device_params->basic_info.device_sn),
					"%s", (char*)_value);
				ret = save_device_params();
				if (!ret)
				{
					DEBUG_LOGE(LOG_TAG, "save_device_params FLASH_CFG_DEVICE_ID fail[0x%x]", ret);
				}
			}
			break;
		}
		case FLASH_CFG_MEMO_PARAMS:
		{	
		#ifndef __NOT_USE_FLSAH__
			memcpy((MEMO_ARRAY_T *)&g_device_params->memo_array, (MEMO_ARRAY_T *)_value, sizeof(MEMO_ARRAY_T));
			ret = save_device_params();
		#endif
			if (!ret)
			{
				DEBUG_LOGE(LOG_TAG, "save_device_params FLASH_CFG_MEMO_PARAMS fail");
			}
		}
		case FLASH_CFG_OTA_MODE:
		{
			if (g_device_params->ota_mode != *(uint32_t*)_value)
			{
				if (*(uint32_t*)_value == OTA_UPDATE_MODE_FORMAL)
				{
					g_device_params->ota_mode = OTA_UPDATE_MODE_FORMAL;
				}
				else if (*(uint32_t*)_value == OTA_UPDATE_MODE_TEST)
				{
					g_device_params->ota_mode = OTA_UPDATE_MODE_TEST;
				}
				else
				{
					g_device_params->ota_mode = OTA_UPDATE_MODE_FORMAL;
				}
				
				ret = save_device_params();
				if (!ret)
				{
					DEBUG_LOGI(LOG_TAG, "save_device_params FLASH_CFG_OTA_MODE fail");
				}
			}
			break;
		}
		case FLASH_CFG_ASR_MODE:
		{
			break;
		}
		case FLASH_CFG_DEVICE_LICENSE:
		{
			if (memcmp((char*)&g_device_params->basic_info, (char*)_value, sizeof(g_device_params->basic_info)) != 0)
			{
				memcpy((char*)&g_device_params->basic_info, (char*)_value, sizeof(g_device_params->basic_info));
				ret = save_device_params();
				if (!ret)
				{
					DEBUG_LOGE(LOG_TAG, "save_device_params FLASH_CFG_DEVICE_LICENSE fail");
				}
			}
			break;
		}
		case FLASH_CFG_END:
		{
			break;
		}
		default:
			break;
	}
	SEMPHR_TRY_UNLOCK(g_lock_flash_config);
	
	return ret;
}

void init_device_params_v1(DEVICE_PARAMS_CONFIG_T *_config)
{
	memset(_config, 0, sizeof(DEVICE_PARAMS_CONFIG_T));
	_config->params_version = DEVICE_PARAMS_VERSION_1;
	snprintf(_config->wifi_infos.wifi_info[0].wifi_ssid, sizeof(_config->wifi_infos.wifi_info[0].wifi_ssid),
		WIFI_SSID_DEFAULT);
	snprintf(_config->wifi_infos.wifi_info[0].wifi_passwd, sizeof(_config->wifi_infos.wifi_info[0].wifi_passwd),
		WIFI_PASSWD_DEFAULT);
}

bool init_device_params(void)
{
	SEMPHR_CREATE_LOCK(g_lock_flash_config);

	g_device_params = (DEVICE_PARAMS_CONFIG_T *)memory_malloc(sizeof(DEVICE_PARAMS_CONFIG_T));
	if (g_device_params == NULL)
	{
		DEBUG_LOGE(LOG_TAG, "memory_malloc g_device_params fail");
		return false;
	}
	memset(g_device_params, 0, sizeof(DEVICE_PARAMS_CONFIG_T));
	
	flash_op_read((uint8_t*)g_device_params, sizeof(DEVICE_PARAMS_CONFIG_T));
	if (get_device_params_crc(g_device_params) != g_device_params->device_params_header.crc32)
	{
		DEBUG_LOGE(LOG_TAG, "g_device_params crc have a change!");
		memset(g_device_params, 0, sizeof(DEVICE_PARAMS_CONFIG_T));
	}
	DEBUG_LOGE(LOG_TAG, "curr crc is [0x%0x] !", get_device_params_crc(g_device_params));

	if (g_device_params->params_version != DEVICE_PARAMS_VERSION_1) 
	{
		init_device_params_v1(g_device_params);
		save_device_params();
	}

	print_device_params();
	
	return true;
}

bool init_default_params(void)
{
	if (g_device_params == NULL)
	{
		return false;
	}

	init_device_params_v1(g_device_params);
	save_device_params();

	return true;
}

