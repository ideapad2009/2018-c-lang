#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "cJSON.h"
#include "sinovoice_api.h"
#include "crypto_api.h"
#include "socket.h"
#include "debug_log.h"
#include "device_api.h"
#include "http_api.h"
#include "ctypes.h"

static const char *SINOVOICE_CAP_KEY = "asr.cloud.freetalk";
static const char *SINOVOICE_TTS_CAP_KEY = "tts.cloud.liangjiahe";
static const char *SINOVOICE_TTS_FORMAT = "mp3_24";

//接收语音识别结果
typedef enum 
{	
	SINOVOICE_RECV_STATUS_IDLE,
	SINOVOICE_RECV_STATUS_START,
	SINOVOICE_RECV_STATUS_RECVING,
	SINOVOICE_RECV_STATUS_STOP,
}SINOVOICE_RECV_STATUS_T;

typedef struct
{
	int sock;
	SINOVOICE_RECV_STATUS_T recv_status;
	int recv_len;
	char recv_buff[2048];
	char recv_buff_swap[2048];
}SINOVOICE_RECV_HANDLE_T;

//HTTP 请求缓存
typedef struct 
{
	int socket;
	int audio_index;
	char domain[64];
	char port[8];
	char params[256];
	char identify[32];		//唯一标识符
	char strTimeStamp[32];	//时间戳
	char strSessionKey[64];	//session key
	int  send_len;
	char header[640];
	int audio_buffer_len;		//音频数据大小
	char audio_buffer[16*1000];	//16k音频缓存	
	char send_buffer[16*1000+640];//16k发送缓存	
	char recv_buffer[10*1024];//10k接收缓存	
}SINOVOICE_HTTP_BUFF_T;

typedef struct
{
	SINOVOICE_HTTP_BUFF_T http_buff;
	SINOVOICE_ACOUNT_T acount;
}SINOVOICE_ASR_HANDLE_T;

static const char *POST_REQUEST_HEADER = 
	"POST %s HTTP/1.1\r\nHOST: %s:%s\r\n"\
	"Content-Type: application/octet-stream; charset=iso-8859-1\r\n"\  
    "x-app-key: %s\r\n"\
    "x-sdk-version: 5.0\r\n"\
    "x-request-date: %s\r\n"\
    "x-task-config: capkey=%s,audioformat=pcm16k16bit,"\
    "realtime=yes,identify=%s,index=%d\r\n"\
    "x-session-key: %s\r\n"\
    "x-udid: 101:1234567890\r\n"\
    "x-result-format: json\r\n"\
    "Content-Length: %d\r\n"\
    "\r\n";

static const char *TTS_POST_REQUEST_HEADER = 
	"POST %s HTTP/1.0\r\nHOST: %s:%s\r\n"\
	"Content-Type: application/octet-stream; charset=iso-8859-1\r\n"\  
	"x-app-key: %s\r\n"\
	"x-sdk-version: 5.0\r\n"\
	"x-request-date: %s\r\n"\
	"x-task-config: capkey=%s,audioformat=%s,speed=5\r\n"\
	"x-session-key: %s\r\n"\
	"x-udid: 101:1234567890\r\n"\
	"Content-Length: %d\r\n"\
	"\r\n";

static const char *TAG_ASR = "[SinoVoice ASR]";
static SINOVOICE_RECV_HANDLE_T *g_recv_result_handle = NULL;

static char* generate_session_key(
	char* str_key, 
	int str_key_len, 
	char *str_date,
	char *dev_key)
{
	unsigned char session_key[64] = {0};
	unsigned char sec_key[16] = {0};

	snprintf((char*)session_key, sizeof(session_key), "%s%s", str_date, dev_key);
	crypto_md5((unsigned char*)&session_key, strlen((char*)session_key), (unsigned char*)&sec_key);
	int i  = 0;
	memset(str_key, 0, str_key_len);
	for (i=0; i < sizeof(sec_key); i++)
	{
		snprintf((char*)session_key, sizeof(session_key), "%02X", sec_key[i]);
		strcat((char*)str_key, (char*)session_key);
	}
	
	return str_key;
}

