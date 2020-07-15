
#include <string.h>
#include "cJSON.h"
#include "baidu_api.h"
#include "socket.h"
#include "http_api.h"
#include "debug_log.h"

static char g_str_token[128] = {0};

static const char *TAG_TTS = "[Baidu TTS]";
static const char *TAG_ASR = "[Baidu ASR]";

static const char *BAIDU_TOKEN_URL = "http://openapi.baidu.com/oauth/2.0/token"
	"?grant_type=client_credentials&client_id=%s&client_secret=%s";

static const char *BAIDU_TTS_URL = "http://tsn.baidu.com/text2audio"
	"?tex=%s&lan=zh&cuid=8698179&ctp=1&tok=%s&per=4&spd=4";

static const char *GET_ACCESS_TOKEN_REQUEST_HEADER = "GET %s HTTP/1.0\r\n"
    "Host: %s\r\n"
    "User-Agent: esp-idf/1.0 esp32\r\n"
    "Content-Type: application/json; charset=utf-8\r\n"
    "\r\n";

static const char *BAIDU_ASR_URL = "http://vop.baidu.com/server_api";

static const char *GET_ASR_REQUEST_HEADER = "POST %s?cuid=8698179&dev_pid=1536&token=%s HTTP/1.0\r\n"
    "Host: %s\r\n"
    "User-Agent: esp-idf/1.0 esp32\r\n"
    "Content-Type: audio/amr;rate=8000\r\n"
	"Content-Length: %d\r\n"
    "\r\n";

static int _get_access_token(
	BAIDU_ACOUNT_T *_acount,
	char *str_token, 
	size_t str_token_len)
{
	char 	domain[64] 	= {0};
	char 	port[6] 	= {0};
	char 	params[256] = {0};
	char 	header[256] = {0};
	char	str_buf[1024]= {0};
	sock_t 	sock 		= INVALID_SOCK;

	snprintf(header, sizeof(header), BAIDU_TOKEN_URL, _acount->app_key, _acount->secret_key);
	if (sock_get_server_info(header, &domain, &port, &params) != 0)
	{
		DEBUG_LOGE(TAG_TTS, "sock_get_server_info fail");
		goto GET_ACCESS_TOKEN_ERROR;
	}

	sock = sock_connect(domain, port);
	if (sock == INVALID_SOCK)
	{
		DEBUG_LOGE(TAG_TTS, "sock_connect fail");
		goto GET_ACCESS_TOKEN_ERROR;
	}
	sock_set_nonblocking(sock);
	
	snprintf(header, sizeof(header), GET_ACCESS_TOKEN_REQUEST_HEADER, params, domain);
	if (sock_writen_with_timeout(sock, header, strlen(header), 1000) != strlen(header)) 
	{
		DEBUG_LOGE(TAG_TTS, "sock_write http header fail");
		goto GET_ACCESS_TOKEN_ERROR;
	}
	//DEBUG_LOGE(TAG_ASR, "header=%s", header);
	/* Read HTTP response */
	memset(str_buf, 0, sizeof(str_buf));
	sock_readn_with_timeout(sock, str_buf, sizeof(str_buf) - 1, 3000);
	sock_close(sock);
	
	if (http_get_error_code(str_buf) == 200)
	{
		char* pBody = http_get_body(str_buf);
		if (NULL != pBody)
		{
			cJSON *pJsonSub_status = NULL;
			cJSON *pJson = cJSON_Parse(pBody);
		    if (NULL != pJson) 
			{
		        pJsonSub_status = cJSON_GetObjectItem(pJson, "access_token");
				if (NULL != pJsonSub_status)
				{
					snprintf(str_token, str_token_len, "%s", pJsonSub_status->valuestring);
					return BAIDU_TTS_SUCCESS;
				}
		    }
			else
			{
				DEBUG_LOGE(TAG_TTS, "cJSON_Parse error");
			}
			
			if (NULL != pJson)
			{
				cJSON_Delete(pJson);
				pJson = NULL;
			}
		}
	}
	else
	{
		DEBUG_LOGE(TAG_TTS, "%s", str_buf);
	}

GET_ACCESS_TOKEN_ERROR:

	sock_close(sock);
	return BAIDU_TTS_GET_TOKEN_FAIL;
}

