
#include <string.h>
#include "cJSON.h"
#include "baidu_api.h"
#include "socket.h"
#include "http_api.h"
#include "device_api.h"
#include "debug_log.h"

#define MIN(a,b) ((a)<(b) ? (a):(b))
#define MAX(a,b) ((a)>(b) ? (a):(b))

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

static const char *GET_ASR_REQUEST_HEADER = "POST %s?cuid=8698179&dev_pid=%d&token=%s HTTP/1.0\r\n"
    "Host: %s\r\n"
    "User-Agent: esp-idf/1.0 esp32\r\n"
    "Content-Type: audio/amr;rate=%d\r\n"
	"Content-Length: %d\r\n"
    "\r\n";



typedef struct _ConnectSockt_t {
	char domain[64];
	char port[6];
	char params[256];
	char header[256];
	char buffer[2048];
	sock_t sock;
} _ConnectSockt_t;


static int _get_access_token(
	BAIDU_ACOUNT_T *_acount,
	char *str_token, 
	size_t str_token_len)
{
	_ConnectSockt_t * con_sock;

	if (!(con_sock = esp32_malloc(sizeof(_ConnectSockt_t)))) {
		DEBUG_LOGE(TAG_TTS, "sock_get_server_info fail, because of 'esp32_malloc failed");
		return BAIDU_TTS_GET_TOKEN_FAIL;
	}

	snprintf(con_sock->header, sizeof(con_sock->header), BAIDU_TOKEN_URL, _acount->app_key, _acount->secret_key);
	if (sock_get_server_info(con_sock->header, con_sock->domain, con_sock->port, con_sock->params) != 0)
	{
		DEBUG_LOGE(TAG_TTS, "sock_get_server_info fail");
		goto GET_ACCESS_TOKEN_ERROR;
	}

	con_sock->sock = sock_connect(con_sock->domain, con_sock->port);
	if (con_sock->sock == INVALID_SOCK)
	{
		DEBUG_LOGE(TAG_TTS, "sock_connect fail");
		goto GET_ACCESS_TOKEN_ERROR;
	}
	sock_set_nonblocking(con_sock->sock);
	
	snprintf(con_sock->header, sizeof(con_sock->header), GET_ACCESS_TOKEN_REQUEST_HEADER, con_sock->params, con_sock->domain);
	if (sock_writen_with_timeout(con_sock->sock, con_sock->header, strlen(con_sock->header), 1000) != strlen(con_sock->header)) 
	{
		DEBUG_LOGE(TAG_TTS, "sock_write http header fail");
		goto GET_ACCESS_TOKEN_ERROR;
	}
	//DEBUG_LOGE(TAG_ASR, "header=%s", header);
	/* Read HTTP response */
	memset(con_sock->buffer, 0, sizeof(con_sock->buffer));
	sock_readn_with_timeout(con_sock->sock, con_sock->buffer, sizeof(con_sock->buffer) - 1, 3000);
	sock_close(con_sock->sock);
	
	if (http_get_error_code(con_sock->buffer) == 200)
	{
		char* pBody = http_get_body(con_sock->buffer);
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
					esp32_free(con_sock);
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
		DEBUG_LOGE(TAG_TTS, "%s", con_sock->buffer);
	}

GET_ACCESS_TOKEN_ERROR:
	if (con_sock) {
		sock_close(con_sock->sock);
		esp32_free(con_sock);
	}
	return BAIDU_TTS_GET_TOKEN_FAIL;
}

