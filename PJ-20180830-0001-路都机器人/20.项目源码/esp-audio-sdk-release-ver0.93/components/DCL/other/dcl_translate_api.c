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
#include "dcl_translate_api.h"
#include "http_api.h"
#include "auth_crypto.h"
#include "socket.h"
#include "cJSON.h"
#include "memory_interface.h"
#include "debug_log_interface.h"
#include "time_interface.h"

#define PARAMS_CHINESE			"中文"
#define PARAMS_ENGLISH			"英语"

static const char *TAG_LOG = "[DCL TRANSLATE]";
static const char *DCL_TRANSLATE_URL = "http://api.deepbrain.ai:8383/open-api/service";

static DCL_ERROR_CODE_t dcl_translate_session_begin(
	DCL_TRANSLATE_HANDLE_t **translate_handle)
{
	DCL_HTTP_BUFFER_t *http_buffer = NULL;
	DCL_TRANSLATE_HANDLE_t *handle = NULL;
	
	if (translate_handle == NULL)
	{
		DEBUG_LOGE(TAG_LOG, "dcl_translate_session_begin invalid params");
		return DCL_ERROR_CODE_SYS_INVALID_PARAMS;
	}
	
	*translate_handle = (DCL_TRANSLATE_HANDLE_t *)memory_malloc(sizeof(DCL_TRANSLATE_HANDLE_t));
	if (*translate_handle == NULL)
	{
		DEBUG_LOGE(TAG_LOG, "dcl_translate_session_begin malloc failed");
		return DCL_ERROR_CODE_SYS_NOT_ENOUGH_MEM;
	}
	memset(*translate_handle, 0, sizeof(DCL_TRANSLATE_HANDLE_t));
	handle = *translate_handle;
	http_buffer = &handle->http_buffer;
	http_buffer->sock = INVALID_SOCK;

	if (sock_get_server_info(DCL_TRANSLATE_URL, &http_buffer->domain, &http_buffer->port, &http_buffer->params) != 0)
	{
		DEBUG_LOGE(TAG_LOG, "sock_get_server_info failed");
		return DCL_ERROR_CODE_NETWORK_DNS_FAIL;
	}

	http_buffer->sock = sock_connect(http_buffer->domain, http_buffer->port);
	if (http_buffer->sock == INVALID_SOCK)
	{
		DEBUG_LOGE(TAG_LOG, "sock_connect fail,[%s:%s]", 
			http_buffer->domain, http_buffer->port);
		return DCL_ERROR_CODE_NETWORK_UNAVAILABLE;
	}
	sock_set_nonblocking(http_buffer->sock);
	
	return DCL_ERROR_CODE_OK;
}

static DCL_ERROR_CODE_t dcl_translate_session_end(
	DCL_TRANSLATE_HANDLE_t *translate_handle)
{
	if (translate_handle == NULL)
	{
		return DCL_ERROR_CODE_SYS_INVALID_PARAMS;
	}
	DCL_HTTP_BUFFER_t *http_buffer = &translate_handle->http_buffer;

	//free socket
	if (http_buffer->sock != INVALID_SOCK)
	{
		sock_close(http_buffer->sock);
		http_buffer->sock = INVALID_SOCK;
	}

	//free json object
	if (http_buffer->json_body != NULL)
	{
		cJSON_Delete(http_buffer->json_body);
		http_buffer->json_body = NULL;
	}
	
	//free memory
	memory_free(translate_handle);
	translate_handle = NULL;

	return DCL_ERROR_CODE_OK;
}

static void dcl_translate_make_translate_mode(DCL_TRANSLATE_HANDLE_t *translate_handle)
{
	switch (translate_handle->translate_params.translate_mode)
	{
		case DCL_TRANSLATE_MODE_CH_AND_EN:
		{
			memset(translate_handle->translate_params.from_la, 0, sizeof(translate_handle->translate_params.from_la));
			memset(translate_handle->translate_params.to_la, 0, sizeof(translate_handle->translate_params.to_la));
			break;
		}
		case DCL_TRANSLATE_MODE_CH_TO_EN:
		{
			snprintf(translate_handle->translate_params.from_la, sizeof(translate_handle->translate_params.from_la), "%s", PARAMS_CHINESE);
			snprintf(translate_handle->translate_params.to_la, sizeof(translate_handle->translate_params.from_la), "%s", PARAMS_ENGLISH);
			break;
		}
		case DCL_TRANSLATE_MODE_EN_TO_CH:
		{
			snprintf(translate_handle->translate_params.from_la, sizeof(translate_handle->translate_params.from_la), "%s", PARAMS_ENGLISH);
			snprintf(translate_handle->translate_params.to_la, sizeof(translate_handle->translate_params.from_la), "%s", PARAMS_CHINESE);
			break;
		}
		default:
			break;
	}
}

