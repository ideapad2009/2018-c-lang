
#include <string.h>
#include "deepbrain_api.h"
#include "device_api.h"
#include "http_api.h"
#include "crypto_api.h"
#include "socket.h"
#include "debug_log.h"
#include "userconfig.h"
#include "flash_config_manage.h"

//internal private struct data
typedef struct
{
	char str_nonce[BUFFER_SIZE_64];
	char str_timestamp[BUFFER_SIZE_64];
	char str_private_key[BUFFER_SIZE_64];
	char str_app_id[BUFFER_SIZE_64];
	char str_robot_id[BUFFER_SIZE_64];
	char str_device_id[BUFFER_SIZE_32];
	char str_user_id[BUFFER_SIZE_32];
	char str_city_name[BUFFER_SIZE_32];
	char str_city_longitude[BUFFER_SIZE_16];
	char str_city_latitude[BUFFER_SIZE_16];
	char str_reply[BUFFER_SIZE_KB(30)];
}ST_DB_NLP_HANDLER,*PST_DB_NLP_HANDLER;



static const char *TAG = "[Deep Brain]";

static const char *DeepBrain_URL = "http://api.deepbrain.ai:8383/deep-brain-api/ask";

static const char *POST_REQUEST_HEADER = "POST %s HTTP/1.0\r\n"
    "Host: %s:%s\r\n"
    "User-Agent: esp-idf/1.0 esp32\r\n"
    "Content-Type: application/json; charset=utf-8\r\n"
    "Accept: */*\r\n"
    "Content-Length: %d\r\n"
    "\r\n";

static const char *POST_REQUEST_BODY = "{\"location\":{\"cityName\":\"%s\",\"longitude\":\"%s\",\"latitude\":\"%s\"},"\
	"\"requestHead\":{\"accessToken\":{\"nonce\":\"%s\",\"privateKey\":\"%s\","\
	"\"createdTime\":\"%s\"},\"apiAccount\":{\"appId\":\"%s\",\"robotId\":\"%s\","\
	"\"userId\":\"%s\",\"deviceId\":\"%s\"}},\"nlpData\":{\"inputText\":\"%s\"},\"simpleView\":true}";


static int http_post_method(
	char* str_reply_buf,
	unsigned int reply_buf_len,
	const char* str_web_url,
	const char* post_data,
	const unsigned int post_data_len)
{
	sock_t 	sock		= INVALID_SOCK;
	char 	domain[64] 	= {0};
	char 	port[6] 	= {0};
	char 	params[128] = {0};
	char 	header[512] = {0};

	if (sock_get_server_info(DeepBrain_URL, &domain, &port, &params) != 0)
	{
		DEBUG_LOGE(TAG, "sock_get_server_info fail");
		goto HTTP_POST_METHOD_ERROR;
	}

	sock = sock_connect(domain, port);
	if (sock == INVALID_SOCK)
	{
		DEBUG_LOGE(TAG, "sock_connect %s:%s failed", domain, port);
		goto HTTP_POST_METHOD_ERROR;
	}
	sock_set_nonblocking(sock);

	snprintf(header, sizeof(header), POST_REQUEST_HEADER, 
		params, domain, port, post_data_len);
//	DEBUG_LOGE(TAG, "header is %s", header);
	if (sock_writen_with_timeout(sock, header, strlen(header), 1000) != strlen(header)) 
	{
		DEBUG_LOGE(TAG, "sock_write http header fail");
		goto HTTP_POST_METHOD_ERROR;
	}
	
	if (sock_writen_with_timeout(sock, post_data, post_data_len, 1000) != post_data_len)
	{
		DEBUG_LOGE(TAG, "sock_write http body fail");
		goto HTTP_POST_METHOD_ERROR;
	}
	
	memset(str_reply_buf, 0, reply_buf_len);
	sock_readn_with_timeout(sock, str_reply_buf, reply_buf_len - 1, 3000);
	sock_close(sock);

	if (http_get_error_code(str_reply_buf) == 200)
	{
		return DB_ERROR_CODE_SUCCESS;
	}
	else
	{
		DEBUG_LOGE(TAG, "%s", str_reply_buf);
		return DB_ERROR_CODE_HTTP_FAIL;
	}
	
	return DB_ERROR_CODE_SUCCESS;
HTTP_POST_METHOD_ERROR:

	if (sock > 0)
	{
		sock_close(sock);
	}
	return DB_ERROR_CODE_HTTP_FAIL;

}

