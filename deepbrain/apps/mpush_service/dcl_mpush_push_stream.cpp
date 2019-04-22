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

#include <string.h>
#include "dcl_mpush_push_stream.h"
#include "http_api.h"
#include "auth_crypto.h"
#include "socket_interface.h"
#include "cJSON.h"
#include "debug_log_interface.h"
#include "memory_interface.h"
#include "time_interface.h"

static const char *TAG_LOG = "[DCL MPUSH STREAM]";

static DCL_ERROR_CODE_t dcl_mpush_session_begin(
	DCL_MPUSH_PUSH_STREAM_HANDLER_t **handler)
{
	DCL_MPUSH_PUSH_STREAM_HANDLER_t *new_handler = NULL;
	
	if (handler == NULL)
	{
		DEBUG_LOGE(TAG_LOG, "dcl_mpush_session_begin invalid params");
		return DCL_ERROR_CODE_SYS_INVALID_PARAMS;
	}
	
	new_handler = (DCL_MPUSH_PUSH_STREAM_HANDLER_t *)memory_malloc(sizeof(DCL_MPUSH_PUSH_STREAM_HANDLER_t));
	if (new_handler == NULL)
	{
		DEBUG_LOGE(TAG_LOG, "dcl_mpush_session_begin malloc failed");
		return DCL_ERROR_CODE_SYS_NOT_ENOUGH_MEM;
	}
	memset(new_handler, 0, sizeof(DCL_MPUSH_PUSH_STREAM_HANDLER_t));
	*handler = new_handler;
	new_handler->sock = INVALID_SOCK;

	if (sock_get_server_info(MPUSH_SEND_STREAM_MSG_URL, 
		(char*)&new_handler->domain, (char*)&new_handler->port, (char*)&new_handler->params) != 0)
	{
		DEBUG_LOGE(TAG_LOG, "sock_get_server_info failed");
		return DCL_ERROR_CODE_NETWORK_DNS_FAIL;
	}

	new_handler->sock = sock_connect(new_handler->domain, new_handler->port);
	if (new_handler->sock == INVALID_SOCK)
	{
		DEBUG_LOGE(TAG_LOG, "sock_connect fail,[%s:%s]", 
			new_handler->domain, new_handler->port);
		return DCL_ERROR_CODE_NETWORK_UNAVAILABLE;
	}
	sock_set_nonblocking(new_handler->sock);
	
	return DCL_ERROR_CODE_OK;
}

static DCL_ERROR_CODE_t dcl_mpush_session_end(
	DCL_MPUSH_PUSH_STREAM_HANDLER_t *handler)
{
	if (handler == NULL)
	{
		return DCL_ERROR_CODE_SYS_INVALID_PARAMS;
	}

	//free socket
	if (handler->sock != INVALID_SOCK)
	{
		sock_close(handler->sock);
		handler->sock = INVALID_SOCK;
	}

	//free json object
	if (handler->json_body != NULL)
	{
		cJSON_Delete((cJSON *)handler->json_body);
		handler->json_body = NULL;
	}
	
	//free memory
	memory_free(handler);
	handler = NULL;

	return DCL_ERROR_CODE_OK;
}

static DCL_ERROR_CODE_t dcl_mpush_make_packet(
	DCL_MPUSH_PUSH_STREAM_HANDLER_t *handler,
	const DCL_AUTH_PARAMS_t* const input_params)
{	
	//make header string
	crypto_generate_nonce((uint8_t *)handler->str_nonce, sizeof(handler->str_nonce));
	crypto_time_stamp((unsigned char*)handler->str_timestamp, sizeof(handler->str_timestamp));
	crypto_generate_private_key((uint8_t *)handler->str_private_key, sizeof(handler->str_private_key), 
			handler->str_nonce, 
			handler->str_timestamp, 
			input_params->str_robot_id);
	
	snprintf(handler->req_header, sizeof(handler->req_header),
		"POST %s HTTP/1.1\r\n"
		"Host: %s:%s\r\n"
		"Accept: application/json\r\n"
		"Accept-Language: zh-cn\r\n"
		"Content-Type: application/json\r\n"
		"Transfer-Encoding: chunked\r\n"
		"Nonce: %s\r\n"
		"CreatedTime: %s\r\n"
		"PrivateKey: %s\r\n"
		"Key: %s\r\n"
		"x-device-id: %s\r\n"
		"x-app-id: %s\r\n"
		"x-robot-id: %s\r\n"
		"Connection:close\r\n\r\n", 
		handler->params, 
		handler->domain, 
		handler->port, 
		handler->str_nonce, 
		handler->str_timestamp, 
		handler->str_private_key, 
		input_params->str_robot_id,
		input_params->str_device_id,
		input_params->str_app_id,
		input_params->str_robot_id);

	return DCL_ERROR_CODE_OK;
}

static DCL_ERROR_CODE_t dcl_mpush_decode_packet(
	DCL_MPUSH_PUSH_STREAM_HANDLER_t *handler)
{
	if (http_get_error_code(handler->req_header) == 200)
	{	
		char* pBody = http_get_body(handler->req_header);
		if (pBody != NULL)
		{	
			if (strstr(pBody, "\"statusCode\":\"OK\""))
			{
				return DCL_ERROR_CODE_OK;
			}
			else
			{
				DEBUG_LOGE(TAG_LOG, "invalid body[%s]", pBody);
				return DCL_ERROR_CODE_NETWORK_POOR;
			}
		}
	}
	else
	{
		DEBUG_LOGE(TAG_LOG, "http reply error[%s]", handler->req_header);
		return DCL_ERROR_CODE_NETWORK_POOR;
	}

	return DCL_ERROR_CODE_NETWORK_POOR;
}