static void generate_identify(char *str_identify, const size_t ilen)
{
	int i = 0;
	size_t len = 0;
	unsigned char strSha1[16] = {0};
	unsigned char str_buf[3] ={0};
	unsigned char str_mac[13] = {0};
	uint8_t eth_mac[6];

	memset(eth_mac, 0, sizeof(eth_mac));
	device_get_mac(eth_mac);
	for (i=0; i < sizeof(eth_mac); i++)
	{
		snprintf((char*)str_buf, sizeof(str_buf), "%02X", eth_mac[i]);
		strcat((char*)str_mac, (char*)str_buf);
	}
	crypto_sha1prng(strSha1, str_mac);
	memset(str_identify, 0, ilen);
	crypto_base64_encode((unsigned char*)str_identify, ilen, &len, strSha1, sizeof(strSha1));

	return;
}
#if 0
static int sinovoice_parse_result(PSINOVOICE_ASR_RESULT asr_result, char *str_reply)
{
	int ret = SINOVOICE_ASR_SUCCESS;
	
	if (NULL == asr_result || NULL == str_reply)
	{
		return SINOVOICE_ASR_INVALID_PARAMS;
	}	

	char* str_body = http_get_body(str_reply);
	if (NULL == str_body)
	{
		return SINOVOICE_ASR_INVALID_HTTP_CONTENT;
	}

	
	cJSON *pJson_body = cJSON_Parse(str_body);
	if (NULL != pJson_body) 
	{
		cJSON *pJson_info = cJSON_GetObjectItem(pJson_body, "ResponseInfo");
		if (NULL == pJson_info)
		{
			ret = SINOVOICE_ASR_INVALID_JSON_CONTENT;
			goto SINOVOICE_PARSE_ERROR;
		}
		
		cJSON *pJson_rescode = cJSON_GetObjectItem(pJson_info, "ResCode");
		if (NULL == pJson_rescode)
		{
			ret = SINOVOICE_ASR_INVALID_JSON_CONTENT;
			goto SINOVOICE_PARSE_ERROR;
		}
		else
		{
			if (0 != strcmp(pJson_rescode->valuestring, "Success"))
			{
				ret = SINOVOICE_ASR_FAIL;
				goto SINOVOICE_PARSE_ERROR;
			}
		}

		cJSON *pJson_result = cJSON_GetObjectItem(pJson_info, "Result");
		if (NULL == pJson_result)
		{
			ret = SINOVOICE_ASR_INVALID_JSON_CONTENT;
			goto SINOVOICE_PARSE_ERROR;
		}

		cJSON *pJson_segmemt_count = cJSON_GetObjectItem(pJson_result, "SegmentCount");
		if (NULL == pJson_segmemt_count)
		{
			ret = SINOVOICE_ASR_INVALID_JSON_CONTENT;
			goto SINOVOICE_PARSE_ERROR;
		}
		
		int segment_count = atoi(pJson_segmemt_count->valuestring);
		if (segment_count == 0)
		{
			goto SINOVOICE_PARSE_ERROR;
		}
		
		cJSON *pJson_segmemt = cJSON_GetObjectItem(pJson_result, "Segment");
		if (NULL == pJson_segmemt)
		{
			ret = SINOVOICE_ASR_INVALID_JSON_CONTENT;
			goto SINOVOICE_PARSE_ERROR;
		}
		
		cJSON *pJson_segmemt_index = cJSON_GetObjectItem(pJson_segmemt, "SegmentIndex");
		if (NULL == pJson_segmemt_index)
		{
			ret = SINOVOICE_ASR_INVALID_JSON_CONTENT;
			goto SINOVOICE_PARSE_ERROR;
		}
		asr_result->iSegmentIndex = atoi(pJson_segmemt_index->valuestring);
		
		cJSON *pJson_text = cJSON_GetObjectItem(pJson_segmemt, "Text");
		if (NULL == pJson_text)
		{
			ret = SINOVOICE_ASR_INVALID_JSON_CONTENT;
			goto SINOVOICE_PARSE_ERROR;
		}
		
		if (pJson_text->valuestring != NULL)
		{
			snprintf(asr_result->achText, sizeof(asr_result->achText), "%s", pJson_text->valuestring);
		}
		
		cJSON *pJson_score = cJSON_GetObjectItem(pJson_segmemt, "Score");
		if (NULL == pJson_score)
		{
			ret = SINOVOICE_ASR_INVALID_JSON_CONTENT;
			goto SINOVOICE_PARSE_ERROR;
		}
		asr_result->iScore = atoi(pJson_score->valuestring);
		
		cJSON *pJson_start_time = cJSON_GetObjectItem(pJson_segmemt, "StartTime");
		if (NULL == pJson_start_time)
		{
			ret = SINOVOICE_ASR_INVALID_JSON_CONTENT;
			goto SINOVOICE_PARSE_ERROR;
		}
		asr_result->iStartTime = atoi(pJson_start_time->valuestring);
		
		cJSON *pJson_end_time = cJSON_GetObjectItem(pJson_segmemt, "EndTime");
		if (NULL == pJson_end_time)
		{
			ret = SINOVOICE_ASR_INVALID_JSON_CONTENT;
			goto SINOVOICE_PARSE_ERROR;
		}
		asr_result->iEndTime= atoi(pJson_end_time->valuestring);
		asr_result->iValid = 1;
	}

SINOVOICE_PARSE_ERROR:

	if (NULL != pJson_body)
	{
		cJSON_Delete(pJson_body);
	}
	
	return ret;
}
#endif