//替换特殊字符
static char* _replace_special_char(char *str_dest, int str_dest_len, char *str_src)
{
	int i = 0;
	int m = 0;
	int len = strlen(str_src);
	
	bzero(str_dest, str_dest_len);
	/*
	1. +  URL 中+号表示空格 %2B   
	2. 空格 URL中的空格可以用+号或者编码 %20   
	3. /  分隔目录和子目录 %2F    
	4. ?  分隔实际的 URL 和参数 %3F    
	5. % 指定特殊字符 %25    
	6. # 表示书签 %23    
	7. & URL 中指定的参数间的分隔符 %26    
	8. = URL 中指定参数的值 %3D  
	*/
	for (i = 0; i < len; i++)
	{
		switch(str_src[i])
		{
			case '+':
				strcat(str_dest, "%2B");
				break;
			case ' ':
				strcat(str_dest, "%20");
				break;
			case '/':
				strcat(str_dest, "%2F");
				break;
			case '?':
				strcat(str_dest, "%3F");
				break;
			case '%':
				strcat(str_dest, "%25");
				break;
			case '#':
				strcat(str_dest, "%23");
				break;
			case '&':
				strcat(str_dest, "%26");
				break;
			case '=':
				strcat(str_dest, "%3D");
				break;
			default:
				m = strlen(str_dest);
				if (m < (str_dest_len - 1))
				{
					str_dest[m] = str_src[i];
				}
				break;
		}
	}

	return str_dest;
}