void *deep_brain_nlp_create(void)
{
	PST_DB_NLP_HANDLER handler = (PST_DB_NLP_HANDLER)esp32_malloc(sizeof(ST_DB_NLP_HANDLER));
	if (handler == NULL)
	{
		return NULL;
	}

	memset((char*)handler, 0, sizeof(ST_DB_NLP_HANDLER));
	
	return handler;
}

char *deep_brain_nlp_result(void *_handler)
{
	PST_DB_NLP_HANDLER handler = _handler;
	if (handler != NULL)
	{
		return handler->str_reply;
	}

	return NULL;
}

int deep_brain_set_params(void *_handler, int _index, void *_data)
{
	PST_DB_NLP_HANDLER handler = _handler;
	if (handler == NULL || _data == NULL)
	{
		return DB_ERROR_CODE_INVALID_PARAMS;
	}

	switch(_index)
	{
		case DB_NLP_PARAMS_APP_ID:
		{
			snprintf(handler->str_app_id, sizeof(handler->str_app_id), "%s", (char*)_data);
			break;
		}
		case DB_NLP_PARAMS_ROBOT_ID:
		{
			snprintf(handler->str_robot_id, sizeof(handler->str_robot_id), "%s", (char*)_data);
			break;
		}
		case DB_NLP_PARAMS_DEVICE_ID:
		{
			snprintf(handler->str_device_id, sizeof(handler->str_device_id), "%s", (char*)_data);
			break;
		}
		case DB_NLP_PARAMS_USER_ID:
		{
			snprintf(handler->str_user_id, sizeof(handler->str_user_id), "%s", (char*)_data);
			break;
		}
		case DB_NLP_PARAMS_CITY_NAME:
		{
			snprintf(handler->str_city_name, sizeof(handler->str_city_name), "%s", (char*)_data);
			break;
		}
		case DB_NLP_PARAMS_CITY_LONGITUDE:
		{
			snprintf(handler->str_city_longitude, sizeof(handler->str_city_longitude), "%s", (char*)_data);
			break;
		}
		case DB_NLP_PARAMS_CITY_LATITUDE:
		{
			snprintf(handler->str_city_latitude, sizeof(handler->str_city_latitude), "%s", (char*)_data);
			break;
		}
		default:
			break;
	}

	return DB_ERROR_CODE_SUCCESS;
}


int deep_brain_nlp_process(void *_handler, char *_str_text)
{		
	PST_DB_NLP_HANDLER handler = _handler;
	if (handler == NULL)
	{
		return DB_ERROR_CODE_INVALID_PARAMS;
	}

	char device_id[32] = {0};
	char user_id[32] = {0};
	get_flash_cfg(FLASH_CFG_USER_ID, user_id);
	get_flash_cfg(FLASH_CFG_DEVICE_ID, device_id);

	//生成nonce
	crypto_generate_nonce((unsigned char*)handler->str_nonce, sizeof(handler->str_nonce));
	//生成时间戳
	crypto_time_stamp((unsigned char*)handler->str_timestamp, sizeof(handler->str_timestamp));
	//生成秘钥
	crypto_generate_private_key((unsigned char*)handler->str_private_key, sizeof(handler->str_private_key), 
		handler->str_nonce, handler->str_timestamp, handler->str_robot_id);

	// 2.生成JSON格式字符串 
	snprintf(handler->str_reply, sizeof(handler->str_reply), POST_REQUEST_BODY,	
		handler->str_city_name, handler->str_city_longitude, handler->str_city_latitude,	
		handler->str_nonce, handler->str_private_key, handler->str_timestamp,	
		handler->str_app_id, handler->str_robot_id, user_id, device_id, _str_text);
	//printf("%s\r\n", handler->str_reply);
	return http_post_method(handler->str_reply, sizeof(handler->str_reply), DeepBrain_URL, handler->str_reply, strlen(handler->str_reply));
}

void deep_brain_nlp_destroy(void *_handler)
{
	PST_DB_NLP_HANDLER handler = _handler;
	if (handler != NULL)
	{
		esp32_free(handler);
	}
}

