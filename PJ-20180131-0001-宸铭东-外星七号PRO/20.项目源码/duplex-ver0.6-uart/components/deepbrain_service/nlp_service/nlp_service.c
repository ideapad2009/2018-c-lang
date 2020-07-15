#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "esp_system.h"
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"
#include "soc/sens_reg.h"

#include "debug_log.h"
#include "driver/touch_pad.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "keyboard_manage.h"

#include "cJSON.h"
#include "deepbrain_service.h"
#include "touchpad.h"
#include "DeviceCommon.h"
#include "MediaControl.h"
#include "MediaHal.h"
#include "toneuri.h"
#include "baidu_api.h"
#include "http_api.h"
#include "deepbrain_api.h"
#include "device_api.h"
#include "socket.h"
#include "lwip/netdb.h"
#include "EspAudio.h"
#include "userconfig.h"
#include "Recorder.h"
#include "nlp_service.h"
#include "memo_service.h"
#include "user_experience_test_data_manage.h"
#include "volume_manage.h"

#define LOG_TAG "nlp service" 
#define COMMAND_NAME_VOLUME_CTRL "音量调节"
#define COMMAND_NAME_MEMO_CTRL   "备忘"


typedef enum
{
	NLP_SERVICE_MSG_TYPE_REQUEST,
	NLP_SERVICE_MSG_TYPE_QUIT,	
}NLP_SERVICE_MSG_TYPE_T;

typedef struct
{
	NLP_SERVICE_MSG_TYPE_T msg_type;
	char text[512];
	nlp_service_result_cb cb;
}NLP_SERVICE_REQUEST_MSG_T;

typedef struct
{
	xQueueHandle msg_queue;
	NLP_SERVICE_REQUEST_MSG_T *p_msg;
	NLP_RESULT_T nlp_result;
}NLP_SERVICE_STATUS_T;

static NLP_SERVICE_STATUS_T *g_nlp_service_status = NULL;

extern MEMO_SERVICE_HANDLE_T *memo_struct_handle;


static int music_source_decode(cJSON *json_data, NLP_SERVICE_STATUS_T *status)
{
	int i = 0;
	int m = 0;
	int attrs_size = 0;
	int array_size = 0;
	int has_music = 0;
	char artist_name[32] = {0};
	NLP_RESULT_T *nlp_result = &status->nlp_result;
	NLP_RESULT_LINKS_T *p_links = &nlp_result->link_result;

	cJSON *pJson_music = cJSON_GetObjectItem(json_data, "linkResources");
	if (NULL != pJson_music)
	{
		array_size = cJSON_GetArraySize(pJson_music);
	}
	
	if (array_size > 0)
	{
		for (i = 0; i < array_size && i < NLP_SERVICE_LINK_MAX_NUM; i++)
		{
			cJSON *pJson_music_info = cJSON_GetArrayItem(pJson_music, i);
			cJSON *pJson_music_url = cJSON_GetObjectItem(pJson_music_info, "accessURL");
			cJSON *pJson_music_name = cJSON_GetObjectItem(pJson_music_info, "resName");
			if (NULL == pJson_music_url 
				&& NULL == pJson_music_url->valuestring)
			{
				continue;
			}

			nlp_result->type = NLP_RESULT_TYPE_LINK;
			snprintf(p_links->link[p_links->link_size].link_url, sizeof(p_links->link[p_links->link_size].link_url),
				"%s", pJson_music_url->valuestring);
			cJSON *pJson_attrs = cJSON_GetObjectItem(pJson_music_info, "resAttrs");
			if (pJson_attrs != NULL)
			{
				attrs_size =  cJSON_GetArraySize(pJson_attrs);
				if (attrs_size > 0)
				{
					for (m=0; m < attrs_size; m++)
					{
						cJSON *pJson_attrs_item = cJSON_GetArrayItem(pJson_attrs, m);
						if (pJson_attrs_item == NULL)
						{
							break;
						}

						cJSON *pJson_attrs_item_name = cJSON_GetObjectItem(pJson_attrs_item, "attrName");
						cJSON *pJson_attrs_item_value = cJSON_GetObjectItem(pJson_attrs_item, "attrValue");
						if (pJson_attrs_item_name->valuestring != NULL 
							&& pJson_attrs_item_value->valuestring != NULL
							&& strcmp(pJson_attrs_item_name->valuestring, "artist") == 0)
						{
							snprintf(artist_name, sizeof(artist_name), "%s的", pJson_attrs_item_value->valuestring);
						}
					}
				}
			}
			
			if (pJson_music_name != NULL && pJson_music_name->valuestring != NULL)
			{
				snprintf(p_links->link[p_links->link_size].link_name, sizeof(p_links->link[p_links->link_size].link_name),
					"正在给您播放%s%s", artist_name, pJson_music_name->valuestring);
			}
			else
			{
				snprintf(p_links->link[p_links->link_size].link_name, sizeof(p_links->link[p_links->link_size].link_name), 
					"%s", "现在开始播放");
			}
			p_links->link_size++;
		}
	}

	if (p_links->link_size == 0)
	{
		cJSON *pJson_tts = cJSON_GetObjectItem(json_data, "ttsText");
		if (pJson_tts != NULL && pJson_tts->valuestring != NULL)
		{
			ESP_LOGE(LOG_TAG, "nlp tts result:%s", pJson_tts->valuestring);
			nlp_result->type = NLP_RESULT_TYPE_CHAT;
			snprintf(nlp_result->chat_result.text, sizeof(nlp_result->chat_result.text), "%s", pJson_tts->valuestring);
		}
		else
		{
			nlp_result->type = NLP_RESULT_TYPE_NONE;
		}
	}
	
	return ESP_OK;
}

