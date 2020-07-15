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

//#define DEBUG_PRINT

static TRANSLATE_TALK_HANDLE_T *g_translate_talk_handle = NULL;

static int get_translate_result(TRANSLATE_TALK_HANDLE_T *translate_talk_handle, const char *_nlp_text)
{
//	char 	str_type[8] = {0};
	char	domain[64]	= {0};
	char	port[6] 	= {0};
	char	params[128] = {0};
	char	str_nonce[64] = {0};
	char	str_timestamp[64] = {0};
	char	str_private_key[64] = {0};
	char	str_device_id[32] = {0};
	char	str_buf[512]= {0};
	sock_t	sock		= INVALID_SOCK;
	cJSON	*pJson = NULL;
	
	if (translate_talk_handle == NULL)
	{
		return;
	}
	
	if (sock_get_server_info(TRANSLATE_SEND_MSG_URL, &domain, &port, &params) != 0)
	{
		ESP_LOGE(TRANSLATE_TAG, "get_translate_result,sock_get_server_info fail\r\n");
		goto get_translate_result_error;
	}

	sock = sock_connect(domain, port);
	if (sock == INVALID_SOCK)
	{
		ESP_LOGE(TRANSLATE_TAG, "get_translate_result,sock_connect fail,%s,%s\r\n", domain, port);
		goto get_translate_result_error;
	}
	
	sock_set_nonblocking(sock);
	crypto_generate_request_id(str_nonce, sizeof(str_nonce));
	get_flash_cfg(FLASH_CFG_DEVICE_ID, str_device_id);
	snprintf(translate_talk_handle->str_comm_buf, sizeof(TRANSLATE_TALK_HANDLE_T),
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
		_nlp_text,
		"",
		"",
		str_device_id,
		str_nonce,
		DEEP_BRAIN_ROBOT_ID,
		str_device_id);
	crypto_generate_nonce((uint8_t *)str_nonce, sizeof(str_nonce));
	crypto_time_stamp((unsigned char*)str_timestamp, sizeof(str_timestamp));
	crypto_generate_private_key((uint8_t *)str_private_key, sizeof(str_private_key), str_nonce, str_timestamp, DEEP_BRAIN_ROBOT_ID);
	snprintf(str_buf, sizeof(str_buf), 
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
		params, domain, port, strlen(translate_talk_handle->str_comm_buf), str_nonce, str_timestamp, str_private_key, DEEP_BRAIN_ROBOT_ID);

	//ESP_LOGE(PRINT_TAG, "%s", str_buf);
	//ESP_LOGE(PRINT_TAG, "%s", _pHandler->str_comm_buf);
	if (sock_writen_with_timeout(sock, str_buf, strlen(str_buf), 1000) != strlen(str_buf)) 
	{
		printf("sock_writen http header fail\r\n");
		goto get_translate_result_error;
	}

	if (sock_writen_with_timeout(sock, translate_talk_handle->str_comm_buf, strlen(translate_talk_handle->str_comm_buf), 3000) != strlen(translate_talk_handle->str_comm_buf)) 
	{
		printf("sock_writen http body fail\r\n");
		goto get_translate_result_error;
	}
	
	/* Read HTTP response */
	memset(translate_talk_handle->str_comm_buf, 0, sizeof(translate_talk_handle->str_comm_buf));
	sock_readn_with_timeout(sock, translate_talk_handle->str_comm_buf, sizeof(translate_talk_handle->str_comm_buf) - 1, 8000);
	sock_close(sock);
	sock = INVALID_SOCK;

	if (http_get_error_code(translate_talk_handle->str_comm_buf) == 200)
	{	
		char* pBody = http_get_body(translate_talk_handle->str_comm_buf);
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
					snprintf(translate_talk_handle->translate_result_text, TRANSLATE_TEXT_SIZE, "%s", cjson_translate_result_text->valuestring);
					ESP_LOGI(TRANSLATE_TAG, "get_translate_result,translateText:[%s]", cjson_translate_result_text->valuestring);
				}
			}
			else
			{
				ESP_LOGE(TRANSLATE_TAG, "get_translate_result,invalid json[%s]", translate_talk_handle->str_comm_buf);
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
		ESP_LOGE(TRANSLATE_TAG, "get_translate_result,http reply error[%s]", translate_talk_handle->str_comm_buf);
		goto get_translate_result_error;
	}
	
	return TRANSLATE_MANAGE_RET_GET_RESULT_OK;
	
get_translate_result_error:
	
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