static DCL_ERROR_CODE_t dcl_translate_make_packet(
	DCL_TRANSLATE_HANDLE_t *translate_handle,
	DCL_TRANSLATE_MODE_t translate_mode,
	const DCL_AUTH_PARAMS_t* const input_params,
	const char* const input_text)
{
	DCL_HTTP_BUFFER_t *http_buffer = &translate_handle->http_buffer;

	//设置翻译模式
	translate_handle->translate_params.translate_mode = translate_mode;
	dcl_translate_make_translate_mode(translate_handle);
	
	//make body string
	crypto_generate_request_id(http_buffer->str_nonce, sizeof(http_buffer->str_nonce));
	snprintf(http_buffer->req_body, sizeof(http_buffer->req_body),
		"{ \"app_id\": \"%s\","
			"\"content\": "
				"{\"inputText\":\"%s\","
					"\"fromLa\":\"%s\","
					"\"toLa\":\"%s\"},"
			"\"device_id\": \"%s\","
			"\"request_id\": \"%s\","
			"\"robot_id\": \"%s\","
			"\"service\": \"DeepBrainTranslateSkillService\","
			"\"user_id\": \"%s\","
			"\"version\": \"2.0.0.0\"}",
		input_params->str_app_id,
		input_text,
		translate_handle->translate_params.from_la,
		translate_handle->translate_params.to_la,
		input_params->str_device_id,
		http_buffer->str_nonce,
		input_params->str_robot_id,
		input_params->str_user_id);
	
	//make header string
	crypto_generate_nonce((uint8_t *)http_buffer->str_nonce, sizeof(http_buffer->str_nonce));
	crypto_time_stamp((unsigned char*)http_buffer->str_timestamp, sizeof(http_buffer->str_timestamp));
	crypto_generate_private_key((uint8_t *)http_buffer->str_private_key, sizeof(http_buffer->str_private_key), 
			http_buffer->str_nonce, 
			http_buffer->str_timestamp, 
			input_params->str_robot_id);
	snprintf(http_buffer->req_header, sizeof(http_buffer->req_header), 
		"POST %s HTTP/1.0\r\n"
		"Host: %s:%s\r\n"
		"Accept: application/json\r\n"
		"Accept-Language: zh-cn\r\n"
		"Content-Length: %d\r\n"
		"Content-Type: application/json\r\n"
		"Nonce: %s\r\n"
		"CreatedTime: %s\r\n"
		"PrivateKey: %s\r\n"
		"Key: %s\r\n"
		"Connection:close\r\n\r\n", 
		http_buffer->params, 
		http_buffer->domain, 
		http_buffer->port, 
		strlen(http_buffer->req_body), 
		http_buffer->str_nonce, 
		http_buffer->str_timestamp, 
		http_buffer->str_private_key, 
		input_params->str_robot_id);

	//make full request body
	snprintf(http_buffer->str_request, sizeof(http_buffer->str_request), "%s%s", http_buffer->req_header, http_buffer->req_body);

	return DCL_ERROR_CODE_OK;
}

