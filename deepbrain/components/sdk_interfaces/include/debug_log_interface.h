/* debug_log.h
** debug log api
**
** Copyright (C) 2005-2009 Collecta, Inc. 
**
**  This software is provided AS-IS with no warranty, either express
**  or implied.
**
**  This program is dual licensed under the MIT and GPLv3 licenses.
*/

#ifndef __DEBUG_LOG_INTERFACE_H__
#define __DEBUG_LOG_INTERFACE_H__

#include "us_ticker_api.h"
#include "lightduer_log.h"
#include "semaphore_lock_interface.h"
#ifdef __cplusplus
extern "C" {
#endif /* C++ */

#define ENABLE_DEBUG_LOG 1

#if defined(ENABLE_DEBUG_LOG)
#define DEBUG_LOG(tag,_level, ...)     \
//    printf("[%s][%s][%lu] %s(%d): %s\r\n", tag, _level, us_ticker_read(), __FILE__, __LINE__, ##__VA_ARGS__)
//    mbed_error_printf(__VA_ARGS__), \
//    mbed_error_printf("\n")
//    duer_log_report(_level,__VA_ARGS__);
#endif

#if ENABLE_DEBUG_LOG == 1
#define DEBUG_LOGE( tag,  ... )   DUER_LOGE( __VA_ARGS__) 
#define DEBUG_LOGW( tag,  ... )   DUER_LOGW( __VA_ARGS__) 
#define DEBUG_LOGI( tag,  ... )   DUER_LOGI( __VA_ARGS__) 
#define DEBUG_LOGD( tag,  ... )   DUER_LOGD( __VA_ARGS__) 
#define DEBUG_LOGV( tag,  ... )   DUER_LOGV( __VA_ARGS__) 
#else
#define DEBUG_LOGE( tag, format, ... )
#define DEBUG_LOGW( tag, format, ... )
#define DEBUG_LOGI( tag, format, ... )
#define DEBUG_LOGD( tag, format, ... ) 
#define DEBUG_LOGV( tag, format, ... )
#endif

#ifdef __cplusplus
} /* extern "C" */	
#endif /* C++ */

#endif /* DEBUG_LOG_INTERFACE_H */
