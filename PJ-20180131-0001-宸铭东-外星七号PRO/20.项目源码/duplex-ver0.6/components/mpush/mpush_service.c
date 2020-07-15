#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_system.h"

#include "MediaService.h"
#include "mpush_service.h"
#include "DeviceCommon.h"
#include "mpush_client_api.h"
#include "EspAudio.h"
#include "toneuri.h"
#include "baidu_api.h"
#include "player_middleware.h"
#include "auto_play_service.h"
#include "nlp_service.h"
#include "geling_edu.h"
#include "free_talk.h"

#define MPUSH_SERV_TAG "MPUSH_SERVICE"
#define MPUSH_SERVICE_TASK_PRIORITY					13
#define MPUSH_SERVICE_TASK_STACK_SIZE            	5*1024
#define KEY_WORD_SIZE								96
#define RET_ERR										1
#define RET_OK										0

static MPUSH_CLIENT_HANDLER_t *g_mpush_handler = NULL;
static SemaphoreHandle_t g_lock_mpush_service;
static APP_LINK_LIST_T *g_link_list = NULL;

QueueHandle_t g_mpush_msg_queue = NULL;
static APP_ASK_HANDLE_T *g_app_ask_handle = NULL;

//extern void play_tts(MediaService *media, char *str_tts);
void app_ask_result_cb(NLP_RESULT_T *result);

static void DeviceEvtNotifiedToMpush(DeviceNotification *note)
{
    MPUSH_SERVICE_T *service = (MPUSH_SERVICE_T *) note->receiver;
    if (DEVICE_NOTIFY_TYPE_WIFI == note->type) {
        DeviceNotifyMsg msg = *((DeviceNotifyMsg *) note->data);
        switch (msg) {
        case DEVICE_NOTIFY_WIFI_GOT_IP:
			service->Based._blocking = 0;
            break;
		case DEVICE_NOTIFY_WIFI_SETTING_TIMEOUT:
		case DEVICE_NOTIFY_WIFI_SC_DISCONNECTED:
        case DEVICE_NOTIFY_WIFI_DISCONNECTED:
			service->Based._blocking = 1;
            break;
        default:
            break;
        }
    }
}

static void PlayerStatusUpdatedToMpush(ServiceEvent *event)
{

}

static void get_prev_music(AUTO_PLAY_MUSIC_SOURCE_T *src)
{	
	if (g_app_ask_handle == NULL)
	{
		return;
	}
	xSemaphoreTake(g_lock_mpush_service, portMAX_DELAY);

	NLP_RESULT_LINKS_T *p_links = &g_app_ask_handle->nlp_result.link_result;

	memset(src, 0, sizeof(AUTO_PLAY_MUSIC_SOURCE_T));
	if (p_links->link_size == 0)
	{
		xSemaphoreGive(g_lock_mpush_service);
		return;
	}

	if (g_app_ask_handle->cur_play_index > 0)
	{
		g_app_ask_handle->cur_play_index--;
	}

	ESP_LOGI(MPUSH_SERV_TAG, "get_prev_music [%d][%s][%s]", 
		g_app_ask_handle->cur_play_index+1,
		p_links->link[g_app_ask_handle->cur_play_index].link_name,
		p_links->link[g_app_ask_handle->cur_play_index].link_url);
	
	src->need_play_name = 1;
	snprintf(src->music_name, sizeof(src->music_name), "%s", p_links->link[g_app_ask_handle->cur_play_index].link_name);
	snprintf(src->music_url, sizeof(src->music_url), "%s", p_links->link[g_app_ask_handle->cur_play_index].link_url);
	xSemaphoreGive(g_lock_mpush_service);
}

