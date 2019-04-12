// Copyright 2017 Baidu Inc. All Rights Reserved.
// Author: Zhang Leliang (zhangleliang@baidu.com)
//
// Description: Measure memory statistic

#ifndef BAIDU_IOT_TINYDU_IOT_OS_SRC_UTILITY_BAIDU_MEASURE_MEMORY_H
#define BAIDU_IOT_TINYDU_IOT_OS_SRC_UTILITY_BAIDU_MEASURE_MEMORY_H

#ifdef BAIDU_STACK_MONITOR

#include "mbed.h"

#ifdef __cpluscplus
extern "C" {
#endif

/**
  *this function try to print out the heap statictis using UART
  *requirement:
  * Add the command-line flag -DMBED_HEAP_STATS_ENABLED=1.
*/
void get_heap_usage();

/*
 * @param thread
 * @name the name of the thread
 */
bool register_thread(rtos::Thread* thread, const char* name);

bool unregister_thread(rtos::Thread* thread);

/**
  *this function try to print out the stack statictis using UART
  *requirement:
  * only print the thread register by register_thread
*/
void get_current_thread_stack_usage();

void get_current_thread_stack_usage(rtos::Thread* thread, const char* name);

/**
  *this function try to print out the stack statictis using UART
  *requirement:
  * add the command-line flag -DMBED_STACK_STATS_ENABLED=1.
  * NOT SUPPORT NOW (need higher version of mbed-os)
*/
void get_stack_usage();

#ifdef __cpluscplus
}
#endif

#endif // BAIDU_STACK_MONITOR

#endif // BAIDU_IOT_TINYDU_IOT_OS_SRC_UTILITY_BAIDU_MEASURE_MEMORY_H
