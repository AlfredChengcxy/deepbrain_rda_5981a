// Copyright 2017 Baidu Inc. All Rights Reserved.
// Author: Gang Chen (chengang12@baidu.com)
//
// Description: This file is wrapper for OS mutex

#include "baidu_iot_mutex.h"
#include "rtos.h"
#include "lightduer_log.h"

iot_mutex_t iot_mutex_create(void) {
    Mutex* mutex = NULL;
    mutex = new Mutex();

    if (!mutex) {
        DUER_LOGE("Failed to create mutex: no memory\n");
    }

    return mutex;
}

iot_mutex_status iot_mutex_lock(iot_mutex_t mutex, uint32_t timeout_ms) {
    osStatus ret = osOK;

    if (!mutex) {
        return IOT_MUTEX_ERROR;
    }

    ret = ((Mutex*)mutex)->lock(timeout_ms);

    switch (ret) {
    case osOK:
        return IOT_MUTEX_OK;

    case osErrorTimeoutResource:
        return IOT_MUTEX_TIMEOUT;

    default:
        return IOT_MUTEX_ERROR;
    }
}

bool iot_mutex_try_lock(iot_mutex_t mutex) {
    if (!mutex) {
        return IOT_MUTEX_ERROR;
    }

    return ((Mutex*)mutex)->trylock();
}

iot_mutex_status iot_mutex_unlock(iot_mutex_t mutex) {
    if (!mutex) {
        return IOT_MUTEX_ERROR;
    }

    ((Mutex*)mutex)->unlock();
    return IOT_MUTEX_OK;
}

iot_mutex_status iot_mutex_destroy(iot_mutex_t mutex) {
    if (!mutex) {
        return IOT_MUTEX_ERROR;
    }

    delete mutex;
    return IOT_MUTEX_OK;
}

