#ifndef __MDNN_LOG_H__
#define __MDNN_LOG_H__

#include <stdio.h>
#include <string.h>
#include <time.h>

#ifdef __MDNN_LOG_VERBOSE__
    #ifndef __UVISION_VERSION
        #define MDNN_LOG_VERBOSE(format, arg...) \
            { \
                char format_str[512]; \
                sprintf(format_str, format, ##arg); \
                time_t timep; \
                struct tm * timeinfo; \
                time (&timep); \
                timeinfo = localtime ( &timep); \
                printf("INTERNAL LOG-VERBOSE-TIME:%s - FILE:%s LINE:%d FUNC:%s] == %s\n", asctime(timeinfo), __FILE__, __LINE__, __func__, format_str); \
            }
    #else
        #define MDNN_LOG_VERBOSE(format, arg...) printf(format, ##arg); printf("\n")
    #endif
#else
    #define MDNN_LOG_VERBOSE(format, arg...)
#endif

#if defined __MDNN_LOG_INFO__ || defined __MDNN_LOG_VERBOSE__
    #ifndef __UVISION_VERSION
        #define MDNN_LOG_INFO(format, arg...) \
            { \
                char format_str[512]; \
                sprintf(format_str, format, ##arg); \
                time_t timep; \
                struct tm * timeinfo; \
                time (&timep); \
                timeinfo = localtime ( &timep); \
                printf("INTERNAL LOG-INFO-TIME:%s - FILE:%s LINE:%d FUNC:%s] == %s\n", asctime(timeinfo), __FILE__, __LINE__, __func__, format_str); \
            }
    #else
        #define MDNN_LOG_INFO(format, arg...) printf(format, ##arg); printf("\n")
    #endif
#else
    #define MDNN_LOG_INFO(format, arg...)
#endif

#define MDNN_PRINTF_DIRECT(format, arg...) printf(format, ##arg)

#endif
