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

#include "flash_operation_interface.h"
#include "mbed.h"
#include "debug_log_interface.h"
#include "rda5991h_wland.h"

#define LOG_TAG "flash interface"

bool flash_op_read(
	uint8_t* const data, 
	uint32_t data_len)
{
	int rs = rda5981_flash_read_3rdparter_data_length();
	if(rs < 0 || rs > data_len)
	{
		DEBUG_LOGE(LOG_TAG, "rda5981_read_flash failed");
		return false;
	}	

	data_len = rs;

	rs = rda5981_flash_read_3rdparter_data((unsigned char*)data, data_len);
	if (rs < 0)
	{
		DEBUG_LOGE(LOG_TAG, "rda5981_read_flash failed");
		return false;
	}
	 
	return true;
}

bool flash_op_write(
	const uint8_t* const data, 
	const uint32_t data_len)
{
	int err = rda5981_flash_write_3rdparter_data((unsigned char*)data, data_len);
	if (err != 0)
	{
		DEBUG_LOGE(LOG_TAG, "rda5981_write_flash failed");
		return false;
	}
	 
	return true;
}

bool flash_op_erase()
{
	int err = rda5981_flash_erase_3rdparter_data();
	if (err != 0)
	{
		DEBUG_LOGE(LOG_TAG, "rda5981_erase_flash failed");
		return false;
	}
	 
	return true;
}