static void get_next_music(AUTO_PLAY_MUSIC_SOURCE_T *src)
{
	if (g_app_ask_handle == NULL)
	{
		return;
	}
	
	xSemaphoreTake(g_lock_mpush_service, portMAX_DELAY);

	NLP_RESULT_LINKS_T *p_links = &g_app_ask_handle->nlp_result.link_result;

	memset(src, 0, sizeof(AUTO_PLAY_MUSIC_SOURCE_T));
	if (p_links->link_size == 0)
	{
		xSemaphoreGive(g_lock_mpush_service);
		return;
	}

	if (g_app_ask_handle->cur_play_index < p_links->link_size - 1)
	{
		g_app_ask_handle->cur_play_index++;
	}
	else if (g_app_ask_handle->cur_play_index == p_links->link_size - 1)
	{
		if (strlen(g_app_ask_handle->nlp_result.input_text) > 0 && p_links->link_size == NLP_SERVICE_LINK_MAX_NUM)
		{
			nlp_service_send_request(g_app_ask_handle->nlp_result.input_text, app_ask_result_cb);
		}
		xSemaphoreGive(g_lock_mpush_service);
		return;
	}

	ESP_LOGI(MPUSH_SERV_TAG, "get_next_music [%d][%s][%s]", 
		g_app_ask_handle->cur_play_index+1,
		p_links->link[g_app_ask_handle->cur_play_index].link_name,
		p_links->link[g_app_ask_handle->cur_play_index].link_url);
	
	src->need_play_name = 1;
	snprintf(src->music_name, sizeof(src->music_name), "%s", p_links->link[g_app_ask_handle->cur_play_index].link_name);
	snprintf(src->music_url, sizeof(src->music_url), "%s", p_links->link[g_app_ask_handle->cur_play_index].link_url);
	xSemaphoreGive(g_lock_mpush_service);
}

static void get_current_music(AUTO_PLAY_MUSIC_SOURCE_T *src)
{
	if (g_app_ask_handle == NULL)
	{
		return;
	}

	xSemaphoreTake(g_lock_mpush_service, portMAX_DELAY);
	
	NLP_RESULT_LINKS_T *p_links = &g_app_ask_handle->nlp_result.link_result;

	memset(src, 0, sizeof(AUTO_PLAY_MUSIC_SOURCE_T));
	if (p_links->link_size == 0)
	{
		xSemaphoreGive(g_lock_mpush_service);
		return;
	}

	ESP_LOGI(MPUSH_SERV_TAG, "get_current_music [%d][%s][%s]", 
		g_app_ask_handle->cur_play_index+1,
		p_links->link[g_app_ask_handle->cur_play_index].link_name,
		p_links->link[g_app_ask_handle->cur_play_index].link_url);
	
	src->need_play_name = 1;
	snprintf(src->music_name, sizeof(src->music_name), "%s", p_links->link[g_app_ask_handle->cur_play_index].link_name);
	snprintf(src->music_url, sizeof(src->music_url), "%s", p_links->link[g_app_ask_handle->cur_play_index].link_url);
	xSemaphoreGive(g_lock_mpush_service);
}

void app_ask_result_cb(NLP_RESULT_T *result)
{
	NLP_RESULT_LINKS_T *p_links = &result->link_result;
	AUTO_PLAY_CALL_BACK_T call_backs = {get_prev_music, get_next_music, get_current_music};

	if (g_app_ask_handle == NULL)
	{
		return;
	}
	xSemaphoreTake(g_lock_mpush_service, portMAX_DELAY);
	
	switch (result->type)
	{
		case NLP_RESULT_TYPE_NONE:
		{
			break;
		}
		case NLP_RESULT_TYPE_CHAT:
		{
			//play tts
			auto_play_pause();
			ESP_LOGE(MPUSH_SERV_TAG, "chat_result=[%s]\r\n", result->chat_result.text);
			play_tts(result->chat_result.text);
			break;
		}
		case NLP_RESULT_TYPE_LINK:
		{
			if (p_links->link_size > 0)
			{
				g_app_ask_handle->nlp_result = *result;
				g_app_ask_handle->cur_play_index = 0;
				set_auto_play_cb(&call_backs);
			}
			break;
		}
		default:
		{
			break;
		}
	}
	xSemaphoreGive(g_lock_mpush_service);
}