static void set_recv_result_status(SINOVOICE_RECV_STATUS_T status)
{
	if (g_recv_result_handle != NULL)
	{
		g_recv_result_handle->recv_status = status;
	}
}

static void set_recv_result_sock(int sock)
{
	if (g_recv_result_handle != NULL)
	{
		g_recv_result_handle->sock = sock;
	}
}


static void task_recv_asr_result(void *_handler)
{
	int ret = 0;
	SINOVOICE_RECV_HANDLE_T *handle = (SINOVOICE_RECV_HANDLE_T *)esp32_malloc(sizeof(SINOVOICE_RECV_HANDLE_T));
	if (handle == NULL)
	{
		DEBUG_LOGE(TAG_ASR, "task_recv_asr_result create failed");
		return;
	}
	g_recv_result_handle = handle;
	memset(handle, 0, sizeof(SINOVOICE_RECV_HANDLE_T));
	
	while (1)
	{
		vTaskDelay(10);
		switch (handle->recv_status)
		{
			case SINOVOICE_RECV_STATUS_IDLE:
			{
				break;
			}
			case SINOVOICE_RECV_STATUS_START:
			{
				handle->recv_len = 0;
				handle->recv_status = SINOVOICE_RECV_STATUS_RECVING;
				break;
			}
			case SINOVOICE_RECV_STATUS_RECVING:
			{
				if (handle->sock <= 0)
				{
					break;
				}
				
				ret = sock_readn_with_timeout(
					handle->sock, handle->recv_buff + handle->recv_len, sizeof(handle->recv_buff) - handle->recv_len - 1, 1000);
				if (ret > 0)
				{
					handle->recv_len += ret;
					//find http head start
					char *http_start = strstr(handle->recv_buff, "HTTP/");
					if (http_start == NULL)
					{
						break;
					}

					//find http head end
					char *http_end = strstr(handle->recv_buff, "\r\n\r\n");
					if (http_end == NULL)
					{
						break;
					}

					//find http content length
					int content_len = http_get_content_length(handle->recv_buff);
					if (content_len < 0)
					{
						break;
					}

					if (http_get_error_code(handle->recv_buff) == 200)
					{
						char *http_body = http_get_body(handle->recv_buff);
						if (http_body != NULL)
						{
							int body_len = handle->recv_len - (http_body - handle->recv_buff);
							if (body_len == content_len)
							{
								memset(handle->recv_buff_swap, 0, sizeof(handle->recv_buff_swap));
								memcpy(handle->recv_buff_swap, http_body, content_len);
								ESP_LOGE(TAG_ASR, "[%s]", handle->recv_buff_swap);
							}
							else if (body_len > content_len)
							{
								memset(handle->recv_buff_swap, 0, sizeof(handle->recv_buff_swap));
								memcpy(handle->recv_buff_swap, http_body, content_len);
								ESP_LOGE(TAG_ASR, "[%s]", handle->recv_buff_swap);
								handle->recv_len = body_len - content_len;
								memcpy(handle->recv_buff_swap, http_body + content_len, handle->recv_len);
								memcpy(handle->recv_buff, handle->recv_buff_swap, handle->recv_len);
							}
							else
							{
								break;
							}
						}
					}
					else
					{
						handle->recv_len = 0;
						ESP_LOGE(TAG_ASR, "str_reply=[%s]", handle->recv_buff);
					}
				}
				break;
			}
			case SINOVOICE_RECV_STATUS_STOP:
			{
				break;
			}
			default:
				break;
		}		
	}
}