DCL_ERROR_CODE_t dcl_mpush_push_stream_create(
	void **handler,
	const DCL_AUTH_PARAMS_t* const input_params)
{
	int ret = 0;
	DCL_MPUSH_PUSH_STREAM_HANDLER_t *push_handler = NULL;
	DCL_ERROR_CODE_t err_code = DCL_ERROR_CODE_OK;
	uint64_t start_time = get_time_of_day();

	if (input_params == NULL
		|| handler == NULL)
	{
		return DCL_ERROR_CODE_SYS_INVALID_PARAMS;
	}

	//建立session
	err_code = dcl_mpush_session_begin(&push_handler);
	if (err_code != DCL_ERROR_CODE_OK)
	{
		DEBUG_LOGE(TAG_LOG, "dcl_mpush_session_begin failed");
		return err_code;
	}

	//组包
	err_code = dcl_mpush_make_packet(push_handler, input_params);
	if (err_code != DCL_ERROR_CODE_OK)
	{
		DEBUG_LOGE(TAG_LOG, "dcl_mpush_make_packet failed");
		dcl_mpush_session_end(push_handler);
		return err_code;
	}
	
	//发包
	if (sock_writen_with_timeout(push_handler->sock, push_handler->req_header, strlen(push_handler->req_header), 2000) != strlen(push_handler->req_header)) 
	{
		DEBUG_LOGE(TAG_LOG, "sock_writen_with_timeout failed");
		err_code = DCL_ERROR_CODE_NETWORK_POOR;
		dcl_mpush_session_end(push_handler);
		return err_code;
	}

	*handler = push_handler;
	
	return DCL_ERROR_CODE_OK;
}

DCL_ERROR_CODE_t dcl_mpush_push_stream(
	void *handler,
	const char *data, 
	const uint32_t data_len)
{
	int ret = 0;
	DCL_MPUSH_PUSH_STREAM_HANDLER_t *push_handler = (DCL_MPUSH_PUSH_STREAM_HANDLER_t *)handler;
	DCL_ERROR_CODE_t err_code = DCL_ERROR_CODE_OK;
	char str_chunk_len[32]= {0};

	if (data == NULL
		&& data_len == 0)
	{
		snprintf(str_chunk_len, sizeof(str_chunk_len), "0\r\n\r\n");
	}
	else
	{
		snprintf(str_chunk_len, sizeof(str_chunk_len), "%x\r\n", data_len);
	}

	//发包1
	if (sock_writen_with_timeout(push_handler->sock, str_chunk_len, strlen(str_chunk_len), 1000) != strlen(str_chunk_len)) 
	{
		DEBUG_LOGE(TAG_LOG, "sock_writen_with_timeout failed");
		err_code = DCL_ERROR_CODE_NETWORK_POOR;
		goto dcl_mpush_push_stream_error;
	}

	if (data_len > 0)
	{
		//发包2
		if (sock_writen_with_timeout(push_handler->sock, data, data_len, 3000) != data_len) 
		{
			DEBUG_LOGE(TAG_LOG, "sock_writen_with_timeout failed");
			err_code = DCL_ERROR_CODE_NETWORK_POOR;
			goto dcl_mpush_push_stream_error;
		}

		//发包3
		if (sock_writen_with_timeout(push_handler->sock, "\r\n", strlen("\r\n"), 1000) != strlen("\r\n")) 
		{
			DEBUG_LOGE(TAG_LOG, "sock_writen_with_timeout failed");
			err_code = DCL_ERROR_CODE_NETWORK_POOR;
			goto dcl_mpush_push_stream_error;
		}
		DEBUG_LOGE(TAG_LOG, "data_len = %d",data_len);
	}
	else
	{
		//接包
		memset(push_handler->req_header, 0, sizeof(push_handler->req_header));
		ret = sock_readn_with_timeout(push_handler->sock, push_handler->req_header, sizeof(push_handler->req_header) - 1, 2000);
		if (ret <= 0)
		{
			DEBUG_LOGE(TAG_LOG, "sock_readn_with_timeout failed");
			err_code = DCL_ERROR_CODE_NETWORK_POOR;
			goto dcl_mpush_push_stream_error;
		}

		printf("[%s]", push_handler->req_header);
		//解包
		err_code = dcl_mpush_decode_packet((DCL_MPUSH_PUSH_STREAM_HANDLER_t *)handler);
		if (err_code != DCL_ERROR_CODE_OK)
		{
			DEBUG_LOGE(TAG_LOG, "dcl_mpush_decode_packet failed");
			goto dcl_mpush_push_stream_error;
		}
	}

	return DCL_ERROR_CODE_OK;
	
dcl_mpush_push_stream_error:

	//销毁session
	if (dcl_mpush_session_end((DCL_MPUSH_PUSH_STREAM_HANDLER_t *)handler) != DCL_ERROR_CODE_OK)
	{
		DEBUG_LOGE(TAG_LOG, "dcl_mpush_session_end failed");
	}
	
	return err_code;
}


DCL_ERROR_CODE_t dcl_mpush_push_stream_delete(
	void *handler)
{
	return dcl_mpush_session_end((DCL_MPUSH_PUSH_STREAM_HANDLER_t*)handler);
}

