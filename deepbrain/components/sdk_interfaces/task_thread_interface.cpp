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

#include "task_thread_interface.h"
#include "mbed.h"
#include <Thread.h>
#include <stdio.h>
void * task_thread_create(
	void (*task_func)(void), 
	const char * const task_name,
	const uint32_t static_size,
	void * const task_params,
	const int32_t task_priority)
{
	rtos::Thread *new_thread = new rtos::Thread((osPriority)task_priority, static_size);
	
	if (new_thread != NULL)
	{
		new_thread->start(task_func);
		return new_thread;
	}

	return NULL;
}

void task_thread_exit(void * pThread)
{
	if(pThread)
	{
		printf("%d,delete thread \r\n",((rtos::Thread *)pThread)->get_state());
		delete (rtos::Thread *)pThread;
	}
	pThread = NULL;
	return;
}

void task_thread_sleep(const uint32_t ms)
{
	rtos::Thread::wait(ms);
	
	return;
}

