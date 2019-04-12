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

#ifndef TASK_THREAD_INTERFACE_H
#define TASK_THREAD_INTERFACE_H

#include "ctypes_interface.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * The SpeakerInterface is concerned with the control of volume and mute settings of a speaker.
 * The two settings are independent of each other, and the respective APIs shall not affect
 * the other setting in any way. Compound behaviors (such as unmuting when volume is adjusted) will
 * be handled at a layer above this interface.
 */

/**
 * Set the absolute volume of the speaker. @c volume
 * will be [AVS_SET_VOLUME_MIN, AVS_SET_VOLUME_MAX], and implementers of the interface must normalize
 * the volume to fit the needs of their drivers.
 *
 * @param volume A volume to set the speaker to.
 * @return Whether the operation was successful.
 */
void * task_thread_create(
	void (*task_func)(void), 
	const char * const task_name,
	const uint32_t static_size,
	void * const task_params,
	const int32_t task_priority);

/**
 * Get the absolute volume of the speaker. @c volume
 * will be [AVS_SET_VOLUME_MIN, AVS_SET_VOLUME_MAX], and implementers of the interface must normalize
 * the volume to fit the needs of their drivers.
 *
 * @param volume A volume get from the speaker.
 * @return Whether the operation was successful.
 */
void task_thread_exit(void * pThread = NULL);

/**
 * Set the mute of the speaker.
 *
 * @param mute Represents whether the speaker should be muted (true) or unmuted (false).
 * @return Whether the operation was successful.
 */
void task_thread_sleep(const uint32_t ms);

#ifdef __cplusplus
}
#endif

#endif  //TASK_THREAD_INTERFACE_H