//提取接收到的技能关键词，并且进行播放操作
static void get_key_word_and_play(char *string)
{
	char *start = NULL;
	char *end = NULL;
	char key_word[KEY_WORD_SIZE] = {0};

	start = strstr(string, "<music>");
	if (start == NULL)
	{
		player_mdware_play_tone(FLASH_MUSIC_NETWORK_POOR);
		return;
	}
	start += strlen("<music>");

	end = strstr(string, "</music>");
	if (end == NULL)
	{
		player_mdware_play_tone(FLASH_MUSIC_NETWORK_POOR);
		return;
	}

	memcpy(key_word, start, end-start);
	//printf("src is %s\n", key_word);
	nlp_service_send_request(key_word, app_ask_result_cb);
}

static void get_prev_url(AUTO_PLAY_MUSIC_SOURCE_T *src)
{
	if (g_link_list == NULL)
	{
		return;
	}
	xSemaphoreTake(g_lock_mpush_service, portMAX_DELAY);
	
	memset(src, 0, sizeof(AUTO_PLAY_MUSIC_SOURCE_T));
	if (g_link_list->link_size == 0)
	{
		xSemaphoreGive(g_lock_mpush_service);
		return;
	}

	if (g_link_list->cur_play_index > 0)
	{
		g_link_list->cur_play_index--;
	}

	ESP_LOGI(MPUSH_SERV_TAG, "get_prev_music [%d][%s][%s]", 
		g_link_list->cur_play_index+1,
		g_link_list->list[g_link_list->cur_play_index].link_name,
		g_link_list->list[g_link_list->cur_play_index].link_url);
	
	src->need_play_name = 1;
	snprintf(src->music_name, sizeof(src->music_name), "%s", g_link_list->list[g_link_list->cur_play_index].link_name);
	snprintf(src->music_url, sizeof(src->music_url), "%s", g_link_list->list[g_link_list->cur_play_index].link_url);
	xSemaphoreGive(g_lock_mpush_service);
}

static void get_next_url(AUTO_PLAY_MUSIC_SOURCE_T *src)
{
	if (g_link_list == NULL)
	{
		return;
	}
	
	xSemaphoreTake(g_lock_mpush_service, portMAX_DELAY);

	memset(src, 0, sizeof(AUTO_PLAY_MUSIC_SOURCE_T));
	if (g_link_list->link_size == 0)
	{
		xSemaphoreGive(g_lock_mpush_service);
		return;
	}

	if (g_link_list->cur_play_index < g_link_list->link_size - 1)
	{
		g_link_list->cur_play_index++;
	}
	else if (g_link_list->cur_play_index == g_link_list->link_size - 1)
	{
		xSemaphoreGive(g_lock_mpush_service);
		return;
	}

	ESP_LOGI(MPUSH_SERV_TAG, "get_next_music [%d][%s][%s]", 
		g_link_list->cur_play_index+1,
		g_link_list->list[g_link_list->cur_play_index].link_name,
		g_link_list->list[g_link_list->cur_play_index].link_url);
	
	src->need_play_name = 1;
	snprintf(src->music_name, sizeof(src->music_name), "%s", g_link_list->list[g_link_list->cur_play_index].link_name);
	snprintf(src->music_url, sizeof(src->music_url), "%s", g_link_list->list[g_link_list->cur_play_index].link_url);
	xSemaphoreGive(g_lock_mpush_service);
}

static void get_current_url(AUTO_PLAY_MUSIC_SOURCE_T *src)
{
	if (g_link_list == NULL)
	{
		return;
	}

	xSemaphoreTake(g_lock_mpush_service, portMAX_DELAY);
	
	memset(src, 0, sizeof(AUTO_PLAY_MUSIC_SOURCE_T));
	if (g_link_list->link_size == 0)
	{
		xSemaphoreGive(g_lock_mpush_service);
		return;
	}

	ESP_LOGI(MPUSH_SERV_TAG, "get_current_music [%d][%s][%s]", 
		g_link_list->cur_play_index+1,
		g_link_list->list[g_link_list->cur_play_index].link_name,
		g_link_list->list[g_link_list->cur_play_index].link_url);
	
	src->need_play_name = 1;
	snprintf(src->music_name, sizeof(src->music_name), "%s", g_link_list->list[g_link_list->cur_play_index].link_name);
	snprintf(src->music_url, sizeof(src->music_url), "%s", g_link_list->list[g_link_list->cur_play_index].link_url);
	xSemaphoreGive(g_lock_mpush_service);
}

