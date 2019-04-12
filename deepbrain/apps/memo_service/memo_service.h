#ifndef MEMO_SERVICE_H
#define MEMO_SERVICE_H

#include "app_config.h"
#include "lightduer_timers.h"

#define MEMO_EVEVT_MAX 3

typedef enum  
{
	MEMO_IDLE,
	MEMO_INIT,
	MEMO_ADD,
	MEMO_LOOP,
}MEMO_STATUS_T; 

// 68字节
typedef struct 
{	
	duer_timer_handler timer;
	bool is_valid;
	uint32_t set_time;
	uint32_t time;
	char str[64];
}MEMO_EVENT_T;

//684字节
typedef struct 
{	
	MEMO_EVENT_T event[MEMO_EVEVT_MAX];
}MEMO_ARRAY_T;

typedef struct 
{	
	MEMO_ARRAY_T  memo_array;
	MEMO_STATUS_T memo_status;
}MEMO_SERVICE_HANDLE_T;

/**
 * create memo service
 *
 * @param task_priority, the priority of running thread
 * @return app framework errno
 */
extern APP_FRAMEWORK_ERRNO_t memo_service_create(const int task_priority);


/**
 * show memo
 *
 * @param none
 * @return none
 */
extern void show_memo(void);

/**
 * get memo result
 *
 * @param str_body, string body 
 * @return int
 */
extern int get_memo_result(char *str_body);
#endif
