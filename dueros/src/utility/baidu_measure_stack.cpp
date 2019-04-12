// Copyright 2017 Baidu Inc. All Rights Reserved.
// Author: Zhang Leliang (zhangleliang@baidu.com)
//
// Description: Measure stack statistic

#ifdef BAIDU_STACK_MONITOR

#include "baidu_measure_stack.h"
#include "lightduer_lib.h"
#include "mbed_stats.h"
#include "heap_monitor.h"
#include "lightduer_log.h"

struct thread_info {
    rtos::Thread* thread;
    char* name;
    thread_info* next;
};

static thread_info* s_header = NULL;
static rtos::Mutex s_header_mutex;

void get_heap_usage() {
    mbed_stats_heap_t heap_stats;
    mbed_stats_heap_get(&heap_stats);
    mbed_error_printf("current_size: %lu \n", heap_stats.current_size);
    mbed_error_printf("max_size: %lu \n", heap_stats.max_size);
    mbed_error_printf("total_size: %lu \n", heap_stats.total_size);
    mbed_error_printf("alloc_cnt: %lu \n", heap_stats.alloc_cnt);
    mbed_error_printf("alloc_fail_cnt: %lu \n", heap_stats.alloc_fail_cnt);
}

bool register_thread(rtos::Thread* thread, const char* name) {
    thread_info* new_thread_info = (thread_info*)MALLOC(sizeof(thread_info), UTILITY);
    if (new_thread_info == NULL) {
        DUER_LOGW("malloc fail!!");
        return false;
    }
    size_t name_len = DUER_STRLEN(name);
    char* new_name = NULL;
    if (name_len > 0) {
        new_name = (char*)MALLOC(DUER_STRLEN(name) + 1, UTILITY);
        DUER_MEMSET(new_name, 0, DUER_STRLEN(name) + 1);
        DUER_MEMCPY(new_name, name, DUER_STRLEN(name));
    }
    new_thread_info->thread = thread;
    new_thread_info->name = new_name;
    new_thread_info->next = s_header;
    s_header_mutex.lock();
    s_header = new_thread_info;
    s_header_mutex.unlock();
    return true;
}

bool unregister_thread(rtos::Thread* thread) {
    if (s_header == NULL) {
        return false;
    }
    get_current_thread_stack_usage();

    s_header_mutex.lock();
    thread_info* pre = s_header;
    if (s_header->thread == thread) {
        s_header = s_header->next;
        FREE(pre->name);
        pre->name = NULL;
        FREE(pre);
        s_header_mutex.unlock();
        return true;
    }
    thread_info* current = s_header->next;
    while(current && current->thread != thread) {
        pre = current;
        current = current->next;
    }
    if (current) {
        pre->next = current->next;
        FREE(current->name);
        current->name = NULL;
        FREE(current);
        s_header_mutex.unlock();
        return true;
    } else {
        s_header_mutex.unlock();
        return false;
    }
}

void get_current_thread_stack_usage() {
    if (s_header == NULL) {
        return;
    }
    mbed_error_printf("======begin Thread stack info========\n");
    s_header_mutex.lock();
    thread_info* current = s_header;
    while(current) {
        get_current_thread_stack_usage(current->thread, current->name);
        current = current->next;
    }
    s_header_mutex.unlock();
    mbed_error_printf("======end Thread stack info========\n");

#if 0 // NOT SUPPORT NOW (need higher version of mbed-os)
    osThreadId main_id = osThreadGetId();

    osEvent info;
    info = _osThreadGetInfo(main_id, osThreadInfoStackSize);
    if (info.status != osOK) {
        mbed_error_printf("Could not get stack size");
        return;
    }
    uint32_t stack_size = (uint32_t)info.value.v;
    info = _osThreadGetInfo(main_id, osThreadInfoStackMax);
    if (info.status != osOK) {
        mbed_error_printf("Could not get max stack");
        return
    }
    uint32_t max_stack = (uint32_t)info.value.v;

    mbed_error_printf("Stack used %li of %li bytes\r\n", max_stack, stack_size);
#endif
}


void get_current_thread_stack_usage(rtos::Thread* thread, const char* name) {
    mbed_error_printf("thread: %s, stack size: %d, free: %d, used: %d, max: %d \n",
                          name,
                          thread->stack_size(), thread->free_stack(),
                          thread->used_stack(), thread->max_stack());
}

void get_stack_usage() {
    DUER_LOGW("not impl !!");
#if 0 // NOT SUPPORT NOW (need higher version of mbed-os)
    int cnt = osThreadGetCount();
    mbed_stats_stack_t *stats = (mbed_stats_stack_t*) malloc(cnt * sizeof(mbed_stats_stack_t));

    cnt = mbed_stats_stack_get_each(stats, cnt);
    for (int i = 0; i < cnt; i++) {
        mbed_error_printf("Thread: 0x%X, Stack size: %u, Max stack: %u\r\n",
            stats[i].thread_id, stats[i].reserved_size, stats[i].max_size);
    }
#endif
}

#endif // BAIDU_STACK_MONITOR