static SINOVOICE_ASR_ERRNO_T sinovoice_make_packet(
	SINOVOICE_ASR_HANDLE_T *_handler, const char *data, const uint32_t data_len)
{
	int left_len = 0;
	SINOVOICE_ASR_HANDLE_T *handler = (SINOVOICE_ASR_HANDLE_T *)_handler;
	if (_handler == NULL)
	{
		return SINOVOICE_ASR_INVALID_PARAMS;
	}
	SINOVOICE_HTTP_BUFF_T *http_buff = &handler->http_buff;

	//拼接为16k数据上传
	if (http_buff->audio_buffer_len + data_len >= sizeof(http_buff->audio_buffer))
	{
		memcpy(http_buff->audio_buffer + http_buff->audio_buffer_len, data, sizeof(http_buff->audio_buffer) - http_buff->audio_buffer_len);
		left_len = http_buff->audio_buffer_len + data_len - sizeof(http_buff->audio_buffer);
		http_buff->audio_buffer_len = sizeof(http_buff->audio_buffer);
	}
	else
	{
		memcpy(http_buff->audio_buffer + http_buff->audio_buffer_len, data, data_len);
		http_buff->audio_buffer_len += data_len;
		if (data_len != 0)
		{
			return SINOVOICE_ASR_OK;
		}
	}
	
	http_buff->audio_index++;
	if (http_buff->audio_index == 1)
	{
		generate_identify(&http_buff->identify, sizeof(http_buff->identify));
	}

	if (data_len == 0)
	{
		http_buff->audio_index *= -1;
	}

	crypto_time_stamp((unsigned char*)http_buff->strTimeStamp, sizeof(http_buff->strTimeStamp));
	generate_session_key(http_buff->strSessionKey, sizeof(http_buff->strSessionKey), http_buff->strTimeStamp, handler->acount.dev_key);

	snprintf(http_buff->send_buffer, sizeof(http_buff->send_buffer), POST_REQUEST_HEADER, 
		http_buff->params, http_buff->domain, http_buff->port, handler->acount.app_key, 
		http_buff->strTimeStamp, SINOVOICE_CAP_KEY, http_buff->identify, http_buff->audio_index, 
		http_buff->strSessionKey, http_buff->audio_buffer_len);
	http_buff->send_len = strlen(http_buff->send_buffer);
	DEBUG_LOGE(TAG_ASR, "header:%s", http_buff->send_buffer);

	if (http_buff->audio_buffer_len > 0)
	{
		memcpy(http_buff->send_buffer + http_buff->send_len, http_buff->audio_buffer, http_buff->audio_buffer_len);
		http_buff->send_len += http_buff->audio_buffer_len;
	}

	if (http_buff->audio_buffer_len == sizeof(http_buff->audio_buffer))
	{
		http_buff->audio_buffer_len = 0;
	}

	if (left_len > 0)
	{
		memcpy(http_buff->audio_buffer + http_buff->audio_buffer_len, data + (data_len - left_len), left_len);
		http_buff->audio_buffer_len += left_len;
	}
	
	return SINOVOICE_ASR_OK;
}