//替换特殊字符
static int _replace_special_char(char *dest, int str_dest_len, char *str_src)
{
	int i = 0;
	int dest_cur_pos = 0;
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

	if (!dest || str_dest_len <= 0) {
		for(i = 0; str_src[i]; i++) {
			switch(str_src[i])
			{
				case '+':
				case ' ':
				case '/':
				case '?':
				case '%':
				case '#':
				case '&':
				case '=': dest_cur_pos +=3; break;
				default: dest_cur_pos += 1; break;
			}
		}
		return (dest_cur_pos+1);
	}
	
	for (i = 0; str_src[i]; i++)
	{
		switch(str_src[i])
		{
			case '+':
				if (dest_cur_pos + 3 >= str_dest_len)
					return -1;
				dest[dest_cur_pos++] = '%';
				dest[dest_cur_pos++] = '2';
				dest[dest_cur_pos++] = 'B';
				break;
			case ' ':
				if (dest_cur_pos + 3 > str_dest_len)
					return -1;
				dest[dest_cur_pos++] = '%';
				dest[dest_cur_pos++] = '2';
				dest[dest_cur_pos++] = '0';
				break;
			case '/':
				if (dest_cur_pos + 3 >= str_dest_len)
					return -1;
				dest[dest_cur_pos++] = '%';
				dest[dest_cur_pos++] = '2';
				dest[dest_cur_pos++] = 'F';
				break;
			case '?':
				if (dest_cur_pos + 3 >= str_dest_len)
					return -1;
				dest[dest_cur_pos++] = '%';
				dest[dest_cur_pos++] = '3';
				dest[dest_cur_pos++] = 'F';
				break;
			case '%':
				if (dest_cur_pos + 3 >= str_dest_len)
					return -1;
				dest[dest_cur_pos++] = '%';
				dest[dest_cur_pos++] = '2';
				dest[dest_cur_pos++] = '5';
				break;
			case '#':
				if (dest_cur_pos + 3 >= str_dest_len)
					return -1;
				dest[dest_cur_pos++] = '%';
				dest[dest_cur_pos++] = '2';
				dest[dest_cur_pos++] = '3';
				break;
			case '&':
				if (dest_cur_pos + 3 >= str_dest_len)
					return -1;
				dest[dest_cur_pos++] = '%';
				dest[dest_cur_pos++] = '2';
				dest[dest_cur_pos++] = '6';
				break;
			case '=':
				if (dest_cur_pos + 3 >= str_dest_len)
					return -1;
				dest[dest_cur_pos++] = '%';
				dest[dest_cur_pos++] = '3';
				dest[dest_cur_pos++] = 'D';
				break;
			default:
				if (dest_cur_pos + 1 >= str_dest_len)
					return -1;
				dest[dest_cur_pos++] = str_src[i];
				break;
		}
	}
	dest[dest_cur_pos] = 0;
	return dest_cur_pos;
}

static int get_asr_result(
	BAIDU_ACOUNT_T *_acount,
	char* const _str_ret,
	const size_t _str_ret_len,
	const unsigned char* _data,
	const size_t _data_len,
	const char *_token,
	BAIDU_ASR_MODE_T mode,
	BAIDU_ASR_AUDIO_TYPE_T audio_type)
{
	int 	err_no 		= BAIDU_ASR_SUCCESS;

	_ConnectSockt_t * con_sock;
	if (!(con_sock = esp32_malloc(sizeof(_ConnectSockt_t)))) {
		err_no = BAIDU_ASR_FAIL;
		DEBUG_LOGE(TAG_TTS, "esp32_malloc for inner data fail");
		goto GET_ASR_RESULT_ERROR;
	}
	con_sock->sock = 0;
	if (sock_get_server_info(_acount->asr_url, con_sock->domain, con_sock->port, con_sock->params) != 0)
	{
		DEBUG_LOGE(TAG_ASR, "sock_get_server_info fail");
		err_no = BAIDU_ASR_FAIL;
		goto GET_ASR_RESULT_ERROR;
	}

	con_sock->sock = sock_connect(con_sock->domain, con_sock->port);
	if (con_sock->sock == INVALID_SOCK) {
		DEBUG_LOGE(TAG_ASR, "sock_connect fail");
		err_no = BAIDU_ASR_FAIL;
		goto GET_ASR_RESULT_ERROR;
	}
	sock_set_nonblocking(con_sock->sock);

	//post http header
	if (audio_type == BAIDU_ASR_AUDIO_TYPE_AMRNB) {
		snprintf(con_sock->header, sizeof(con_sock->header), GET_ASR_REQUEST_HEADER, 
			con_sock->params, mode, _token, con_sock->domain, 8000, _data_len);
	} else {
		snprintf(con_sock->header, sizeof(con_sock->header), GET_ASR_REQUEST_HEADER, 
			con_sock->params, mode, _token, con_sock->domain, 16000, _data_len);
	}
	
	if (sock_writen_with_timeout(con_sock->sock, con_sock->header, strlen(con_sock->header), 1000) != strlen(con_sock->header)) {
		DEBUG_LOGE(TAG_ASR, "sock_write http header fail");
		err_no = BAIDU_ASR_FAIL;
		goto GET_ASR_RESULT_ERROR;
	}
	//DEBUG_LOGE(TAG_ASR, "header=%s", header);
	//post http body
	if (sock_writen_with_timeout(con_sock->sock, _data, _data_len, 3000) != _data_len) {
		DEBUG_LOGE(TAG_ASR, "sock_write http body fail");
		err_no = BAIDU_ASR_FAIL;
		goto GET_ASR_RESULT_ERROR;
	}

	/* Read HTTP response */
	memset(con_sock->buffer, 0, sizeof(con_sock->buffer));
	sock_readn_with_timeout(con_sock->sock, con_sock->buffer, sizeof(con_sock->buffer) -1, 3500);
	//DEBUG_LOGE(TAG_ASR, "ASR 杩斿洖瀹屾暣鏁版嵁锛歴tr_reply=%s\n", str_buf);
	
	if (http_get_error_code(con_sock->buffer) != 200)
	{
		DEBUG_LOGE(TAG_ASR, "reply:[%s]", con_sock->buffer);
		err_no = BAIDU_ASR_FAIL;
		goto GET_ASR_RESULT_ERROR;
	}
	
	char *pBody = http_get_body(con_sock->buffer);
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
	if (con_sock) {
		sock_close(con_sock->sock);
		esp32_free(con_sock);
	}
	return err_no;
}

