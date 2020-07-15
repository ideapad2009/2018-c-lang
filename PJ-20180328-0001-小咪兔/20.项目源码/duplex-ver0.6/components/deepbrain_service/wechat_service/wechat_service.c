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
#include "keyboard_manage.h"
#include "keyboard_service.h"
#include "wechat_service.h"
#include "led.h"
#include "player_middleware.h"
#include "interf_enc.h"
#include "mpush_service.h"
#include "bind_device.h"
#include "ReSampling.h"

#define LOG_TAG "wechat service"

#define WECHAT_AMR_MAX_TIME	10
#define WECHAT_AMR_MAX_TIME_MS	(WECHAT_AMR_MAX_TIME*1000)
#define WECHAT_AMR_ONE_MS_AUDIO_SIZE 32
#define WECHAT_AMR_ONE_SEC_AUDIO_SIZE 1600
#define WECHAT_MAX_AMR_AUDIO_SIZE (WECHAT_AMR_MAX_TIME*WECHAT_AMR_ONE_SEC_AUDIO_SIZE + 6)

typedef struct
{
	xQueueHandle msg_queue;
}WECHAT_SERVICE_HANDLE_T;

typedef enum
{
	WECHAT_SERVICE_RECORD_START,
	WECHAT_SERVICE_RECORD_READ,
	WECHAT_SERVICE_RECORD_STOP,
	WECHAT_SERVICE_QUIT,
}WECHAT_MSG_TYPE_T;

typedef struct 
{
	int record_id;
	int record_len;
	int record_ms;
	int is_max_ms;
	char data[AUDIO_RECORD_BUFFER_SIZE];
}WECHAT_RECORD_DATA_T;

typedef struct
{
	char asr_result[512];
}WECHAT_ASR_RESULT_T;

typedef struct
{
	WECHAT_MSG_TYPE_T msg_type;
	WECHAT_RECORD_DATA_T *p_record_data;
}WECHAT_MSG_T;

static WECHAT_SERVICE_HANDLE_T *g_wechat_handle = NULL;