static int get_url_and_play(cJSON *pJson)
{	
	cJSON *pJson_command = cJSON_GetObjectItem(pJson, "command");
	if (NULL == pJson_command || pJson_command->valuestring == NULL)
	{
		ESP_LOGE(MPUSH_SERV_TAG, "_pJson_command not found");
		return RET_ERR;
	}
	//printf ("*****_pJson_command is %s\n", pJson_command->valuestring);
	if (strcmp(pJson_command->valuestring, "playList") == 0)
	{
		cJSON *pJson_date = cJSON_GetObjectItem(pJson, "data");
		if (NULL == pJson_date)
		{
			ESP_LOGE(MPUSH_SERV_TAG, "_pJson_data not found");
			return RET_ERR;
		}
		
		int list_size = cJSON_GetArraySize(pJson_date);
		if (list_size <= 0)
		{
			ESP_LOGE(MPUSH_SERV_TAG, "list_size is 0, no offline msg");
			return RET_ERR;
		}
		else
		{
			memset(g_link_list, 0, sizeof(APP_LINK_LIST_T));
			//printf("*************list_size = [ %d ]\n", list_size);
			int i = 0;
			int list_num = 0;
			for (i = 0; i < list_size; i++)
			{
				cJSON *pJson_item = cJSON_GetArrayItem(pJson_date, i);
				cJSON *pJson_name = cJSON_GetObjectItem(pJson_item, "name");
				cJSON *pJson_url = cJSON_GetObjectItem(pJson_item, "url");
				if (NULL == pJson_name
					|| NULL == pJson_name->valuestring
					|| strlen(pJson_name->valuestring) <= 0
					|| NULL == pJson_url
					|| NULL == pJson_url->valuestring
					|| strlen(pJson_url->valuestring) <= 0)
				{
					continue;
				}
				snprintf(g_link_list->list[list_num].link_name, sizeof(g_link_list->list[list_num].link_name), "正在给您播放%s", pJson_name->valuestring);
				snprintf(g_link_list->list[list_num].link_url, sizeof(g_link_list->list[list_num].link_url), pJson_url->valuestring);
				list_num++;
				g_link_list->link_size = list_num;
			}
		}
	}
	else
	{
		ESP_LOGE(MPUSH_SERV_TAG, "_pJson_command faild !");
		return RET_ERR;
	}
	return RET_OK;
}