static void translate_talk_amr_encode(
	TRANSLATE_TALK_RECORD_DATA_T *p_record_data, TRANSLATE_TALK_HANDLE_T *translate_talk_handle) 
{
	int ret = 0;
	int i  = 0;
	char amr_data_frame[AMR_20MS_AUDIO_SIZE] = {0};
	RESAMPLE resample = {0};
	
	if (p_record_data == NULL || translate_talk_handle == NULL)
	{
		return;
	}

	TRANSLATE_TALK_STATUS_T *p_status = &translate_talk_handle->status;
	TRANSLATE_TALK_ENCODE_BUFF_T *p_pcm_encode = &translate_talk_handle->pcm_encode;
	
	//降采样
	ret = unitSampleProc((short *)p_record_data->data, (short *)p_pcm_encode->data, 
		AUDIO_PCM_RATE, 8000, p_record_data->record_len, 1, &resample);
	if (ret > 0)
	{
		p_pcm_encode->data_len = ret;
		//ESP_LOGE(LOG_TAG, "task_translate unitSampleProc success[%d]", ret);
	}
	else
	{
		ESP_LOGE(TRANSLATE_TAG, "translate_talk_amr_encode unitSampleProc failed[%d]", ret);
		return;
	}

	//转AMR
	for (i=0; (i*320) < p_pcm_encode->data_len; i++)
	{
		if ((p_status->record_len + sizeof(amr_data_frame)) > sizeof(p_status->record_data))
		{
			break;
		}
		
		ret = Encoder_Interface_Encode(translate_talk_handle->amr_encode_handle, MR122, 
			(short*)(p_pcm_encode->data + i*320), &amr_data_frame, 0);
		if (ret == sizeof(amr_data_frame))
		{
			memcpy(p_status->record_data + p_status->record_len, amr_data_frame, sizeof(amr_data_frame));
			p_status->record_len += sizeof(amr_data_frame);
			p_status->record_ms += 20;
		}
		else
		{
			ESP_LOGE(TRANSLATE_TAG, "translate_talk_amr_encode Encoder_Interface_Encode failed[%d]", ret);
		}
	}
}

static void task_translate_talk(void *pv)
{
	int ret = 0;
	int i = 0;
    DEVICE_KEY_EVENT_T key_event = DEVICE_KEY_START;
    TRANSLATE_TALK_HANDLE_T *translate_talk_handle = (TRANSLATE_TALK_HANDLE_T *)pv;
	MediaService *service = translate_talk_handle->service;
	TRANSLATE_TALK_MSG_T msg = {0};
	TRANSLATE_MANAGE_RET_T translate_ret = TRANSLATE_MANAGE_RET_INIT;
//	char translate_result_text[TRANSLATE_TEXT_SIZE] = {0};
	
    while (1) 
	{
		if (translate_talk_handle->amr_encode_handle == NULL)
		{
			translate_talk_handle->amr_encode_handle = Encoder_Interface_init(0);
			if (translate_talk_handle->amr_encode_handle == NULL)
			{
				ESP_LOGE(TRANSLATE_TAG, "Encoder_Interface_init failed");
				vTaskDelay(1000);
				continue;
			}
		}
		
		if (xQueueReceive(translate_talk_handle->msg_queue, &msg, portMAX_DELAY) == pdPASS) 
		{			
#ifdef DEBUG_PRINT
			if (msg.p_record_data != NULL)
			{
				ESP_LOGI(TRANSLATE_TAG, "id=%d, len=%d, time=%dms", 
				msg.p_record_data->record_id, msg.p_record_data->record_len, msg.p_record_data->record_ms);
			}
#endif			
			switch (msg.msg_type)
			{
				case TRANSLATE_TALK_MSG_RECORD_START:
				{
					snprintf(translate_talk_handle->status.record_data, sizeof(translate_talk_handle->status.record_data), "%s", AMR_HEADER);
					translate_talk_handle->status.record_len = strlen(AMR_HEADER);
				}
				case TRANSLATE_TALK_MSG_RECORD_READ:
				{
					translate_talk_amr_encode(msg.p_record_data, translate_talk_handle);
					break;
				}
				case TRANSLATE_TALK_MSG_RECORD_STOP:
				{			
					//语音小于1秒
					if (msg.p_record_data->record_ms < 1000)
					{
						//长按翻译键 听到嘟声后 说出你想说的
						player_mdware_play_tone(FLASH_MUSIC_TRANSLATE_SHORT_AUDIO);
						break;
					}

					player_mdware_play_tone(FLASH_MUSIC_MSG_SEND);
				
					//AMR语音识别
					get_ai_acount(AI_ACOUNT_BAIDU, &translate_talk_handle->baidu_acount);
					memset(&translate_talk_handle->asr_result, 0, sizeof(translate_talk_handle->asr_result));
#if	DEBUG_RECORD_SDCARD
					static int count = 1;
					FILE *p_record_file = NULL;
					char str_record_path[32] = {0};
					snprintf(str_record_path, sizeof(str_record_path), "/sdcard/1-%d.amr", count++);
					p_record_file = fopen(str_record_path, "w+");
					if (p_record_file != NULL)
					{
						fwrite(translate_talk_handle->status.record_data, translate_talk_handle->status.record_len, 1, p_record_file);
						fclose(p_record_file);
					}
#endif
					int start_time = get_time_of_day();
					if (baidu_asr(&translate_talk_handle->baidu_acount, translate_talk_handle->asr_result.asr_result, sizeof(translate_talk_handle->asr_result.asr_result), 
						(unsigned char*)translate_talk_handle->status.record_data, translate_talk_handle->status.record_len) == BAIDU_ASR_FAIL)
					{
						ESP_LOGE(TRANSLATE_TAG, "translate_chat_talk baidu_asr first fail");
						if (baidu_asr(&translate_talk_handle->baidu_acount, translate_talk_handle->asr_result.asr_result, sizeof(translate_talk_handle->asr_result.asr_result), 
							(unsigned char*)translate_talk_handle->status.record_data, translate_talk_handle->status.record_len) == BAIDU_ASR_FAIL)
						{
							ESP_LOGE(TRANSLATE_TAG, "task_translate_talk baidu_asr second fail");
						}
					}

					int asr_cost = get_time_of_day() - start_time;

					if (strlen(translate_talk_handle->asr_result.asr_result) > 0)
					{
						translate_ret = get_translate_result(translate_talk_handle, &(translate_talk_handle->asr_result.asr_result));
						if (translate_ret == TRANSLATE_MANAGE_RET_GET_RESULT_OK)
						{
							play_tts(translate_talk_handle->translate_result_text);
						}
					}
					else
					{
						player_mdware_play_tone(FLASH_MUSIC_2_DYY_NOT_HEAR_CLEARLY_PLEASE_REPEAT);
					}
					
					ESP_LOGI(TRANSLATE_TAG, "baidu asr:record_len[%d],cost[%dms],result[%s]", translate_talk_handle->status.record_len, asr_cost, translate_talk_handle->asr_result.asr_result);
					break;
				}
				default:
					break;
			}

			if (msg.msg_type == TRANSLATE_TALK_MSG_QUIT)
			{
				break;
			}

			if (msg.p_record_data != NULL)
			{
				esp32_free(msg.p_record_data);
				msg.p_record_data = NULL;
			}
		}
    }

	Encoder_Interface_exit(translate_talk_handle->amr_encode_handle);
    vTaskDelete(NULL);
}