static int nlp_result_decode(char* str_reply, NLP_SERVICE_STATUS_T *status)
{	
	char* str_body = http_get_body(str_reply);
	if (NULL == str_body)
	{
#if PRINT_USER_EXPERIENCE_TEST_DATE
		//语义返回结果为空时的打印
		nlp_date_null_print();
#endif
		return ESP_FAIL;
	}
#if PRINT_USER_EXPERIENCE_TEST_DATE
	//获取语义处理返回的结果
	get_nlp_result(str_body);
#endif
	cJSON *pJson_body = cJSON_Parse(str_body);
    if (NULL != pJson_body) 
	{
        cJSON *pJson_cmd_data = cJSON_GetObjectItem(pJson_body, "commandData");
		if (NULL == pJson_cmd_data)
		{
			goto nlp_result_decode_error;
		}

		cJSON *pJson_cmd_name = cJSON_GetObjectItem(pJson_cmd_data, "commandName");
		if (NULL == pJson_cmd_name || pJson_cmd_name->valuestring == NULL)
		{
			goto nlp_result_decode_error;
		}
		
		if (strcmp(pJson_cmd_name->valuestring, COMMAND_NAME_VOLUME_CTRL) == 0)
		{
			volume_control(pJson_cmd_data);
			status->nlp_result.type = NLP_RESULT_TYPE_CMD;
		}
		else if (strcmp(pJson_cmd_name->valuestring, COMMAND_NAME_MEMO_CTRL) == 0)
		{
			if(memo_struct_handle != NULL) {
				if (get_memo_result(str_body) == 0)
					status->nlp_result.type = NLP_RESULT_TYPE_CMD;
			}
		}
		else
		{
			if (music_source_decode(pJson_cmd_data, status) != ESP_OK)
			{
				goto nlp_result_decode_error;
			}
		}		
    }
	else
	{
		ESP_LOGE(LOG_TAG, "nlp_result_decode cJSON_Parse error");
		ESP_LOGE(LOG_TAG, "[%d][%s]", strlen(str_body), str_body);
		goto nlp_result_decode_error;
	}

	if (NULL != pJson_body)
	{
		cJSON_Delete(pJson_body);
	}
	
	return ESP_OK;
	
nlp_result_decode_error:
	
	if (NULL != pJson_body)
	{
		cJSON_Delete(pJson_body);
	}

	return ESP_FAIL;
}


