
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

static const char *POST_REQUEST_HEADER = 
	"POST %s HTTP/1.1\r\nHOST: %s:%s\r\n"\
	"Content-Type: application/octet-stream; charset=iso-8859-1\r\n"\  
    "x-app-key: %s\r\n"\
    "x-sdk-version: 5.0\r\n"\
    "x-request-date: %s\r\n"\
    "x-task-config: capkey=%s,audioformat=pcm16k16bit,"\
    "realtime=rt,vadSwitch=yes,vadHead=10000,vadSeg=30000,vadThreshold=10,identify=%s,index=%d\r\n"\
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

static int _get_server_sock(SINOVOICE_ASR_HANDLER *_pHandler)
{
	xSemaphoreTake(_pHandler->lock_handle, portMAX_DELAY);
	int sock = _pHandler->sock;
	xSemaphoreGive(_pHandler->lock_handle);
	
	return sock;
}

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

static void sinovoice_asr_end(PSINOVOICE_ASR_HANDLER handler)
{
	if (handler->sock > 0)
	{
		sock_close(handler->sock);
		handler->sock = INVALID_SOCK;
	}
}

static int sinovoice_asr_begin(PSINOVOICE_ASR_HANDLER handler)
{
	sinovoice_asr_end(handler);

	if (sock_get_server_info(handler->acount.asr_url, 
		&handler->ach_domain, &handler->ach_port, &handler->ach_params) != 0)
	{
		DEBUG_LOGE(TAG_ASR, "sock_get_server_info fail");
		return SINOVOICE_ASR_SOCKET_FAIL;
	}

	handler->sock = sock_connect(handler->ach_domain, handler->ach_port);
	if (handler->sock == INVALID_SOCK)
	{
		DEBUG_LOGE(TAG_ASR, "sock_connect fail");
		return SINOVOICE_ASR_SOCKET_FAIL;
	}
	DEBUG_LOGE(TAG_ASR, "handler->sock=%d", handler->sock);

	sock_set_nonblocking(handler->sock);

	return SINOVOICE_ASR_SOCKET_SUCCESS;
}

typedef enum
{
	RECV_HTTP_STATUS_HEAD = 0,
	RECV_HTTP_STATUS_HTTP_LENGTH,
	RECV_HTTP_STATUS_HTTP_BODY,
}RECV_HTTP_STATUS_T;

void _recv_asr_result(void *_handler)
{
	fd_set readfds;
	struct timeval timeout;
	int sock = INVALID_SOCK;
	int ret = 0;
	RECV_HTTP_STATUS_T recv_status = RECV_HTTP_STATUS_HEAD;
	SINOVOICE_ASR_HANDLER *pHandler = (SINOVOICE_ASR_HANDLER *)_handler;
	uint32_t head_len = 0;
	uint32_t recv_len = 0;
	char reply_buf[1024+1] = {0};
	SINOVOICE_ASR_RESULT asr_result = {0};
	memset(reply_buf, 0, sizeof(reply_buf));
	
	while (1)
	{
		vTaskDelay(100);
		sock = _get_server_sock(pHandler);
		if (sock == INVALID_SOCK)
		{
			vTaskDelay(100);
			continue;
		}

		FD_ZERO(&readfds);
		timeout.tv_sec	= 2;
		timeout.tv_usec = 0;
		FD_SET(sock, &readfds);

		int ret = select(sock + 1, &readfds, NULL, NULL, &timeout);
		//ESP_LOGE(TAG_ASR, "select ret=%d\r\n", ret);
		if (ret == -1) 
		{
			ESP_LOGE(TAG_ASR, "select failed(%d:%s)\r\n", errno, strerror(errno));
			vTaskDelay(100);
			break;
		} 
		else if (ret == 0) 
		{
			continue;
		}
		else
		{
			if (FD_ISSET(sock, &readfds))
			{
				ESP_LOGE(TAG_ASR, "asr result is arrived\r\n");
			}
			else
			{
				continue;			
			}
		}
		
		if (recv_status == RECV_HTTP_STATUS_HEAD)
		{
			ret = sock_read(sock, reply_buf + head_len, sizeof(reply_buf) - head_len - 1);
			//ESP_LOGE(TAG_ASR, "\r\nhead_len=%d,ret=%d,%s\r\n", head_len, ret, reply_buf);
			if (ret > 0)
			{
				head_len += ret;
				//find http head start
				char *http_start = strstr(reply_buf, "HTTP/");
				if (http_start != &reply_buf)
				{
					head_len = 0;
					memset(reply_buf, 0, sizeof(reply_buf));
					continue;
				}

				//find http head end
				char *http_end = strstr(reply_buf, "\r\n\r\n");
				if (http_end == NULL)
				{
					continue;
				}

				//find http content length
				char *http_body_len = strstr(reply_buf, "Content-Length");
				if (http_body_len == NULL)
				{
					head_len = 0;
					memset(reply_buf, 0, sizeof(reply_buf));
					continue;
				}
				ESP_LOGE(TAG_ASR, "\r\n%s\r\n", reply_buf);

				if (http_get_error_code(reply_buf) == 200)
				{
					memset(&asr_result, 0, sizeof(asr_result));
					if (sinovoice_parse_result(&asr_result, reply_buf) == SINOVOICE_ASR_SUCCESS)
					{
						ESP_LOGE(TAG_ASR, "ASR RESULT=[%s]", asr_result.achText);
						if (pHandler->cb_asr_result)
						{
							pHandler->cb_asr_result(asr_result.achText);
						}
						sinovoice_asr_end(_handler);
					}
				}
				else
				{
					ESP_LOGE(TAG_ASR, "str_reply=[%s]", reply_buf);
				}

				head_len = 0;
				memset(reply_buf, 0, sizeof(reply_buf));
			}
			else
			{
				ESP_LOGE(TAG_ASR, "\r\n%d\r\n", ret);
			}
		}
			
	}
}


