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
#include "user_experience_test_data_manage.h"
#include "deepbrain_nlp_decode.h"
//#include "translate_manage.h"

#define LOG_TAG "nlp service" 

typedef enum
{
	NLP_SERVICE_MSG_TYPE_REQUEST,
	NLP_SERVICE_MSG_TYPE_DECODE,
	NLP_SERVICE_MSG_TYPE_TRANSLATE,
	NLP_SERVICE_MSG_TYPE_QUIT,	
}NLP_SERVICE_MSG_TYPE_T;

typedef struct
{
	NLP_SERVICE_MSG_TYPE_T msg_type;
	char text[512];
	nlp_result_cb cb;
	char nlp_json_data[30*1024];
}NLP_SERVICE_REQUEST_MSG_T;

typedef struct
{
	xQueueHandle msg_queue;
	NLP_SERVICE_REQUEST_MSG_T *p_msg;
	NLP_RESULT_T nlp_result;
}NLP_SERVICE_STATUS_T;

static NLP_SERVICE_STATUS_T *g_nlp_service_status = NULL;

extern MEMO_SERVICE_HANDLE_T *memo_struct_handle;

static int msg_type_process(NLP_SERVICE_STATUS_T *status, void *nlp_handler)
{
	int ret 			= DB_ERROR_CODE_HTTP_FAIL;
	int start_time		= 0;
	int end_time		= 0;
	int nlp_req_count	= 0;
	int nlp_req_total_ms = 0;
	char *nlp_text 	= NULL;

	switch (status->p_msg->msg_type)
	{
		case NLP_SERVICE_MSG_TYPE_REQUEST:		//nlp请求
		{
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
				nlp_text = http_get_body(deep_brain_nlp_result(nlp_handler));
				if (nlp_text == NULL)
				{
					if (status->p_msg->cb)
					{
						status->nlp_result.type = NLP_RESULT_TYPE_NONE;
						status->p_msg->cb(&status->nlp_result);
					}
					ret = DB_ERROR_CODE_HTTP_FAIL;
				}
			}
			if (ret == DB_ERROR_CODE_SUCCESS && nlp_text != NULL)
			{
				memset(&status->nlp_result, 0, sizeof(status->nlp_result));
				snprintf(status->nlp_result.input_text, sizeof(status->nlp_result.input_text), "%s", status->p_msg->text);
				if (dp_nlp_result_decode(nlp_text, &status->nlp_result) != NLP_DECODE_ERRNO_OK)
				{
					ESP_LOGE(LOG_TAG, "nlp_result_decode failed");
					if (status->p_msg->cb)
					{
						status->p_msg->cb(&status->nlp_result);
					}
				}
				else
				{
					ESP_LOGE(LOG_TAG, "nlp_result_decode success");
					if (status->p_msg->cb)
					{
						status->p_msg->cb(&status->nlp_result);
					}
				}
			}
			break;
		}
		case NLP_SERVICE_MSG_TYPE_QUIT:			//退出
		{
			esp32_free(status->p_msg);
			status->p_msg = NULL;
			return 1;
		}
		default:
			break;
	}
	
	return 0;
}

void task_deepbrain_nlp(void *pv)
{
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
			if (msg_type_process(status, nlp_handler) != 0)
			{
				esp32_free(status->p_msg);
				status->p_msg = NULL;
				break;
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
	nlp_result_cb cb)
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
		if (p_msg->msg_type == NLP_SERVICE_MSG_TYPE_DECODE)
		{
			snprintf(p_msg->nlp_json_data, sizeof(p_msg->nlp_json_data), "%s", text);
		}
		else
		{
			snprintf(p_msg->text, sizeof(p_msg->text), "%s", text);
		}
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
                    1024*6,
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

void nlp_service_send_request(const char *text, nlp_result_cb cb)
{
	nlp_service_send_msg(NLP_SERVICE_MSG_TYPE_REQUEST, text, cb);
}

void nlp_service_send_translate_request(const char *text, nlp_result_cb cb)
{
	nlp_service_send_msg(NLP_SERVICE_MSG_TYPE_TRANSLATE, text, cb);
}