void task_deepbrain_nlp(void *pv)
{
	int ret 			= 0;
	int start_time		= 0;
	int end_time		= 0;
	int nlp_req_count	= 0;
	int nlp_req_total_ms = 0;
	NLP_SERVICE_STATUS_T *status = (NLP_SERVICE_STATUS_T *)g_nlp_service_status;
	static const char *DeepBrain_DEVICE_ID	= "18115025667";
	static const char *DeepBrain_USER_ID	= "18115025667";
	static const char *str_city_name		= " ";
	static const char *str_city_longitude	= "121.48";
	static const char *str_city_latitude	= "31.22";

	/* 语义请求的参数设置 */

	//create nlp handler
	void *nlp_handler = deep_brain_nlp_create();
	ESP_ERROR_CHECK(!nlp_handler);
	
	//set nlp request params
	deep_brain_set_params(nlp_handler, DB_NLP_PARAMS_APP_ID, DEEP_BRAIN_APP_ID);
	deep_brain_set_params(nlp_handler, DB_NLP_PARAMS_ROBOT_ID, DEEP_BRAIN_ROBOT_ID);
	deep_brain_set_params(nlp_handler, DB_NLP_PARAMS_DEVICE_ID, DeepBrain_DEVICE_ID);
	deep_brain_set_params(nlp_handler, DB_NLP_PARAMS_USER_ID, DeepBrain_USER_ID);
	deep_brain_set_params(nlp_handler, DB_NLP_PARAMS_CITY_NAME, str_city_name);
	deep_brain_set_params(nlp_handler, DB_NLP_PARAMS_CITY_LONGITUDE, str_city_longitude);
	deep_brain_set_params(nlp_handler, DB_NLP_PARAMS_CITY_LATITUDE, str_city_latitude);

	while (1) 
	{
		if (xQueueReceive(status->msg_queue, &status->p_msg, portMAX_DELAY) == pdPASS) 
		{	
			//判断消息类型是否为退出
			if (status->p_msg->msg_type == NLP_SERVICE_MSG_TYPE_QUIT)
			{
				esp32_free(status->p_msg);
				status->p_msg = NULL;
				break;
			}

			//判断消息长度
			if (strlen(status->p_msg->text) > 0)
			{

				start_time = get_time_of_day();
				//多次请求，防止由于网络不好造成的请求失败
				ret = deep_brain_nlp_process(nlp_handler, status->p_msg->text);
				if (ret != DB_ERROR_CODE_SUCCESS)
				{
					ret = deep_brain_nlp_process(nlp_handler, status->p_msg->text);
					if (ret != DB_ERROR_CODE_SUCCESS)
					{
#if PRINT_USER_EXPERIENCE_TEST_DATE
						//无法进行语义处理时的打印
						nlp_process_err_print();
#endif
						ESP_LOGE(LOG_TAG, "deep_brain_nlp_process failed");
						if (status->p_msg->cb)
						{
							status->nlp_result.type = NLP_RESULT_TYPE_NONE;
							status->p_msg->cb(&status->nlp_result);
						}
					}
				}
				end_time = abs(get_time_of_day() - start_time);
				nlp_req_count++;
				nlp_req_total_ms += end_time;
				ESP_LOGE(LOG_TAG, "nlp request:nlp_req_count=%d, avg cost=%dms, now cost=%dms, text=%s", 
					nlp_req_count, nlp_req_total_ms/nlp_req_count, end_time, status->p_msg->text);
#if PRINT_USER_EXPERIENCE_TEST_DATE
				//获取语义处理所需的时间
				get_nlp_cost_time(end_time);
#endif

				if (ret == DB_ERROR_CODE_SUCCESS)
				{
					memset(&status->nlp_result, 0, sizeof(status->nlp_result));
					snprintf(status->nlp_result.input_text, sizeof(status->nlp_result.input_text), "%s", status->p_msg->text);
					if (nlp_result_decode(deep_brain_nlp_result(nlp_handler), status) != ESP_OK)
					{
#if PRINT_USER_EXPERIENCE_TEST_DATE
						//语义处理失败情况下的打印
						nlp_date_err_print();
#endif
						ESP_LOGE(LOG_TAG, "nlp_result_process failed");
						if (status->p_msg->cb)
						{
							status->nlp_result.type = NLP_RESULT_TYPE_NONE;
							status->p_msg->cb(&status->nlp_result);
						}
					}
					else
					{
#if PRINT_USER_EXPERIENCE_TEST_DATE
						//从语音识别到语义处理 流程完全正常情况下的打印，即正常数据
						print_date();
#endif
						if (status->p_msg->cb)
						{
							status->p_msg->cb(&status->nlp_result);
						}
					}
				}
			}

			esp32_free(status->p_msg);
			status->p_msg = NULL;	
		}
	}

	deep_brain_nlp_destroy(nlp_handler);
	vTaskDelete(NULL);
}

static void nlp_service_send_msg(
	NLP_SERVICE_MSG_TYPE_T msg_type, 
	char *text,
	nlp_service_result_cb cb)
{
	if (g_nlp_service_status == NULL || g_nlp_service_status->msg_queue == NULL)
	{
		return;
	}

	NLP_SERVICE_REQUEST_MSG_T *p_msg = (NLP_SERVICE_REQUEST_MSG_T *)esp32_malloc(sizeof(NLP_SERVICE_REQUEST_MSG_T));
	if (p_msg == NULL)
	{
		ESP_LOGE(LOG_TAG, "nlp_service_send_msg malloc failed, out of memory");
		return;
	}
	p_msg->msg_type = msg_type;
	if (text != NULL)
	{
		snprintf(p_msg->text, sizeof(p_msg->text), "%s", text);
	}
	p_msg->cb = cb;

	if (xQueueSend(g_nlp_service_status->msg_queue, &p_msg, 0) != pdPASS)
	{
		ESP_LOGE(LOG_TAG, "nlp_service_send_msg xQueueSend failed");
		esp32_free(p_msg);
		p_msg = NULL;
	}
}

void nlp_service_create(void)
{
	g_nlp_service_status = (NLP_SERVICE_STATUS_T *)esp32_malloc(sizeof(NLP_SERVICE_STATUS_T));
	if (g_nlp_service_status == NULL)
	{
		ESP_LOGE(LOG_TAG, "nlp_service_create failed, out of memory");
		return;
	}
	memset((char*)g_nlp_service_status, 0, sizeof(NLP_SERVICE_STATUS_T));
	g_nlp_service_status->msg_queue = xQueueCreate(5, sizeof(char *));

	
    if (xTaskCreate(task_deepbrain_nlp,
                    "task_deepbrain_nlp",
                    1024*4,
                    g_nlp_service_status,
                    4,
                    NULL) != pdPASS) {

        ESP_LOGE(LOG_TAG, "ERROR creating task_deepbrain_nlp task! Out of memory?");
    }
}

void nlp_service_delete(void)
{
	nlp_service_send_msg(NLP_SERVICE_MSG_TYPE_QUIT, NULL, NULL);
}

void nlp_service_send_request(const char *text, nlp_service_result_cb cb)
{
	nlp_service_send_msg(NLP_SERVICE_MSG_TYPE_REQUEST, text, cb);
}

