#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "debug_log.h"
#include "cJSON.h"
#include "socket.h"
#include "device_api.h"
#include "unisound_api.h"
#include "http_api.h"

//HTTP 请求缓存
typedef struct 
{
	int socket;
	char domain[64];
	char port[8];
	char params[256];
	char header[512];
	int send_len;
	char send_buffer[1024*32];//32k发送缓存	
	char recv_buffer[2048];//2k接收缓存	
}UNISOUND_HTTP_BUFF_T;

//语音识别句柄
typedef struct
{
	UNISOUND_HTTP_BUFF_T http_buff;
}UNISOUND_ASR_HANDLE_T;

static const char *UNISOUND_ASR_URL = "http://api.hivoice.cn/USCService/WebApi?appkey=%s&userid=%s&id=%s";
static const char *POST_REQUEST_HEADER = "POST %s HTTP/1.1\r\n"\
	"HOST: %s:%s\r\n"\
	"Content-Type: audio/x-wav;codec=pcm;bit=16;rate=16000\r\n"\  
	"Accept: text/plain\r\n"\
	"Accept-Language: zh_CN\r\n"\
	"Accept-Charset: utf-8\r\n"\
	"Accept-Topic: general\r\n"\
	"Transfer-Encoding: chunked\r\n\r\n";

static const char *TAG_ASR = "[Unisound ASR]";

static UNISOUND_ASR_ERRNO_T make_asr_packet(
	UNISOUND_HTTP_BUFF_T *http_buff, 
	const char *data, 
	const uint32_t data_len)
{
	
	if (http_buff == NULL || data == NULL || data_len == 0)
	{
		return UNISOUND_ASR_ERRNO_INVALID_PARAMS;
	}

	snprintf(http_buff->send_buffer, sizeof(http_buff->send_buffer), "%x\r\n", data_len);
	http_buff->send_len = strlen(http_buff->send_buffer);
	if (data_len > (sizeof(http_buff->send_buffer) - http_buff->send_len - strlen("\r\n")))
	{
		return UNISOUND_ASR_ERRNO_DATA_TOO_LONG;
	}
	
	memcpy(http_buff->send_buffer + http_buff->send_len, data, data_len);
	http_buff->send_len += data_len;
	memcpy(http_buff->send_buffer + http_buff->send_len, "\r\n", strlen("\r\n"));
	http_buff->send_len += strlen("\r\n");
	
	return UNISOUND_ASR_ERRNO_OK;
}

void *unisound_asr_create(const UNISOUND_ASR_ACCOUNT_T *asr_acount)
{
	int ret = 0;
	UNISOUND_HTTP_BUFF_T *http_buff = NULL;	
	UNISOUND_ASR_HANDLE_T *asr_handle = (UNISOUND_ASR_HANDLE_T *)esp32_malloc(sizeof(UNISOUND_ASR_HANDLE_T));
	if (asr_handle == NULL)
	{
		goto UNISOUND_ASR_CREATE_ERROR;
	}
	memset(asr_handle, 0, sizeof(UNISOUND_ASR_HANDLE_T));
	http_buff = &asr_handle->http_buff;
	
	snprintf(http_buff->send_buffer, sizeof(http_buff->send_buffer), UNISOUND_ASR_URL, 
		asr_acount->appkey, asr_acount->user_id, asr_acount->device_id);
	
	if (sock_get_server_info(http_buff->send_buffer, 
			http_buff->domain, http_buff->port, http_buff->params) != 0)
	{
		DEBUG_LOGE(TAG_ASR, "sock_get_server_info fail");
		goto UNISOUND_ASR_CREATE_ERROR;
	}

	//连接服务器
	http_buff->socket = sock_connect(http_buff->domain, http_buff->port);
	if (http_buff->socket == INVALID_SOCK) 
	{
		DEBUG_LOGE(TAG_ASR, "sock_connect fail");
		goto UNISOUND_ASR_CREATE_ERROR;
	}
	sock_set_nonblocking(http_buff->socket);

	//发送http信息
	snprintf(http_buff->send_buffer, sizeof(http_buff->send_buffer), POST_REQUEST_HEADER, 
		http_buff->params, http_buff->domain, http_buff->port);

	ret = sock_writen_with_timeout(
		http_buff->socket, http_buff->send_buffer, strlen(http_buff->send_buffer), 1000);
	if (ret != strlen(http_buff->send_buffer)) 
	{
		DEBUG_LOGE(TAG_ASR, "sock_writen_with_timeout head failed[%d][%d]", strlen(http_buff->send_buffer), ret);
		goto UNISOUND_ASR_CREATE_ERROR;
	}

	return asr_handle;
	
UNISOUND_ASR_CREATE_ERROR:

	if (asr_handle != NULL)
	{
		esp32_free(asr_handle);
		asr_handle = NULL;
	}

	if (http_buff->socket != INVALID_SOCK)
	{
		sock_close(http_buff->socket);
		http_buff->socket = INVALID_SOCK;
	}
	
	return asr_handle;
}

