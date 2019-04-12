//
//  authorize.h
//  protocol version 5.0
//
//  Created by ustudy on 19/1/15.
//  ustudy. All rights reserved.
//

#ifndef authorize_h
#define authorize_h
#include "app_config.h"

typedef enum
{
	AUTH_STAUTS_IDLE = 0,
	AUTH_STAUTS_INIT,
	AUTH_STAUTS_WAIT,
	AUTH_STAUTS_SUCCESS,
}AUTH_STATUS_t;

bool youngtone_is_authorized();

APP_FRAMEWORK_ERRNO_t authorize_service_create(const int task_priority);
APP_FRAMEWORK_ERRNO_t authorize_service_delete(void);

#endif /* authorize_h */
