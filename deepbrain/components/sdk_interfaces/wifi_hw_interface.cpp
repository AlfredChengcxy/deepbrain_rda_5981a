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
#include <string.h>
#include <stdlib.h>
#include "debug_log_interface.h"
#include "wifi_hw_interface.h"
#include "rda5991h_wland.h"

#define TAG_LOG "wifi get mac"

bool get_wifi_mac(uint8_t eth_mac[6])
{
	rda_get_macaddr(eth_mac, 0);
	
	return true;
}

bool get_wifi_mac_str(
	char* const str_mac_addr, 
	const uint32_t str_len)
{
	uint8_t eth_mac[6] = {0};

	get_wifi_mac(eth_mac);
	snprintf(str_mac_addr, str_len, "%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX", 
			 eth_mac[0],eth_mac[1],eth_mac[2],eth_mac[3],eth_mac[4],eth_mac[5]);

	//DEBUG_LOGI(TAG_LOG,"MAC:%s",str_mac_addr);
	return true;
}
	