void *sinovoice_asr_create(cb_asr_result _cb)
{
	SINOVOICE_ASR_HANDLER *handler = (SINOVOICE_ASR_HANDLER *)esp32_malloc(sizeof(SINOVOICE_ASR_HANDLER));
	if (handler != NULL)
	{
		memset(handler, 0, sizeof(SINOVOICE_ASR_HANDLER));
	}
	handler->sock = INVALID_SOCK;
	handler->cb_asr_result = _cb;

	handler->lock_handle = xSemaphoreCreateMutex();
	if (xTaskCreate(_recv_asr_result,
                    "_recv_asr_result",
                    1024*4,
                    handler,
                    10,
                    NULL) != pdPASS) 
    {
        ESP_LOGE(TAG_ASR, "create _recv_asr_result failed");
    }

	return handler;
}

int sinovoice_asr_write(
	void *_handler,
	PSINOVOICE_ASR_RECORD _record,
	PSINOVOICE_ASR_RESULT _asr_result)
{
	int  ret = 0;
	PSINOVOICE_ASR_HANDLER handler = (PSINOVOICE_ASR_HANDLER)_handler;

	if (_handler == NULL || _record == NULL || _asr_result == NULL)
	{
		return SINOVOICE_ASR_INVALID_PARAMS;
	}

	if (_record->audio_index == 1)
	{
		generate_identify(&handler->identify, sizeof(handler->identify));
		sinovoice_asr_begin(handler);
	}

	if (_record->last_audio)
	{
		_record->audio_index *= -1;
	}

	if (handler->sock == INVALID_SOCK)
	{
		sinovoice_asr_begin(handler);
		if (handler->sock == INVALID_SOCK)
		{
			return SINOVOICE_ASR_SOCKET_FAIL;
		}
	}

	crypto_time_stamp((unsigned char*)handler->strTimeStamp, sizeof(handler->strTimeStamp));
	generate_session_key(handler->strSessionKey, sizeof(handler->strSessionKey), handler->strTimeStamp, handler->acount.dev_key);

	snprintf(handler->header, sizeof(handler->header), POST_REQUEST_HEADER, 
		handler->ach_params, handler->ach_domain, handler->ach_port, handler->acount.app_key, 
		handler->strTimeStamp, SINOVOICE_CAP_KEY, handler->identify, _record->audio_index, 
		handler->strSessionKey, _record->audio_len);
	DEBUG_LOGE(TAG_ASR, "header:%s", handler->header);
	if (sock_writen_with_timeout(handler->sock, handler->header, strlen(handler->header), 1000) != strlen(handler->header)) 
	{
		DEBUG_LOGE(TAG_ASR, "sock_writen http header fail");
		goto SINOVOICE_ASR_WRITE_ERROR;
	}

	ret = sock_writen_with_timeout(handler->sock, _record->audio_data, _record->audio_len, 3000);
	if (ret != _record->audio_len) 
	{
		DEBUG_LOGE(TAG_ASR, "sock_writen send data body fail,ret=%d", ret);
		goto SINOVOICE_ASR_WRITE_ERROR;
	}
	
	return SINOVOICE_ASR_SUCCESS;
	
SINOVOICE_ASR_WRITE_ERROR:
	
	sinovoice_asr_end(handler);
	return SINOVOICE_ASR_FAIL;
}


void sinovoice_asr_destroy(void *_handler)
{
	if (_handler != NULL)
	{
		esp32_free(_handler);
	}
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