static DCL_ERROR_CODE_t dcl_translate_decode_packet(
	DCL_TRANSLATE_HANDLE_t *translate_handle,
	char* const out_text,
	const uint32_t out_text_len)
{
	DCL_HTTP_BUFFER_t *http_buffer = &translate_handle->http_buffer;

	if (http_get_error_code(http_buffer->str_response) == 200)
	{	
		char* pBody = http_get_body(http_buffer->str_response);
		if (pBody != NULL)
		{
			http_buffer->json_body = cJSON_Parse(pBody);
			if (http_buffer->json_body != NULL) 
			{
				cJSON *pJson_status = cJSON_GetObjectItem(http_buffer->json_body, "statusCode");
				if (pJson_status == NULL || pJson_status->valuestring == NULL)
				{
					DEBUG_LOGE(TAG_LOG, "statusCode not found");
					return DCL_ERROR_CODE_SERVER_ERROR;
				}

				if (strncasecmp(pJson_status->valuestring, "OK", strlen("OK")) != 0)
				{
					DEBUG_LOGE(TAG_LOG, "statusCode:%s", pJson_status->valuestring);
					return DCL_ERROR_CODE_SERVER_ERROR;
				}
				
				cJSON *json_content = cJSON_GetObjectItem(http_buffer->json_body, "content");
				if (json_content == NULL)
				{
					DEBUG_LOGE(TAG_LOG, "json string has no content node");
					return DCL_ERROR_CODE_SERVER_ERROR;
				}

				cJSON *json_tts = cJSON_GetObjectItem(json_content, "translateText");
				if (json_tts == NULL)
				{
					DEBUG_LOGE(TAG_LOG, "json string has no translateText node");
					return DCL_ERROR_CODE_SERVER_ERROR;
				}

				if (json_tts == NULL || json_tts->valuestring == NULL)
				{
					DEBUG_LOGE(TAG_LOG, "translateText node is NULL");
					return DCL_ERROR_CODE_SERVER_ERROR;
				}
				
				snprintf(out_text, out_text_len, "%s", json_tts->valuestring);
			}
			else
			{
				DEBUG_LOGE(TAG_LOG, "invalid json[%s]", pBody);
				return DCL_ERROR_CODE_NETWORK_POOR;
			}
		}
	}
	else
	{
		DEBUG_LOGE(TAG_LOG, "http reply error[%s]", http_buffer->str_response);
		return DCL_ERROR_CODE_NETWORK_POOR;
	}

	return DCL_ERROR_CODE_OK;
}

DCL_ERROR_CODE_t dcl_get_translate_text(
	DCL_TRANSLATE_MODE_t translate_mode,
	const DCL_AUTH_PARAMS_t* const input_params,
	const char* const input_text,
	char* const out_text,
	const uint32_t out_text_len)
{
	int ret = 0;
	DCL_TRANSLATE_HANDLE_t *translate_handle = NULL;
	DCL_HTTP_BUFFER_t *http_buffer = NULL;
	DCL_ERROR_CODE_t err_code = DCL_ERROR_CODE_OK;
	uint64_t start_time = get_time_of_day();
	
	if (input_params == NULL
		|| input_text == NULL
		|| strlen(input_text) == 0
		|| out_text == NULL
		|| out_text_len == 0)
	{
		return DCL_ERROR_CODE_SYS_INVALID_PARAMS;
	}

	//建立session
	err_code = dcl_translate_session_begin(&translate_handle);
	if (err_code != DCL_ERROR_CODE_OK)
	{
		DEBUG_LOGE(TAG_LOG, "dcl_translate_session_begin failed");
		goto dcl_get_translate_text_error;
	}
	http_buffer = &translate_handle->http_buffer;
	
	//组包
	err_code = dcl_translate_make_packet(translate_handle, translate_mode, input_params, input_text);
	if (err_code != DCL_ERROR_CODE_OK)
	{
		DEBUG_LOGE(TAG_LOG, "dcl_translate_make_packet failed");
		goto dcl_get_translate_text_error;
	}
	//DEBUG_LOGI(TAG_LOG, "%s", http_buffer->str_request);

	//发包
	if (sock_writen_with_timeout(http_buffer->sock, http_buffer->str_request, strlen(http_buffer->str_request), 1000) != strlen(http_buffer->str_request)) 
	{
		DEBUG_LOGE(TAG_LOG, "sock_writen_with_timeout failed");
		err_code = DCL_ERROR_CODE_NETWORK_POOR;
		goto dcl_get_translate_text_error;
	}
	//接包
	memset(http_buffer->str_response, 0, sizeof(http_buffer->str_response));
	ret = sock_readn_with_timeout(http_buffer->sock, http_buffer->str_response, sizeof(http_buffer->str_response) - 1, 2000);
	if (ret <= 0)
	{
		DEBUG_LOGE(TAG_LOG, "sock_readn_with_timeout failed");
		err_code = DCL_ERROR_CODE_NETWORK_POOR;
		goto dcl_get_translate_text_error;
	}
	//解包
	err_code = dcl_translate_decode_packet(translate_handle, out_text, out_text_len);
	if (err_code != DCL_ERROR_CODE_OK)
	{
		DEBUG_LOGE(TAG_LOG, "dcl_translate_decode_packet failed");
		goto dcl_get_translate_text_error;
	}
	DEBUG_LOGI(TAG_LOG, "dcl_get_translate_text total time cost[%lldms]", get_time_of_day() - start_time);
	
dcl_get_translate_text_error:

	//销毁session
	if (dcl_translate_session_end(translate_handle) != DCL_ERROR_CODE_OK)
	{
		DEBUG_LOGE(TAG_LOG, "dcl_translate_session_end failed");
	}
	
	return err_code;
}

