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

#ifndef SEMAPHONE_LOCK_INTERFACE_H
#define SEMAPHONE_LOCK_INTERFACE_H

#include <Mutex.h>

#define SEMPHR_CREATE_LOCK(mlock) 	do{mlock = new rtos::Mutex();}while(0)
#define SEMPHR_TRY_LOCK(mlock) 		do{if (mlock != NULL) {rtos::Mutex *lock_tmp = (rtos::Mutex *)mlock; lock_tmp->lock();}}while(0)
#define SEMPHR_TRY_UNLOCK(mlock) 	do{if (mlock != NULL) {rtos::Mutex *lock_tmp = (rtos::Mutex *)mlock; lock_tmp->unlock();}}while(0)
#define SEMPHR_DELETE_LOCK(mlock) 	do{if (mlock != NULL) {delete mlock;mlock=NULL;}}while(0)

#endif /*SEMAPHONE_LOCK_INTERFACE_H*/

