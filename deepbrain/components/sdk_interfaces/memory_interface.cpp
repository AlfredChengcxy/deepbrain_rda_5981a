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
#include <stdlib.h>
#include "mbed.h"
#include "rtos.h"
#include "memory_interface.h"
#include <debug_log_interface.h>

#define LOG_TAG "memory"

void *memory_malloc(const uint32_t mem_size)
{
	return malloc(mem_size);
}

void memory_free(const void *mem_addr)
{
	if (mem_addr != NULL)
	{
		free((void *)mem_addr);
	}
}

void memory_info(void)
{
	__heapstats((__heapprt)fprintf,stderr);
    __heapvalid((__heapprt) fprintf, stderr, 1);
	return;
}

