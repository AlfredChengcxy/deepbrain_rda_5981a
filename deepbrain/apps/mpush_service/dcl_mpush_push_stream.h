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

#ifndef DCL_MPUSH_PUSH_STREAM_H
#define DCL_MPUSH_PUSH_STREAM_H
 
#include "dcl_common_interface.h"
 
#ifdef __cplusplus
 extern "C" {
#endif

//HTTP buffer
typedef struct DCL_MPUSH_PUSH_STREAM_HANDLER_t
{
	int32_t sock;
	char 	domain[64];
	char 	port[8];
	char 	params[64];
	char 	req_header[1024];
	
	char    str_nonce[64];
	char    str_timestamp[64];
	char    str_private_key[64];
	
	void	*json_body;
}DCL_MPUSH_PUSH_STREAM_HANDLER_t;

DCL_ERROR_CODE_t dcl_mpush_push_stream_create(
	void **handler,
	const DCL_AUTH_PARAMS_t* const input_params);

DCL_ERROR_CODE_t dcl_mpush_push_stream(
	void *handler,
	const char *data, 
	const uint32_t data_len);

DCL_ERROR_CODE_t dcl_mpush_push_stream_delete(
	void *handler);

#ifdef __cplusplus
 }
#endif
 
#endif  /* DCL_MPUSH_PUSH_STREAM_H */