static void set_translate_talk_record_data(
	int id, char *data, size_t data_len, int record_ms, int is_max)
{
	TRANSLATE_TALK_MSG_T msg = {0};

	if (g_translate_talk_handle == NULL || g_translate_talk_handle->msg_queue == NULL)
	{
		ESP_LOGE(TRANSLATE_TAG, "set_translate_talk_record_data invalid parmas");
		return;
	}

	msg.p_record_data = (TRANSLATE_TALK_RECORD_DATA_T *)esp32_malloc(sizeof(TRANSLATE_TALK_RECORD_DATA_T));
	if (msg.p_record_data == NULL)
	{
		ESP_LOGE(TRANSLATE_TAG, "set_translate_talk_record_data esp32_malloc failed");
		return;
	}

	memset(msg.p_record_data, 0, sizeof(TRANSLATE_TALK_RECORD_DATA_T));
	if (id == 1)
	{
		msg.msg_type = TRANSLATE_TALK_MSG_RECORD_START;
	}
	else if (id > 1)
	{
		msg.msg_type = TRANSLATE_TALK_MSG_RECORD_READ;
	}
	else
	{
		msg.msg_type = TRANSLATE_TALK_MSG_RECORD_STOP;	
	}
	
	msg.p_record_data->record_id = id;
	msg.p_record_data->record_len = data_len;
	msg.p_record_data->record_ms = record_ms;
	msg.p_record_data->is_max_ms = is_max;
	
	if (data_len > 0)
	{
		memcpy(msg.p_record_data->data, data, data_len);
	}

	if (xQueueSend(g_translate_talk_handle->msg_queue, &msg, 0) != pdPASS)
	{
		ESP_LOGE(TRANSLATE_TAG, "xQueueSend g_translate_talk_handle->msg_queue failed");
		esp32_free(msg.p_record_data);
		msg.p_record_data = NULL;
	}
}

void translate_talk_create(MediaService *self)
{
	g_translate_talk_handle = (TRANSLATE_TALK_HANDLE_T *)esp32_malloc(sizeof(TRANSLATE_TALK_HANDLE_T));
	if (g_translate_talk_handle == NULL)
	{
		return;
	}
	memset(g_translate_talk_handle, 0, sizeof(TRANSLATE_TALK_HANDLE_T));

	g_translate_talk_handle->service = self;
	g_translate_talk_handle->msg_queue = xQueueCreate(20, sizeof(TRANSLATE_TALK_MSG_T));
	
	if (xTaskCreate(task_translate_talk,
					"task_translate_talk",
					1024*8,
					g_translate_talk_handle,
					4,
					NULL) != pdPASS) {

		ESP_LOGE(TRANSLATE_TAG, "ERROR creating task_translate_talk task! Out of memory?");
	}
}

void translate_talk_delete(void)
{
	TRANSLATE_TALK_MSG_T msg = {0};
	msg.msg_type = TRANSLATE_TALK_MSG_QUIT;
	
	if (g_translate_talk_handle != NULL && g_translate_talk_handle->msg_queue != NULL)
	{
		xQueueSend(g_translate_talk_handle->msg_queue, &msg, 0);
	}
}

void translate_talk_start(void)
{
	player_mdware_start_record(1, TRANSLATE_TALK_MAX_TIME_MS, set_translate_talk_record_data);
}

void translate_talk_stop(void)
{
	player_mdware_stop_record();
}