UNISOUND_ASR_ERRNO_T unisound_asr_wirte(
	void *asr_handle, const char *data, const uint32_t data_len)
{	
	int ret = 0;
	UNISOUND_HTTP_BUFF_T *http_buff = NULL;	
	UNISOUND_ASR_HANDLE_T *handle = asr_handle;
	
	if (asr_handle == NULL || data == NULL || data_len == 0)
	{
		return UNISOUND_ASR_ERRNO_INVALID_PARAMS;
	}
	http_buff = &handle->http_buff;
	
	ret = make_asr_packet(http_buff, data, data_len);
	if (ret != UNISOUND_ASR_ERRNO_OK)
	{
		DEBUG_LOGE(TAG_ASR, "make_asr_packet failed[%d]", ret);
		return ret;
	}

	ret = sock_writen_with_timeout(
		http_buff->socket, http_buff->send_buffer, http_buff->send_len, 2000);
	if (ret != http_buff->send_len) 
	{
		DEBUG_LOGE(TAG_ASR, "sock_writen_with_timeout body failed[%d][%d]", http_buff->send_len, ret);
		return UNISOUND_ASR_ERRNO_WRITE_FAIL;
	}
	
	return UNISOUND_ASR_ERRNO_OK;
}

UNISOUND_ASR_ERRNO_T unisound_asr_result(
	void *asr_handle, char* str_reply, const size_t str_reply_len)
{	
	int ret = 0;
	int i = 0;
	int total_len = 0;
	char *str_end = "0\r\n\r\n";
	UNISOUND_HTTP_BUFF_T *http_buff = NULL;	
	UNISOUND_ASR_HANDLE_T *handle = asr_handle;
	
	if (asr_handle == NULL)
	{
		return UNISOUND_ASR_ERRNO_INVALID_PARAMS;
	}
	http_buff = &handle->http_buff;

	ret = sock_writen_with_timeout(
		http_buff->socket, str_end, strlen(str_end), 500);
	if (ret != strlen(str_end)) 
	{
		DEBUG_LOGE(TAG_ASR, "sock_writen_with_timeout failed[%d][%d]", strlen(str_end), ret);
		return UNISOUND_ASR_ERRNO_WRITE_FAIL;
	}

	/* Read HTTP response */
	memset(http_buff->recv_buffer, 0, sizeof(http_buff->recv_buffer));
	for (i=0; i<10; i++)
	{
		ret = sock_readn_with_timeout(http_buff->socket, http_buff->recv_buffer + total_len, sizeof(http_buff->recv_buffer) - total_len - 1, 200);
		if (ret >= 0)
		{
			total_len += ret; 
		}

		if (strstr(http_buff->recv_buffer, str_end))
		{
			break;
		}
	}

	char *http_body = http_get_body(http_buff->recv_buffer);
	if (http_get_error_code(http_buff->recv_buffer) == 200
		&& http_body != NULL)
	{
		memset(str_reply, 0, str_reply_len);
		http_get_chunked_body(str_reply, str_reply_len, http_body);
	}
	else
	{
		DEBUG_LOGE(TAG_ASR, "ASR返回完整数据:[%s]", http_buff->recv_buffer);
	}
	
	return UNISOUND_ASR_ERRNO_OK;
}

void unisound_asr_delete(void *asr_handle)
{
	UNISOUND_ASR_HANDLE_T *handle = asr_handle;
	
	if (asr_handle != NULL)
	{
		if (handle->http_buff.socket != INVALID_SOCK)
		{
			sock_close(handle->http_buff.socket);
		}
		
		esp32_free(handle);
	}
	
	return;
}