void *sinovoice_asr_create(SINOVOICE_ACOUNT_T *acount)
{
	SINOVOICE_HTTP_BUFF_T *http_buff = NULL;
	SINOVOICE_ASR_HANDLE_T *handler = (SINOVOICE_ASR_HANDLE_T *)esp32_malloc(sizeof(SINOVOICE_ASR_HANDLE_T));
	if (handler == NULL)
	{
		return NULL;
	}
	memset(handler, 0, sizeof(SINOVOICE_ASR_HANDLE_T));
	http_buff = &handler->http_buff;
	http_buff->socket = INVALID_SOCK;
	handler->acount = *acount;

	if (g_recv_result_handle == NULL)
	{
		if (xTaskCreate(task_recv_asr_result,
	                    "task_recv_asr_result",
	                    512*4,
	                    NULL,
	                    10,
	                    NULL) != pdPASS) 
	    {
	        DEBUG_LOGE(TAG_ASR, "create task_recv_asr_result failed");
	    }
	}
	
	if (sock_get_server_info(acount->asr_url, 
			&http_buff->domain, &http_buff->port, &http_buff->params) != 0)
	{
		DEBUG_LOGE(TAG_ASR, "sock_get_server_info fail");
		goto sinovoice_asr_create_error;
	}
	
	http_buff->socket = sock_connect(http_buff->domain, http_buff->port);
	if (http_buff->socket == INVALID_SOCK)
	{
		DEBUG_LOGE(TAG_ASR, "sock_connect fail");
		goto sinovoice_asr_create_error;
	}
	sock_set_nonblocking(http_buff->socket);
	set_recv_result_sock(http_buff->socket);
	set_recv_result_status(SINOVOICE_RECV_STATUS_START);

	return handler;
sinovoice_asr_create_error:
	if (handler != NULL)
	{
		esp32_free(handler);
		handler = NULL;
	}
	
	return handler;
}

SINOVOICE_ASR_ERRNO_T sinovoice_asr_write(void *_handler, const char *data, const uint32_t data_len)
{
	int ret = 0;
	SINOVOICE_HTTP_BUFF_T *http_buff = NULL;
	SINOVOICE_ASR_HANDLE_T* handler = (SINOVOICE_ASR_HANDLE_T *)_handler;
	if (_handler == NULL)
	{
		return SINOVOICE_ASR_INVALID_PARAMS;
	}	
	http_buff = &handler->http_buff;

	ret = sinovoice_make_packet(handler, data, data_len);
	if (ret != SINOVOICE_ASR_OK)
	{
		return ret;
	}

	ret = sock_writen_with_timeout(http_buff->socket, http_buff->send_buffer, http_buff->send_len, 2000);
	if (ret != http_buff->send_len) 
	{
		DEBUG_LOGE(TAG_ASR, "sock_writen send data body fail,[%d][%d]", http_buff->send_len, ret);
		return SINOVOICE_ASR_FAIL;
	}
	
	return SINOVOICE_ASR_OK;	
}

SINOVOICE_ASR_ERRNO_T sinovoice_asr_result(
	void *asr_handle, char* str_reply, const size_t str_reply_len)
{	
	int i = 0;
	int ret = 0;
	int total_len = 0;
	SINOVOICE_HTTP_BUFF_T *http_buff = NULL;	
	SINOVOICE_ASR_HANDLE_T *handle = (SINOVOICE_ASR_HANDLE_T *)asr_handle;
	
	if (asr_handle == NULL)
	{
		return SINOVOICE_ASR_INVALID_PARAMS;
	}
	http_buff = &handle->http_buff;

	ret = sinovoice_asr_write(handle, NULL, 0);
	if (ret != SINOVOICE_ASR_OK)
	{
		return ret;
	}

	vTaskDelay(3000);
	
	return SINOVOICE_ASR_OK;
}