static void task_wechat(void *pv)
{
	int ret = 0;
	int i = 0;
    DEVICE_KEY_EVENT_T key_event = DEVICE_KEY_START;
    WECHAT_SERVICE_HANDLE_T *wechat_handle = (WECHAT_SERVICE_HANDLE_T *)pv;
	WECHAT_MSG_T msg = {0};
	void *amr_encode_handle = NULL;
	int arm_data_len = 0;
	RESAMPLE resample = {0};
	FILE *p_pcm_file = NULL;
	char amr_data_frame[WECHAT_AMR_ONE_MS_AUDIO_SIZE] = {0};

	WECHAT_RECORD_DATA_T *p_wechat_data = esp32_malloc(sizeof(WECHAT_RECORD_DATA_T));
	if (p_wechat_data == NULL)
	{
		ESP_LOGE(LOG_TAG, "task_wechat p_wechat_data esp32_malloc(%d) failed", sizeof(WECHAT_RECORD_DATA_T));
		return;
	}
	
	WECHAT_ASR_RESULT_T* p_asr_result = esp32_malloc(sizeof(WECHAT_ASR_RESULT_T));
	if (p_asr_result == NULL)
	{
		ESP_LOGE(LOG_TAG, "task_wechat p_asr_result esp32_malloc(%d) failed", sizeof(WECHAT_ASR_RESULT_T));
		return;
	}
	
	char *amr_data = (char*)esp32_malloc(WECHAT_MAX_AMR_AUDIO_SIZE);
	if (amr_data == NULL)
	{
		ESP_LOGE(LOG_TAG, "task_wechat amr_data esp32_malloc(%d) failed", WECHAT_MAX_AMR_AUDIO_SIZE);
		return;
	}

	BAIDU_ACOUNT_T *baidu_acount = esp32_malloc(sizeof(BAIDU_ACOUNT_T));
	if (baidu_acount == NULL)
	{
		ESP_LOGE(LOG_TAG, "task_wechat baidu_acount esp32_malloc(%d) failed", sizeof(BAIDU_ACOUNT_T));
		return;
	}
	
    while (1) 
	{
		if (amr_encode_handle == NULL)
		{
			amr_encode_handle = Encoder_Interface_init(0);
			if (amr_encode_handle == NULL)
			{
				ESP_LOGE(LOG_TAG, "Encoder_Interface_init failed");
				vTaskDelay(1000);
				continue;
			}
		}
		
		if (xQueueReceive(wechat_handle->msg_queue, &msg, portMAX_DELAY) == pdPASS) 
		{			
			switch (msg.msg_type)
			{
				case WECHAT_SERVICE_RECORD_START:
				{
					snprintf(amr_data, WECHAT_MAX_AMR_AUDIO_SIZE, "%s", AMR_HEADER);
					arm_data_len = strlen(AMR_HEADER);
#if DEBUG_RECORD_PCM
					static int count = 1;
					char str_record_path[32] = {0};
					snprintf(str_record_path, sizeof(str_record_path), "/sdcard/wechat%d.pcm", count++);
					p_pcm_file = fopen(str_record_path, "w+");
#endif

				}
				case WECHAT_SERVICE_RECORD_READ:
				{
					if (msg.p_record_data == NULL)
					{
						break;
					}

#if DEBUG_RECORD_PCM
					if (p_pcm_file != NULL)
					{
						fwrite(msg.p_record_data->data, msg.p_record_data->record_len, 1, p_pcm_file);
					}
#endif

					//降采样
					ret = unitSampleProc((short *)msg.p_record_data->data, (short *)p_wechat_data->data, 
						AUDIO_PCM_RATE, 8000, msg.p_record_data->record_len, 1, &resample);
					if (ret > 0)
					{
						p_wechat_data->record_len = ret;
					}
					else
					{
						ESP_LOGE(LOG_TAG, "task_wechat unitSampleProc failed[%d]", ret);
						break;
					}

					//转AMR
					for (i=0; (i*320) < p_wechat_data->record_len; i++)
					{
						if ((arm_data_len + sizeof(amr_data_frame)) > WECHAT_MAX_AMR_AUDIO_SIZE)
						{
							break;
						}
						
						ret = Encoder_Interface_Encode(amr_encode_handle, MR122, 
							(short*)(p_wechat_data->data + i*320), &amr_data_frame, 0);
						if (ret == sizeof(amr_data_frame))
						{
							memcpy(amr_data + arm_data_len, amr_data_frame, sizeof(amr_data_frame));
							arm_data_len += sizeof(amr_data_frame);
						}
						else
						{
							ESP_LOGE(LOG_TAG, "task_wechat Encoder_Interface_Encode failed[%d]", ret);
						}
					}
					break;
				}
				case WECHAT_SERVICE_RECORD_STOP:
				{
#if DEBUG_RECORD_PCM
					if (p_pcm_file != NULL)
					{
						fclose(p_pcm_file);
						p_pcm_file = NULL;
					}
#endif

					//语音小于1秒
					if (msg.p_record_data->record_ms < 1000)
					{
						//长按微聊键 听到嘟声后 说出你想说的
						player_mdware_play_tone(FLASH_MUSIC_WECHAT_SHORT_AUDIO);
						break;
					}
					player_mdware_play_tone(FLASH_MUSIC_MSG_SEND);
					
					//AMR语音识别
					get_ai_acount(AI_ACOUNT_BAIDU, baidu_acount);
					memset(p_asr_result, 0, sizeof(WECHAT_ASR_RESULT_T));

#if	DEBUG_RECORD_SDCARD
					static int count = 1;
					FILE *p_record_file = NULL;
					char str_record_path[32] = {0};
					snprintf(str_record_path, sizeof(str_record_path), "/sdcard/2-%d.amr", count++);
					p_record_file = fopen(str_record_path, "w+");
					if (p_record_file != NULL)
					{
						fwrite(amr_data, arm_data_len, 1, p_record_file);
						fclose(p_record_file);
					}
#endif

					int start_time = get_time_of_day();
					if (baidu_asr(baidu_acount, p_asr_result->asr_result, sizeof(p_asr_result->asr_result), 
						(unsigned char*)amr_data, arm_data_len) == BAIDU_ASR_FAIL)
					{
						ESP_LOGE(LOG_TAG, "task_wechat baidu_asr first fail");
						if (baidu_asr(baidu_acount, p_asr_result->asr_result, sizeof(p_asr_result->asr_result), 
							(unsigned char*)amr_data, arm_data_len) == BAIDU_ASR_FAIL)
						{
							ESP_LOGE(LOG_TAG, "task_wechat baidu_asr second fail");
						}
					}

					int asr_cost = get_time_of_day() - start_time;
					ESP_LOGI(LOG_TAG, "baidu asr:record_len[%d],cost[%dms],result[%s]", arm_data_len, asr_cost, p_asr_result->asr_result);
					
					//微聊发送
					if (mpush_service_send_file(amr_data, arm_data_len, p_asr_result->asr_result, "amr", "") == MPUSH_ERROR_SEND_MSG_OK)
					{
						player_mdware_play_tone(FLASH_MUSIC_SEND_SUCCESS);
						ESP_LOGI(LOG_TAG, "mpush_service_send_file success"); 
					}
					else
					{
						player_mdware_play_tone(FLASH_MUSIC_SEND_FAIL);	
						ESP_LOGE(LOG_TAG, "mpush_service_send_file failed"); 
					}
					break;
				}
				default:
					break;
			}

			if (msg.msg_type == WECHAT_SERVICE_QUIT)
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

	Encoder_Interface_exit(amr_encode_handle);
    vTaskDelete(NULL);
}

void set_wechat_record_data(
	int id, char *data, size_t data_len, int record_ms, int is_max)
{
	WECHAT_MSG_T msg = {0};

	if (g_wechat_handle == NULL || g_wechat_handle->msg_queue == NULL)
	{
		ESP_LOGE(LOG_TAG, "set_wechat_record_data invalid parmas");
		return;
	}

	msg.p_record_data = (WECHAT_RECORD_DATA_T *)esp32_malloc(sizeof(WECHAT_RECORD_DATA_T));
	if (msg.p_record_data == NULL)
	{
		ESP_LOGE(LOG_TAG, "set_wechat_record_data esp32_malloc failed");
		return;
	}

	memset(msg.p_record_data, 0, sizeof(WECHAT_RECORD_DATA_T));
	if (id == 1)
	{
		msg.msg_type = WECHAT_SERVICE_RECORD_START;
	}
	else if (id > 1)
	{
		msg.msg_type = WECHAT_SERVICE_RECORD_READ;
	}
	else
	{
		msg.msg_type = WECHAT_SERVICE_RECORD_STOP;
	}
	
	msg.p_record_data->record_id = id;
	msg.p_record_data->record_len = data_len;
	msg.p_record_data->record_ms = record_ms;
	msg.p_record_data->is_max_ms = is_max;
	
	if (data_len > 0)
	{
		memcpy(msg.p_record_data->data, data, data_len);
	}

	if (xQueueSend(g_wechat_handle->msg_queue, &msg, 0) != pdPASS)
	{
		ESP_LOGE(LOG_TAG, "xQueueSend g_wechat_handle->msg_queue failed");
		esp32_free(msg.p_record_data);
		msg.p_record_data = NULL;
	}
}


void wechat_service_create(void)
{
	g_wechat_handle = (WECHAT_SERVICE_HANDLE_T *)esp32_malloc(sizeof(WECHAT_SERVICE_HANDLE_T));
	if (g_wechat_handle == NULL)
	{
		return;
	}
	
	g_wechat_handle->msg_queue = xQueueCreate(20, sizeof(WECHAT_MSG_T));
	
    if (xTaskCreate(task_wechat,
                    "task_wechat",
                    1024*8,
                    g_wechat_handle,
                    4,
                    NULL) != pdPASS) {

        ESP_LOGE(LOG_TAG, "ERROR creating task_wechat task! Out of memory?");
    }
}

void wechat_service_delete(void)
{
	WECHAT_MSG_T msg = {0};
	msg.msg_type = WECHAT_SERVICE_QUIT;
	
	if (g_wechat_handle != NULL && g_wechat_handle->msg_queue != NULL)
	{
		xQueueSend(g_wechat_handle->msg_queue, &msg, 0);
	}
}

void wechat_record_start(void)
{
	player_mdware_start_record(1, WECHAT_AMR_MAX_TIME_MS, set_wechat_record_data);
}

void wechat_record_stop(void)
{
	player_mdware_stop_record();
}