static int _get_asr_result(
	BAIDU_ACOUNT_T *_acount,
	char* const _str_ret,
	const size_t _str_ret_len,
	const unsigned char* _data,
	const size_t _data_len,
	const char *_token)
{
	int 	err_no 		= BAIDU_ASR_SUCCESS;
	char	domain[64]	= {0};
	char	port[6] 	= {0};
	char	params[256] = {0};
	char	header[512] = {0};
	char	str_buf[1024]= {0};
	sock_t	sock		= INVALID_SOCK;
	
	if (sock_get_server_info(_acount->asr_url, &domain, &port, &params) != 0)
	{
		DEBUG_LOGE(TAG_ASR, "sock_get_server_info fail");
		err_no = BAIDU_ASR_FAIL;
		goto GET_ASR_RESULT_ERROR;
	}

	sock = sock_connect(domain, port);
	if (sock == INVALID_SOCK)
	{
		DEBUG_LOGE(TAG_ASR, "sock_connect fail");
		err_no = BAIDU_ASR_FAIL;
		goto GET_ASR_RESULT_ERROR;
	}
	sock_set_nonblocking(sock);

	//post http header
	snprintf(header, sizeof(header), GET_ASR_REQUEST_HEADER, params, _token, domain, _data_len);
	if (sock_writen_with_timeout(sock, header, strlen(header), 1000) != strlen(header)) 
	{
		DEBUG_LOGE(TAG_ASR, "sock_write http header fail");
		err_no = BAIDU_ASR_FAIL;
		goto GET_ASR_RESULT_ERROR;
	}
	//DEBUG_LOGE(TAG_ASR, "header=%s", header);
	//post http body
	if (sock_writen_with_timeout(sock, _data, _data_len, 2000) != _data_len) 
	{
		DEBUG_LOGE(TAG_ASR, "sock_write http body fail");
		err_no = BAIDU_ASR_FAIL;
		goto GET_ASR_RESULT_ERROR;
	}

	/* Read HTTP response */
	memset(str_buf, 0, sizeof(str_buf));
	sock_readn_with_timeout(sock, str_buf, sizeof(str_buf) - 1, 2000);
	//DEBUG_LOGE(TAG_ASR, "str_reply=%s\n", str_buf);
	
	if (http_get_error_code(str_buf) != 200)
	{
		DEBUG_LOGE(TAG_ASR, "reply:[%s]", str_buf);
		err_no = BAIDU_ASR_FAIL;
		goto GET_ASR_RESULT_ERROR;
	}
	
	char *pBody = http_get_body(str_buf);
	if (NULL == pBody)
	{
		err_no = BAIDU_ASR_FAIL;
		goto GET_ASR_RESULT_ERROR;
	}

	cJSON *pJson = cJSON_Parse(pBody);
    if (NULL == pJson) 
    {
    	err_no = BAIDU_ASR_FAIL;
		DEBUG_LOGE(TAG_ASR, "invalid json:[%s]", pBody);
		goto GET_ASR_RESULT_ERROR;
	}

    cJSON *pJsonSub_error = cJSON_GetObjectItem(pJson, "err_no");
	if (NULL == pJsonSub_error)
	{
		cJSON_Delete(pJson);
		err_no = BAIDU_ASR_FAIL;
		goto GET_ASR_RESULT_ERROR;
	}

	err_no = pJsonSub_error->valueint;
	if (err_no == 0)
	{
		cJSON *pJsonSub_text = cJSON_GetObjectItem(pJson, "result");
		if (NULL != pJsonSub_text && NULL != pJsonSub_text->child)
		{
			if (pJsonSub_text->child->type == cJSON_Number)
			{
				snprintf(_str_ret, _str_ret_len, "%d", pJsonSub_text->child->valueint);
			}
			else if (pJsonSub_text->child->type == cJSON_String)
			{
				snprintf(_str_ret, _str_ret_len, "%s", pJsonSub_text->child->valuestring);
			}
		}
	}
	else
	{
		switch(err_no)
		{
			case BAIDU_ASR_INVALID_PARAMS:
				ESP_LOGE(TAG_ASR, "BAIDU_ASR_INVALID_PARAMS");
			break;
			case BAIDU_ASR_REC_FAIL:
				ESP_LOGE(TAG_ASR, "BAIDU_ASR_REC_FAIL");
			break;
			case BAIDU_ASR_TOKEN_FAIL:
				ESP_LOGE(TAG_ASR, "BAIDU_ASR_TOKEN_FAIL");
			break;
			case BAIDU_ASR_SERVER_ERROR:
				ESP_LOGE(TAG_ASR, "BAIDU_ASR_SERVER_ERROR");
			break;
			case BAIDU_ASR_LIMIT_GPS:
				ESP_LOGE(TAG_ASR, "BAIDU_ASR_LIMIT_GPS");
			break;
			case BAIDU_ASR_REQUEST_LIMIT:
				ESP_LOGE(TAG_ASR, "BAIDU_ASR_REQUEST_LIMIT");
			break;
			default:
				ESP_LOGE(TAG_ASR, "unknow error:%d", err_no);
				break;
		}
	}

	cJSON_Delete(pJson);

GET_ASR_RESULT_ERROR:
	sock_close(sock);

	return err_no;
}

int baidu_asr(
	BAIDU_ACOUNT_T *_acount,
	char* str_reply, 
	size_t str_reply_len, 
	unsigned char *_data, 
	size_t _data_len)
{
	char str_host[512] = {0};
	
	//get access token
	if (strlen(g_str_token) == 0) 
	{
		if (BAIDU_TTS_SUCCESS != _get_access_token(_acount, g_str_token, sizeof(g_str_token)))
		{
			DEBUG_LOGE(TAG_TTS, "get_access_token failed");
			return BAIDU_ASR_FAIL;
		}
	}

	//post audio data and get result 
	return _get_asr_result(_acount, str_reply, str_reply_len, _data, _data_len, g_str_token);
}


int baidu_tts(
	BAIDU_ACOUNT_T *_acount, 
	char *str_url, 
	size_t str_url_len, 
	char *str_tts_text)
{	
	char str_buf[1024] = {0};
	
	//get access token
	if (strlen(g_str_token) == 0) 
	{
		if (BAIDU_TTS_SUCCESS != _get_access_token(_acount, g_str_token, sizeof(g_str_token)))
		{
			DEBUG_LOGE(TAG_TTS, "get_access_token failed");
			return BAIDU_TTS_GET_TOKEN_FAIL;
		}
	}

	_replace_special_char(str_buf, sizeof(str_buf), str_tts_text);
	snprintf(str_url, str_url_len, BAIDU_TTS_URL, str_buf, g_str_token);

	return BAIDU_TTS_SUCCESS;
}

