#include <stdio.h>
#include <string.h>
#include "debug_log.h"
#include "socket.h"
#include "ctypes.h"
#include "device_api.h"
#include "memory_manage.h"
#include "mpush_message.h"
#include "mpush_client_api.h"
#include "mbedtls/pk.h"
#include "mbedtls/rsa.h"
#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include "lwip/netdb.h"
#include "lwip/dns.h"
#include "http_api.h"
#include "cJSON.h"
#include "flash_config_manage.h"
#include "userconfig.h"
#include "crypto_api.h"
#include "translate_manage.h"
#include "auto_play_service.h"
#include "ReSampling.h"
#include "interf_enc.h"
#include "keyboard_manage.h"
#include "baidu_api.h"
#include "free_talk.h"
#include "player_middleware.h"
#include "baidu_api.h"
#include "debug_log.h"

//#define DEBUG_PRINT

static TRANSLATE_QUEUE_HANDLE_T *g_translate_queue_handle = NULL;
static TRANSLATE_MANAGE_MODE_FLAG_T g_translate_mode_flag = TRANSLATE_MANAGE_MODE_FLAG_INIT;

static int translate_and_return_translate_result(char *need_translate_text, char *translate_result_text)
{
	sock_t	sock   = INVALID_SOCK;
	cJSON	*pJson = NULL;
	
	TRANSLATE_HTTP_BUFFER *buffer = esp32_malloc(sizeof(TRANSLATE_HTTP_BUFFER));
	if (buffer != NULL)
	{
		memset(buffer, 0, sizeof(TRANSLATE_HTTP_BUFFER));
	}
	else
	{
		return TRANSLATE_MANAGE_RET_GET_RESULT_FAIL;
	}
	
	if (sock_get_server_info(TRANSLATE_SEND_MSG_URL, &buffer->domain, &buffer->port, &buffer->params) != 0)
	{
		ESP_LOGE(TRANSLATE_TAG, "get_translate_result,sock_get_server_info fail\r\n");
		goto get_translate_result_error;
	}

	sock = sock_connect(buffer->domain, buffer->port);
	if (sock == INVALID_SOCK)
	{
		ESP_LOGE(TRANSLATE_TAG, "get_translate_result,sock_connect fail,%s,%s\r\n", buffer->domain, buffer->port);
		goto get_translate_result_error;
	}
	
	sock_set_nonblocking(sock);
	crypto_generate_request_id(buffer->str_nonce, sizeof(buffer->str_nonce));
	get_flash_cfg(FLASH_CFG_DEVICE_ID, buffer->str_device_id);
	snprintf(buffer->str_comm_buf, sizeof(buffer->str_comm_buf),
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
		DEEP_BRAIN_APP_ID,
		need_translate_text,
		"",
		"",
		buffer->str_device_id,
		buffer->str_nonce,
		DEEP_BRAIN_ROBOT_ID,
		buffer->str_device_id);
	crypto_generate_nonce((uint8_t *)buffer->str_nonce, sizeof(buffer->str_nonce));
	crypto_time_stamp((unsigned char*)buffer->str_timestamp, sizeof(buffer->str_timestamp));
	crypto_generate_private_key((uint8_t *)buffer->str_private_key, sizeof(buffer->str_private_key), buffer->str_nonce, buffer->str_timestamp, DEEP_BRAIN_ROBOT_ID);
	snprintf(buffer->str_buf, sizeof(buffer->str_buf), 
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
		buffer->params, buffer->domain, buffer->port, strlen(buffer->str_comm_buf), buffer->str_nonce, buffer->str_timestamp, buffer->str_private_key, DEEP_BRAIN_ROBOT_ID);

	//ESP_LOGE(PRINT_TAG, "%s", str_buf);
	//ESP_LOGE(PRINT_TAG, "%s", _pHandler->str_comm_buf);
	if (sock_writen_with_timeout(sock, buffer->str_buf, strlen(buffer->str_buf), 1000) != strlen(buffer->str_buf)) 
	{
		printf("sock_writen http header fail\r\n");
		goto get_translate_result_error;
	}

	if (sock_writen_with_timeout(sock, buffer->str_comm_buf, strlen(buffer->str_comm_buf), 3000) != strlen(buffer->str_comm_buf)) 
	{
		printf("sock_writen http body fail\r\n");
		goto get_translate_result_error;
	}
	
	/* Read HTTP response */
	memset(buffer->str_comm_buf, 0, sizeof(buffer->str_comm_buf));
	sock_readn_with_timeout(sock, buffer->str_comm_buf, sizeof(buffer->str_comm_buf) - 1, 8000);
	sock_close(sock);
	sock = INVALID_SOCK;

	if (http_get_error_code(buffer->str_comm_buf) == 200)
	{	
		char* pBody = http_get_body(buffer->str_comm_buf);
		if (NULL != pBody)
		{
			pJson = cJSON_Parse(pBody);
			if (NULL != pJson) 
			{
				cJSON *pJson_status = cJSON_GetObjectItem(pJson, "statusCode");
				if (NULL == pJson_status || pJson_status->valuestring == NULL)
				{
					ESP_LOGE(TRANSLATE_TAG, "get_translate_result,statusCode not found");
					goto get_translate_result_error;
				}

				if (strncasecmp(pJson_status->valuestring, "OK", 2) != 0)
				{
					ESP_LOGE(TRANSLATE_TAG, "get_translate_result,statusCode:%s", pJson_status->valuestring);
					goto get_translate_result_error;
				}
				ESP_LOGI(TRANSLATE_TAG, "get_translate_result,statusCode:%s", pJson_status->valuestring);
				
				cJSON *cjson_translate_result = cJSON_GetObjectItem(pJson, "content");
				{
					if (NULL == cjson_translate_result)
					{
						ESP_LOGE(TRANSLATE_TAG, "get_translate_result,content not found");
						goto get_translate_result_error;
					}
					cJSON *cjson_translate_result_text = cJSON_GetObjectItem(cjson_translate_result, "translateText");
					if (NULL == cjson_translate_result_text || cjson_translate_result_text->valuestring == NULL)
					{
						ESP_LOGE(TRANSLATE_TAG, "get_translate_result,translateText not found");
						goto get_translate_result_error;
					}
					snprintf(translate_result_text, TRANSLATE_TEXT_SIZE, "%s", cjson_translate_result_text->valuestring);
					ESP_LOGI(TRANSLATE_TAG, "get_translate_result,translateText:[%s]", cjson_translate_result_text->valuestring);
				}
			}
			else
			{
				ESP_LOGE(TRANSLATE_TAG, "get_translate_result,invalid json[%s]", buffer->str_comm_buf);
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
		ESP_LOGE(TRANSLATE_TAG, "get_translate_result,http reply error[%s]", buffer->str_comm_buf);
		goto get_translate_result_error;
	}

	if (buffer != NULL)
	{
		esp32_free(buffer);
		buffer = NULL;
	}
	
	return TRANSLATE_MANAGE_RET_GET_RESULT_OK;
	
get_translate_result_error:

	if (buffer != NULL)
	{
		esp32_free(buffer);
		buffer = NULL;
	}
	
	if (sock != INVALID_SOCK)
	{
		sock_close(sock);
	}

	if (NULL != pJson)
	{
		cJSON_Delete(pJson);
		pJson = NULL;
	}
	
	return TRANSLATE_MANAGE_RET_GET_RESULT_FAIL;
}

void translate_process(char *text)
{
	int ret = 0;
	char translate_result_text[TRANSLATE_TEXT_SIZE] = {0};
	TRANSLATE_MANAGE_RET_T translate_ret = TRANSLATE_MANAGE_RET_INIT;
	if (strlen(text) > 0)
	{
		translate_ret = translate_and_return_translate_result(text, translate_result_text);
		if (translate_ret == TRANSLATE_MANAGE_RET_GET_RESULT_OK)
		{
			play_tts(translate_result_text);
		}
	}
	else
	{
		player_mdware_play_tone(FLASH_MUSIC_NOT_HEAR_CLEARLY_PLEASE_REPEAT);
	}
	
	ESP_LOGI(TRANSLATE_TAG, "asr:[%s],result:[%s]", text, translate_result_text);
}

void set_translate_mode_flag(int translate_mode_flag)
{
	g_translate_mode_flag = translate_mode_flag;
}

int get_translate_mode_flag()
{
	int translate_mode_flag = g_translate_mode_flag;
	return translate_mode_flag;
}

void translate_tone()
{
	static TRANSLATE_TONE_NUM_T tone_num = TRANSLATE_TONE_NUM_ONE;
	
	switch (tone_num)
	{
		case TRANSLATE_TONE_NUM_ONE:
		{
			player_mdware_play_tone(FLASH_MUSIC_EN_AND_CH_TRANSLATION);
			tone_num++;
			break;
		}
		case TRANSLATE_TONE_NUM_TWO:
		{
			player_mdware_play_tone(FLASH_MUSIC_STUDY_ENGLISH_MODE);
			tone_num++;
			break;
		}
		default:
		{
			player_mdware_play_tone(FLASH_MUSIC_EN_AND_CH_TRANSLATION);
			tone_num = TRANSLATE_TONE_NUM_ONE;
			break;
		}
	}
	vTaskDelay(5000);
}

#if 0
static void task_translate(void *pv)
{
	int ret = 0;
	int i = 0;
	TRANSLATE_MSG_T *p_msg = NULL;
	TRANSLATE_QUEUE_HANDLE_T *translate_queue_handle = (TRANSLATE_QUEUE_HANDLE_T *)pv;
	
	while (1) 
	{
		if (xQueueReceive(translate_queue_handle->msg_queue, &p_msg, portMAX_DELAY) == pdPASS) 
		{			
			switch (p_msg->msg_type)
			{
				case TRANSLATE_MSG_TYPE_ASR_RET:
				{
					translate_process(p_msg);
					break;
				}
				case TRANSLATE_MSG_TYPE_QUIT:
				{
					break;
				}
				default:
					break;
			}

			if (p_msg->msg_type == TRANSLATE_MSG_TYPE_QUIT)
			{
				esp32_free(p_msg);
				p_msg = NULL;
				break;
			}

			if (p_msg != NULL)
			{
				esp32_free(p_msg);
				p_msg = NULL;
			}
		}
	}

	vTaskDelete(NULL);
}

void translate_asr_result_cb(ASR_RESULT_T *result)
{
	TRANSLATE_MSG_T *p_msg = NULL;
	
	if (g_translate_queue_handle == NULL || g_translate_queue_handle->msg_queue == NULL)
	{
		ESP_LOGE(TRANSLATE_TAG, "set_translate_record_data invalid parmas");
		return;
	}

	p_msg = (TRANSLATE_MSG_T *)esp32_malloc(sizeof(TRANSLATE_MSG_T));
	if (p_msg == NULL)
	{
		ESP_LOGE(TRANSLATE_TAG, "set_translate_record_data esp32_malloc failed");
		return;
	}
	memset(p_msg, 0, sizeof(TRANSLATE_MSG_T));
	p_msg->msg_type = TRANSLATE_MSG_TYPE_ASR_RET;
	p_msg->asr_result = *result;
	
	if (xQueueSend(g_translate_queue_handle->msg_queue, &p_msg, 0) != pdPASS)
	{
		esp32_free(p_msg);
		p_msg = NULL;
	}

	return;
}

static void set_translate_record_data(
	int id, char *data, size_t data_len, int record_ms, int is_max)
{
	ASR_RECORD_MSG_T *p_msg = NULL;

	if (g_translate_queue_handle == NULL || g_translate_queue_handle->msg_queue == NULL)
	{
		ESP_LOGE(TRANSLATE_TAG, "set_translate_record_data invalid parmas");
		return;
	}

	p_msg = (ASR_RECORD_MSG_T *)esp32_malloc(sizeof(ASR_RECORD_MSG_T));
	if (p_msg == NULL)
	{
		ESP_LOGE(TRANSLATE_TAG, "set_translate_record_data esp32_malloc failed");
		return;
	}

	memset(p_msg, 0, sizeof(ASR_RECORD_MSG_T));
	if (id == 1)
	{
		p_msg->msg_type = ASR_SERVICE_RECORD_START;
	}
	else if (id > 1)
	{
		p_msg->msg_type = ASR_SERVICE_RECORD_READ;
	}
	else
	{
		p_msg->msg_type = ASR_SERVICE_RECORD_STOP;
		player_mdware_play_tone(FLASH_MUSIC_MSG_SEND);
	}
	
	p_msg->record_data.record_id = id;
	p_msg->record_data.record_len = data_len;
	p_msg->record_data.record_ms = record_ms;
	p_msg->record_data.is_max_ms = is_max;
	p_msg->record_data.asr_engine = ASR_ENGINE_TYPE_AMRWB;
	p_msg->asr_call_back = translate_asr_result_cb;
	
	if (data_len > 0)
	{
		memcpy(p_msg->record_data.record_data, data, data_len);
	}

	if (asr_service_send_request(p_msg) == ESP_FAIL)
	{
		ESP_LOGE(TRANSLATE_TAG, "set_translate_record_data asr_service_send_request failed");
		esp32_free(p_msg);
		p_msg = NULL;
	}
}

void translate_service_create(void)
{
	g_translate_queue_handle = (TRANSLATE_QUEUE_HANDLE_T *)esp32_malloc(sizeof(TRANSLATE_QUEUE_HANDLE_T));
	if (g_translate_queue_handle == NULL)
	{
		return;
	}
	
	g_translate_queue_handle->msg_queue = xQueueCreate(2, sizeof(char *));
	
	if (xTaskCreate(task_translate,
					"task_translate",
					512*6,
					g_translate_queue_handle,
					4,
					NULL) != pdPASS) {

		ESP_LOGE(TRANSLATE_TAG, "ERROR creating task_translate task! Out of memory?");
	}
}

void translate_service_delete(void)
{
	
}

void translate_record_start(void)
{
	player_mdware_start_record(1, ASR_AMR_MAX_TIME_MS, set_translate_record_data);
}

void translate_record_stop(void)
{
	player_mdware_stop_record(set_translate_record_data);
}
#endif