void sinovoice_asr_delete(void *_handler)
{
	SINOVOICE_ASR_HANDLE_T *handle = (SINOVOICE_ASR_HANDLE_T *)_handler;

	set_recv_result_status(SINOVOICE_RECV_STATUS_STOP);
	if (_handler != NULL)
	{
		if (handle->http_buff.socket != INVALID_SOCK)
		{
			sock_close(handle->http_buff.socket);
		}
		
		esp32_free(handle);
	}
	
	return;
}


void sinovoice_tts_generate(char *_text)
{
	int 	err_no		= 0;
	char	domain[64]	= {0};
	char	port[6] 	= {0};
	char	params[256] = {0};
	char	header[512] = {0};
	char	str_buf[1024]= {0};
	char    strTimeStamp[32] = {0};
	char    strSessionKey[64] = {0};
	sock_t	sock		= INVALID_SOCK;
	cJSON *pJson_body 	= NULL;
	int audio_len 		= 0;
	int audio_total_len = 0;
	static int count = 0;
	char *data_start = NULL;
	
	if (sock_get_server_info(SINOVOICE_DEFAULT_TTS_URL, &domain, &port, &params) != 0)
	{
		DEBUG_LOGE(TAG_ASR, "sock_get_server_info fail");
		err_no = SINOVOICE_TTS_FAIL;
		goto sinovoice_tts_generate_error;
	}

	sock = sock_connect(domain, port);
	if (sock == INVALID_SOCK)
	{
		DEBUG_LOGE(TAG_ASR, "sock_connect fail");
		err_no = SINOVOICE_TTS_FAIL;
		goto sinovoice_tts_generate_error;
	}
	sock_set_nonblocking(sock);

	crypto_time_stamp((unsigned char*)strTimeStamp, sizeof(strTimeStamp));
	generate_session_key(strSessionKey, sizeof(strSessionKey), strTimeStamp, SINOVOICE_DEFAULT_DEV_KEY);

	//post http header
	snprintf(header, sizeof(header), TTS_POST_REQUEST_HEADER, params, domain, port, 
		SINOVOICE_DEFAULT_APP_KEY, strTimeStamp, SINOVOICE_TTS_CAP_KEY, SINOVOICE_TTS_FORMAT, strSessionKey, strlen(_text));
	DEBUG_LOGE(TAG_ASR, "header=%s\n", header);
	if (sock_writen_with_timeout(sock, header, strlen(header), 2000) != strlen(header)) 
	{
		DEBUG_LOGE(TAG_ASR, "sock_write http header fail");
		err_no = SINOVOICE_TTS_FAIL;
		goto sinovoice_tts_generate_error;
	}
	//DEBUG_LOGE(TAG_ASR, "header=%s", header);
	//post http body
	if (sock_writen_with_timeout(sock, _text, strlen(_text), 5000) != strlen(_text)) 
	{
		DEBUG_LOGE(TAG_ASR, "sock_write http body fail");
		err_no = SINOVOICE_TTS_FAIL;
		goto sinovoice_tts_generate_error;
	}

	/* Read HTTP response */
	memset(str_buf, 0, sizeof(str_buf));
	int recv_len = sock_readn_with_timeout(sock, str_buf, sizeof(str_buf), 5000);
	DEBUG_LOGE(TAG_ASR, "recv_len=%d\n", recv_len);

	if (recv_len <= 0)
	{
		DEBUG_LOGE(TAG_ASR, "sock_readn_with_timeout recv ret=%d", recv_len);
		err_no = SINOVOICE_TTS_FAIL;
		goto sinovoice_tts_generate_error;
	}
	
	if (http_get_error_code(str_buf) != 200)
	{
		DEBUG_LOGE(TAG_ASR, "reply:[%s]", str_buf);
		err_no = SINOVOICE_TTS_FAIL;
		goto sinovoice_tts_generate_error;
	}

	data_start = strstr(str_buf, "</ResponseInfo>");
	if (data_start == NULL)
	{
		err_no = SINOVOICE_TTS_FAIL;
		goto sinovoice_tts_generate_error;
	}
	data_start += strlen("</ResponseInfo>");
	audio_len = recv_len - (data_start - str_buf);
	#if 0
	int content_length = http_get_content_length(str_buf);
	if (content_length <= 0 || content_length >= sizeof(header))
	{
		goto sinovoice_tts_generate_error;
	}

	char *pBody = http_get_body(str_buf);
	if (NULL == pBody)
	{
		err_no = SINOVOICE_TTS_FAIL;
		goto sinovoice_tts_generate_error;
	}

	int content_len_now = recv_len - (pBody - str_buf);
	if (content_len_now < content_length)
	{
		err_no = SINOVOICE_TTS_FAIL;
		goto sinovoice_tts_generate_error;
	}
	audio_len = content_len_now - content_length;
	data_start = pBody + content_length;

	memset(header, 0, sizeof(header));
	memcpy(header, pBody, content_length);
	DEBUG_LOGE(TAG_ASR, "content:[%s]", header);
	pJson_body = cJSON_Parse(header);
	if (NULL != pJson_body) 
	{
		cJSON *pJson_info = cJSON_GetObjectItem(pJson_body, "ResponseInfo");
		if (NULL == pJson_info)
		{
			err_no = SINOVOICE_TTS_FAIL;
			goto sinovoice_tts_generate_error;
		}
		
		cJSON *pJson_rescode = cJSON_GetObjectItem(pJson_info, "ResCode");
		if (NULL == pJson_rescode || pJson_rescode->valuestring == NULL)
		{
			err_no = SINOVOICE_TTS_FAIL;
			goto sinovoice_tts_generate_error;
		}
		else
		{
			if (0 != strcmp(pJson_rescode->valuestring, "Success"))
			{
				err_no = SINOVOICE_TTS_FAIL;
				goto sinovoice_tts_generate_error;
			}
		}

		cJSON *pJson_data_len = cJSON_GetObjectItem(pJson_info, "Data_Len");
		if (NULL == pJson_data_len || pJson_data_len->valuestring == NULL)
		{
			err_no = SINOVOICE_TTS_FAIL;
			goto sinovoice_tts_generate_error;
		}

		audio_total_len = atoi(pJson_data_len->valuestring);
		if (audio_total_len <= 0)
		{
			err_no = SINOVOICE_TTS_FAIL;
			goto sinovoice_tts_generate_error;
		}
	}
	#endif
	
	char str_record_path[32] = {0};
	snprintf(str_record_path, sizeof(str_record_path), "/sdcard/%d.mp3", ++count);
	FILE* fp = fopen(str_record_path, "w+");	
	DEBUG_LOGE(TAG_ASR, "1[%d][%s]", audio_total_len, str_record_path);
	if (fp == NULL)
	{
		err_no = SINOVOICE_TTS_FAIL;
		goto sinovoice_tts_generate_error;
	}

	if (audio_len > 0)
	{
		fwrite(data_start, audio_len, 1, fp);
	}
	audio_total_len = audio_len;
	while(1)
	{
		audio_len = sock_readn_with_timeout(sock, str_buf, sizeof(str_buf), 3000);
		if (audio_len > 0)
		{
			fwrite(str_buf, audio_len, 1, fp);
			audio_total_len += audio_len;
		}
		else
		{
			break;
		}
	}
	DEBUG_LOGE(TAG_ASR, "2[%d][%s]", audio_total_len, str_record_path);
	fclose(fp);
sinovoice_tts_generate_error:

	if (NULL != pJson_body)
	{
		cJSON_Delete(pJson_body);
	} 
	sock_close(sock);

	return err_no;
}