void mpush_service_process_msg(void *pv)
{
	MPUSH_SERVICE_T *service  = (MPUSH_SERVICE_T *) pv;
	MPUSH_CLIENT_MSG_INFO_T *msg_info = NULL;
	PlayerStatus play_status = {0};
	char *last_music_src = esp32_malloc(2048);
	memset(last_music_src, 0, 2048);
	BAIDU_ACOUNT_T *baidu_acount = esp32_malloc(sizeof(BAIDU_ACOUNT_T));
	ESP_ERROR_CHECK(!baidu_acount);
	AUTO_PLAY_CALL_BACK_T call_backs = {get_prev_url, get_next_url, get_current_url};

	//delay 10 secs start msg play
	vTaskDelay(1000*10);
	
    while (1) 
	{
        BaseType_t xStatus = xQueueReceive(g_mpush_msg_queue, &msg_info, portMAX_DELAY);
		if (xStatus == pdPASS && msg_info != NULL) 
		{				
			ESP_LOGE(MPUSH_SERV_TAG, "msg_type[%d],msg_content=[%s]", msg_info->msg_type, msg_info->msg_content);
			while(1)
			{
				memset(&play_status, 0, sizeof(play_status));
				EspAudioStatusGet(&play_status);

				switch (play_status.mode)
				{
					case PLAYER_WORKING_MODE_NONE:
					{
						ESP_LOGE(MPUSH_SERV_TAG, "PLAYER_WORKING_MODE_NONE");
						break;
					}
					case PLAYER_WORKING_MODE_MUSIC:
					{
						ESP_LOGE(MPUSH_SERV_TAG, "PLAYER_WORKING_MODE_MUSIC");
						break;
					}
					case PLAYER_WORKING_MODE_TONE:
					{
						ESP_LOGE(MPUSH_SERV_TAG, "PLAYER_WORKING_MODE_TONE");
						break;
					}
					case PLAYER_WORKING_MODE_RAW:
					{
						ESP_LOGE(MPUSH_SERV_TAG, "PLAYER_WORKING_MODE_RAW");
						break;
					}
					default:
						break;
				}
				
				if (play_status.status == AUDIO_STATUS_PLAYING 
					&& play_status.mode == PLAYER_WORKING_MODE_TONE)
				{
					vTaskDelay(2*1000);
					continue;
				}
				else
				{
					break;
				}
			}
			/*
			while (1)
			{
				if (keyboard_lock())
				{
					vTaskDelay(1000);
					continue;
				}
				else
				{
					break;
				}
			}
			*/
			//stop music first
			//EspAudioPause();
			
			if (msg_info->msg_type == MPUSH_SEND_MSG_TYPE_TEXT)
			{				
				free_talk_stop();
				if (strncmp(&msg_info->msg_content, "<music>", strlen("<music>")) == 0)
				{
					get_key_word_and_play(&msg_info->msg_content);					
				}
				else
				{
					//play key press tone
					player_mdware_play_tone(FLASH_MUSIC_MESSAGE_ALERT);
					vTaskDelay(1500);
					
					memset(last_music_src, 0, 2048);
					get_ai_acount(AI_ACOUNT_BAIDU, baidu_acount);
					int ret = baidu_tts(baidu_acount, last_music_src, 2048, msg_info->msg_content);
					if (BAIDU_TTS_SUCCESS != ret)
					{
						ESP_LOGE(MPUSH_SERV_TAG, "mpush_service_process_msg baidu_tts first fail");
						int ret = baidu_tts(baidu_acount, last_music_src, 2048, msg_info->msg_content);
						if (BAIDU_TTS_SUCCESS != ret)
						{
							ESP_LOGE(MPUSH_SERV_TAG, "mpush_service_process_msg baidu_tts second fail");
						}
					}

					if (strlen(last_music_src) > 0)
					{
						EspAudioTonePlay(last_music_src, TERMINATION_TYPE_NOW);
						vTaskDelay(3000);
					}
				}
				
			}
			else if (msg_info->msg_type == MPUSH_SEND_MSG_TYPE_LINK)
			{
				free_talk_stop();
				//play message alert tone
				player_mdware_play_tone(FLASH_MUSIC_MESSAGE_ALERT);
				vTaskDelay(1500);
				EspAudioTonePlay(msg_info->msg_content, TERMINATION_TYPE_NOW);
				vTaskDelay(3000);
			}
			else if (msg_info->msg_type == MPUSH_SEND_MSG_TYPE_CMD)
			{
				free_talk_stop();
				cJSON * pJson = cJSON_Parse(msg_info->msg_content);
				if (pJson != NULL) 
				{
					if (get_url_and_play(pJson) != RET_ERR)
					{
						if (strstr(&g_link_list->list[0].link_url, ".m3u8") != NULL)
						{
							geling_edu_service_send_request(&g_link_list->list[0].link_url);
						}
						else
						{
							set_auto_play_cb(&call_backs);
						}
					}
					else
					{
						player_mdware_play_tone(FLASH_MUSIC_DYY_ERROR_PLEASE_TRY_AGAIN_LATER);
					}
					cJSON_Delete(pJson);
				}
				else
				{
					player_mdware_play_tone(FLASH_MUSIC_DYY_ERROR_PLEASE_TRY_AGAIN_LATER);
					ESP_LOGE(MPUSH_SERV_TAG, "MPUSH_SEND_MSG_TYPE_CMD invalid json data");
				}
			}
			else
			{
				ESP_LOGE(MPUSH_SERV_TAG, "not support msg type");
			}
			
			esp32_free(msg_info);
			msg_info = NULL;
			//keyboard_unlock();
		}
    }
	
    vTaskDelete(NULL);
}

