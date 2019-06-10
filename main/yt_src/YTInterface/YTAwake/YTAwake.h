/*********************************************************************************
  *Copyright(C),http://www.youngtone.cn
  *FileName:  YTAwake.h
  *Author:  Àî¾ý
  *Version:  v1.0
  *Date:  2018.11.11
**********************************************************************************/
#ifndef __YT_AWAKE_H__
#define __YT_AWAKE_H__

#include "app_framework.h"




APP_FRAMEWORK_ERRNO_t yt_awake_start();
APP_FRAMEWORK_ERRNO_t yt_awake_stop();
APP_FRAMEWORK_ERRNO_t yt_awake_create(const int task_priority);
APP_FRAMEWORK_ERRNO_t yt_awake_delete(void);
APP_FRAMEWORK_ERRNO_t yt_awake_destory(void);


#endif