static int baidu_asr(
	BAIDU_ACOUNT_T *_acount,
	char* str_reply, 
	size_t str_reply_len, 
	unsigned char *_data, 
	size_t _data_len,
	BAIDU_ASR_MODE_T mode,
	BAIDU_ASR_AUDIO_TYPE_T audio_type)
{
	//get access token
	if (strlen(g_str_token) == 0) {
		if (BAIDU_TTS_SUCCESS != _get_access_token(_acount, g_str_token, sizeof(g_str_token))) 	{
			DEBUG_LOGE(TAG_TTS, "get_access_token failed");
			return BAIDU_ASR_FAIL;
		}
	}

	//post audio data and get result 
	return get_asr_result(_acount, str_reply, str_reply_len, _data, _data_len, g_str_token, mode, audio_type);
}

int baidu_asr_amrnb(
	BAIDU_ACOUNT_T *_acount,
	char* str_reply, 
	size_t str_reply_len, 
	unsigned char *_data, 
	size_t _data_len,
	BAIDU_ASR_MODE_T mode)
{
	return baidu_asr(_acount, str_reply, str_reply_len, _data, _data_len, mode, BAIDU_ASR_AUDIO_TYPE_AMRNB);
}

int baidu_asr_amrwb(
	BAIDU_ACOUNT_T *_acount,
	char* str_reply, 
	size_t str_reply_len, 
	unsigned char *_data, 
	size_t _data_len,
	BAIDU_ASR_MODE_T mode)
{
	return baidu_asr(_acount, str_reply, str_reply_len, _data, _data_len, mode, BAIDU_ASR_AUDIO_TYPE_AMRWB);
}

int baidu_tts(
	BAIDU_ACOUNT_T *_acount, 
	char *str_url, 
	size_t str_url_len, 
	char *str_tts_text)
{
	//get access token
	if (strlen(g_str_token) == 0) {
		if (BAIDU_TTS_SUCCESS != _get_access_token(_acount, g_str_token, sizeof(g_str_token)))
		{
			DEBUG_LOGE(TAG_TTS, "get_access_token failed");
			return BAIDU_TTS_GET_TOKEN_FAIL;
		}
	}
	int len = _replace_special_char(NULL, 0, str_tts_text);
	char *str_buf = esp32_malloc(len);
	if (!str_buf) {
		DEBUG_LOGE(TAG_TTS, "request memory failed");
		return BAIDU_TTS_GET_TOKEN_FAIL;
	}
	_replace_special_char(str_buf, len, str_tts_text);
	len = snprintf(str_url, str_url_len, BAIDU_TTS_URL, str_buf, g_str_token);
	esp32_free(str_buf);
	return (len <= str_url_len) ? BAIDU_TTS_SUCCESS : BAIDU_TTS_GET_TOKEN_FAIL;
}