static void mpush_service_active(MPUSH_SERVICE_T *service)
{
	char device_sn[32] = {0};
	get_flash_cfg(FLASH_CFG_DEVICE_ID, device_sn);

	service->_mpush_handler = mpush_client_create(ESP32_FW_VERSION, device_sn);
	
	if (service->_mpush_handler == NULL)
	{
		ESP_LOGE(MPUSH_SERV_TAG, "mpush_client_create failed");
		return;
	}
	g_mpush_handler = service->_mpush_handler;
	
    ESP_LOGI(MPUSH_SERV_TAG, "mpush_service_active");
    if (xTaskCreate(mpush_client_process,
                    "mpush_client_process",
                    4*1024,
                    service,
                    MPUSH_SERVICE_TASK_PRIORITY,
                    NULL) != pdPASS) 
    {
        ESP_LOGE(MPUSH_SERV_TAG, "create mpush_client_process failed");
    }

	if (xTaskCreate(mpush_client_receive,
                    "mpush_client_receive",
                    2*1024,
                    service,
                    MPUSH_SERVICE_TASK_PRIORITY,
                    NULL) != pdPASS) 
    {
        ESP_LOGE(MPUSH_SERV_TAG, "create mpush_client_receive failed");
    }

	if (xTaskCreate(mpush_service_process_msg,
		    "mpush_service_process_msg",
		    3*1024,
		    service,
		    MPUSH_SERVICE_TASK_PRIORITY,
		    NULL) != pdPASS) 
    {
        ESP_LOGE(MPUSH_SERV_TAG, "create mpush_service_process_msg failed");
    }
	
}

static void mpush_service_deactive(MPUSH_SERVICE_T *service)
{
    ESP_LOGI(MPUSH_SERV_TAG, "mpush_service_deactive");
    vQueueDelete(service->_OtaQueue);
    service->_OtaQueue = NULL;
}

MPUSH_SERVICE_T *mpush_service_create()
{
	//��ʼ��mpush_service��ز���
	g_app_ask_handle = esp32_malloc(sizeof(APP_ASK_HANDLE_T));
	if (g_app_ask_handle == NULL)
	{
		ESP_LOGE(MPUSH_SERV_TAG, "esp32_malloc g_app_ask_handle fail");
		return NULL;
	}
	memset(g_app_ask_handle, 0, sizeof(APP_ASK_HANDLE_T));
	
	g_link_list = esp32_malloc(sizeof(APP_LINK_LIST_T));
	if (g_link_list == NULL)
	{
		ESP_LOGE(MPUSH_SERV_TAG, "esp32_malloc g_link_list fail");
		return NULL;
	}
	memset(g_link_list, 0, sizeof(APP_LINK_LIST_T));
	
	g_lock_mpush_service = xSemaphoreCreateMutex();
	
    MPUSH_SERVICE_T *mpush = (MPUSH_SERVICE_T *) calloc(1, sizeof(MPUSH_SERVICE_T));
    ESP_ERROR_CHECK(!mpush);
    mpush->_OtaQueue = xQueueCreate(1, sizeof(uint32_t));
    configASSERT(mpush->_OtaQueue);
	g_mpush_msg_queue = xQueueCreate(20, sizeof(char*));
	configASSERT(g_mpush_msg_queue);
    mpush->Based.deviceEvtNotified = DeviceEvtNotifiedToMpush;
    mpush->Based.playerStatusUpdated = PlayerStatusUpdatedToMpush;
    mpush->Based.serviceActive = mpush_service_active;
    mpush->Based.serviceDeactive = mpush_service_deactive;
	mpush->Based._blocking = 1;

    return mpush;
}

int mpush_service_send_text(
	const char * _string, 
	const char * _nlp_text,
	const char *_to_user)
{
	if (g_mpush_handler == NULL)
	{
		return ESP_FAIL;
	}
	
	return mpush_client_send_text(g_mpush_handler, _string, _nlp_text, _to_user);
}

int mpush_service_send_file(
	const char * _data, 
	const size_t data_len, 
	const char * _nlp_text,
	const char *_file_type,
	const char *_to_user)
{
	if (g_mpush_handler == NULL)
	{
		return ESP_FAIL;
	}
	
	return mpush_client_send_file(g_mpush_handler, _data, data_len, _nlp_text, _file_type, _to_user);
}

