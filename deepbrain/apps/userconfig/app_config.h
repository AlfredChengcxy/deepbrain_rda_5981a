#ifndef APP_CONFIG_H
#define APP_CONFIG_H

#include "app_framework.h"
#include "semaphore_lock_interface.h"
#include "debug_log_interface.h"
#include "task_thread_interface.h"

//所有 app name 定义
#define APP_NAME_MAIN 						APP_MAIN_NAME
#define APP_NAME_MAIN_STACK_SIZE 			(1024) 

#define APP_NAME_SHELL 						"shell"
#define APP_NAME_SHELL_STACK_SIZE 			(1024) 

#define APP_NAME_HTTP_DOWN 					"http-download" 
#define APP_NAME_HTTP_DOWN_STACK_SIZE 		(512) 

#define APP_NAME_AUDIO_PLAYER 				"audio-player"
#define APP_NAME_AUDIO_PLAYER_STACK_SIZE 	(2*1024)

#define APP_NAME_MPUSH_SERVICE				"mpush-service"
#define APP_NAME_MPUSH_SERVICE_STACK_SIZE	(2560)
#define APP_NAME_AUTH_SERVICE				"auth-service"
#define APP_NAME_AUTH_SERVICE_STACK_SIZE	(2*1024)

#define APP_NAME_MPUSH_MESSAGE				"mpush-msg"
#define APP_NAME_MPUSH_MESSAGE_STACK_SIZE	(2*1024)

#define APP_NAME_WIFI_MANAGE				"wifi-manage"
#define APP_NAME_WIFI_MANAGE_STACK_SIZE		(1024*2)

#define APP_NAME_PLAY_LIST					"playlist-service"
#define APP_NAME_PLAY_LIST_STACK_SIZE		(1024)

#define APP_NAME_AIP_SERVICE				"aip-service"
#define APP_NAME_AIP_SERVICE_STACK_SIZE		(1024)

#define APP_NAME_OTA_SERVICE				"ota-service"
#define APP_NAME_OTA_SERVICE_STACK_SIZE		(3*1024)

#define APP_NAME_POWER_MANAGE               "power-manager"
#define APP_NAME_POWER_MANAGE_STACK_SIZE	(2*1024)

#define APP_NAME_ASR_SERVICE				"asr-service"
#define APP_NAME_ASR_SERVICE_STACK_SIZE		(6*1024)

#define APP_NAME_WECHAT_SERVICE				"wechat-service"
#define APP_NAME_WECHAT_SERVICE_STACK_SIZE	(3*1024)

#define APP_NAME_WEIXIN_CLOUD_SERVICE				"weixin-cloud"
#define APP_NAME_WEIXIN_CLOUD_SERVICE_STACK_SIZE	(4*1024)

#define APP_NAME_WEIXIN_MSG							"weixin-msg"
#define APP_NAME_WEIXIN_MSG_STACK_SIZE				(2*1024)

#define APP_NAME_MEMO_MANAGE				        "memo-manage"
#define APP_NAME_MEMO_MANAGE_STACK_SIZE			    (3*1024)

#define APP_NAME_KEYBOARD_SERVICE				 	"keyboard-service"
#define APP_NAME_KEYBOARD_SERVICE_STACK_SIZE 		(3*1024)

#define APP_NAME_MISC_MANAGE                		"misc-manager"
#define APP_NAME_MISC_MANAGE_STACK_SIZE	    		(2*1024)

#define APP_NAME_CHAT_TALK                			"chat-talk"
#define APP_NAME_CHAT_TALK_STACK_SIZE	    		(1024)

#define APP_NAME_DISPLAY_CONTROL            		"display-control"
#define APP_NAME_DISPLAY_CONTROL_STACK_SIZE			(1*1024)

#define APP_NAME_AIRKISS_LAN_DISCOVERY				"airkiss-lan-discovery"
#define APP_NAME_AIRKISS_LAN_DISCOVERY_STACK_SIZE	(2*1024)

#define APP_NAME_SDCARD_MUSIC_MANAGE			 	"sdcard-music-manage"
#define APP_NAME_SDCARD_MUSIC_MANAGE_STACK_SIZE	 	(2*1024)

#define APP_NAME_FREE_TALK           				"free-talk"
#define APP_NAME_FREE_TALK_STACK_SIZE				(3*1024)

#define APP_TOTAL_STACK_SIZE (APP_NAME_MAIN_STACK_SIZE \
								+ APP_NAME_HTTP_DOWN_STACK_SIZE \
								+ APP_NAME_AUDIO_PLAYER_STACK_SIZE \
								+ APP_NAME_MPUSH_SERVICE_STACK_SIZE \
								+ APP_NAME_MPUSH_MESSAGE_STACK_SIZE \
								+ APP_NAME_WIFI_MANAGE_STACK_SIZE \
								+ APP_NAME_PLAY_LIST_STACK_SIZE \
								+ APP_NAME_AIP_SERVICE_STACK_SIZE \
								+ APP_NAME_OTA_SERVICE_STACK_SIZE \
								+ APP_NAME_POWER_MANAGE_STACK_SIZE \
								+ APP_NAME_ASR_SERVICE_STACK_SIZE \
								+ APP_NAME_MEMO_MANAGE_STACK_SIZE \
								+ APP_NAME_WECHAT_SERVICE_STACK_SIZE \
								+ APP_NAME_WEIXIN_CLOUD_SERVICE_STACK_SIZE \
								+ APP_NAME_KEYBOARD_SERVICE_STACK_SIZE \
								+ APP_NAME_MISC_MANAGE_STACK_SIZE \
								+ APP_NAME_CHAT_TALK_STACK_SIZE \
								+ APP_NAME_MISC_MANAGE_STACK_SIZE \
								+ APP_NAME_DISPLAY_CONTROL_STACK_SIZE \
								+ APP_NAME_AIRKISS_LAN_DISCOVERY_STACK_SIZE \
								+ APP_NAME_SDCARD_MUSIC_MANAGE_STACK_SIZE \
								+ APP_NAME_FREE_TALK_STACK_SIZE)
#endif /*APP_CONFIG_H*/

