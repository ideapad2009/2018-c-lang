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

#include "cJSON.h"
#include "touchpad.h"
#include "DeviceCommon.h"
#include "MediaControl.h"
#include "MediaHal.h"
#include "toneuri.h"
#include "http_api.h"
#include "device_api.h"
#include "socket.h"
#include "lwip/netdb.h"
#include "EspAudio.h"
#include "userconfig.h"
#include "Recorder.h"
#include "player_middleware.h"

#define PLAY_TONE_WRITE_SIZE  (1024*2)
#define TRANSLATE_RESULT_BUFFER_SIZE (500*1024)

// audio record
typedef enum
{
	AUDIO_RAW_READ_STATUS_INIT,
	AUDIO_RAW_READ_STATUS_IDEL,
	AUDIO_RAW_READ_STATUS_START,
	AUDIO_RAW_READ_STATUS_READING,
	AUDIO_RAW_READ_STATUS_STOP,
}AUDIO_RAW_READ_STATUS_T;

typedef enum
{	
	PLAY_TONE_STATUS_IDEL = 0,
	PLAY_TONE_STATUS_START,
	PLAY_TONE_STATUS_PLAY,
	PLAY_TONE_STATUS_STOP,
}PLAY_TONE_STATUS_T;

typedef struct
{
	int data_len;
	char data[TRANSLATE_RESULT_BUFFER_SIZE];
}TRANSLATE_RESULT_BUFF_T;

PLAYER_MDWARE_CONFIG_T g_player_mdware_config = {0};
TRANSLATE_RESULT_BUFF_T *g_translate_buffer = NULL;

#define TAG_PLAYER_MDWARE "PLAYER MIDDLERWARE"

int get_player_mdware_status(void)
{
	return g_player_mdware_config.cur_status;
}

void set_player_mdware_status(int status)
{
	g_player_mdware_config.last_status = g_player_mdware_config.cur_status;
	g_player_mdware_config.cur_status = status;
}

int get_player_mdware_last_status(void)
{
	return g_player_mdware_config.last_status;
}

void player_mdware_set_translate_result(char *data, size_t data_len)
{
	if (g_translate_buffer == NULL)
	{
		g_translate_buffer = esp32_malloc(sizeof(TRANSLATE_RESULT_BUFF_T));
		if (g_translate_buffer == NULL)
		{
			return;
		}
		memset(g_translate_buffer, 0, sizeof(TRANSLATE_RESULT_BUFF_T));
	}

	g_translate_buffer->data_len = data_len;
	memcpy(g_translate_buffer->data, data, data_len);
	player_mdware_play_translate_result();
}

void player_mdware_play_translate_result(void)
{
	if (g_player_mdware_config.queue_player_msg == NULL || g_translate_buffer == NULL)
	{
		return;
	}

	PLAYER_MDWARE_MSG_T *p_msg = (PLAYER_MDWARE_MSG_T *)esp32_malloc(sizeof(PLAYER_MDWARE_MSG_T));

	if (p_msg == NULL)
	{
		return;
	}
	memset(p_msg, 0, sizeof(PLAYER_MDWARE_MSG_T));
	p_msg->msg_type = PLAYER_MDWARE_MSG_PLAY_TONE;
	p_msg->msg_play_tone.audio_len = g_translate_buffer->data_len;
	p_msg->msg_play_tone.audio_data = g_translate_buffer->data;
	p_msg->msg_play_tone.is_translate = 1;

	if (xQueueSend(g_player_mdware_config.queue_player_msg, &p_msg, 0) != pdPASS)
	{
		esp32_free(p_msg);
		p_msg = NULL;
	}
}

void player_mdware_play_tone(FLASH_MUSIC_INDEX_T flash_music_index)
{
	EspAudioTonePlay(get_flash_music_name_ex(flash_music_index), TERMINATION_TYPE_NOW);
}

void player_mdware_start_record(
	int record_id, 
	int record_ms,
	player_mdware_record_cb cb)
{
	if (g_player_mdware_config.queue_player_msg == NULL)
	{
		return;
	}

	PLAYER_MDWARE_MSG_T *p_msg = (PLAYER_MDWARE_MSG_T *)esp32_malloc(sizeof(PLAYER_MDWARE_MSG_T));

	if (p_msg == NULL)
	{
		return;
	}
	memset(p_msg, 0, sizeof(PLAYER_MDWARE_MSG_T));
	p_msg->msg_type = PLAYER_MDWARE_MSG_RECORD_START;
	p_msg->msg_start_recrod.record_id = record_id;
	p_msg->msg_start_recrod.cb = cb;
	p_msg->msg_start_recrod.record_ms = record_ms;

	if (xQueueSend(g_player_mdware_config.queue_player_msg, &p_msg, 0) != pdPASS)
	{
		esp32_free(p_msg);
		p_msg = NULL;
	}
}


void player_mdware_stop_record(void)
{
	if (g_player_mdware_config.queue_player_msg == NULL)
	{
		return;
	}

	PLAYER_MDWARE_MSG_T *p_msg = (PLAYER_MDWARE_MSG_T *)esp32_malloc(sizeof(PLAYER_MDWARE_MSG_T));

	if (p_msg == NULL)
	{
		return;
	}
	memset(p_msg, 0, sizeof(PLAYER_MDWARE_MSG_T));
	p_msg->msg_type = PLAYER_MDWARE_MSG_RECORD_STOP;

	if (xQueueSend(g_player_mdware_config.queue_player_msg, &p_msg, 0) != pdPASS)
	{
		esp32_free(p_msg);
		p_msg = NULL;
	}
}

static void start_record(PLAYER_MDWARE_MSG_T *p_msg)
{
	if (g_player_mdware_config.queue_record_msg == NULL || p_msg == NULL)
	{
		return;
	}

	if (xQueueSend(g_player_mdware_config.queue_record_msg, &p_msg, 0) != pdPASS)
	{
		esp32_free(p_msg);
		p_msg = NULL;
	}
}

static void stop_record(PLAYER_MDWARE_MSG_T *p_msg)
{
	if (g_player_mdware_config.queue_record_msg == NULL || p_msg == NULL)
	{
		return;
	}

	if (xQueueSend(g_player_mdware_config.queue_record_msg, &p_msg, 0) != pdPASS)
	{
		esp32_free(p_msg);
		p_msg = NULL;
	}
}

static void play_tone(PLAYER_MDWARE_MSG_T *p_msg)
{
	if (g_player_mdware_config.queue_play_tone_msg == NULL || p_msg == NULL)
	{
		return;
	}

	if (xQueueSend(g_player_mdware_config.queue_play_tone_msg, &p_msg, 0) != pdPASS)
	{
		esp32_free(p_msg);
		p_msg = NULL;
	}
}

static void stop_tone(void)
{
	if (g_player_mdware_config.queue_play_tone_msg == NULL)
	{
		return;
	}

	PLAYER_MDWARE_MSG_T *p_msg = (PLAYER_MDWARE_MSG_T *)esp32_malloc(sizeof(PLAYER_MDWARE_MSG_T));

	if (p_msg == NULL)
	{
		return;
	}
	memset(p_msg, 0, sizeof(PLAYER_MDWARE_MSG_T));
	p_msg->msg_type = PLAYER_MDWARE_MSG_STOP_TONE;

	if (xQueueSend(g_player_mdware_config.queue_play_tone_msg, &p_msg, 0) != pdPASS)
	{
		esp32_free(p_msg);
		p_msg = NULL;
	}
}



static void task_player_middlerware(void)
{
	int status = PLAYER_MDWARE_STATUS_IDEL;
	PLAYER_MDWARE_MSG_T *p_msg;
	
	while(1)
	{
		BaseType_t xStatus = xQueueReceive(g_player_mdware_config.queue_player_msg, &p_msg, portMAX_DELAY);
		if (xStatus == pdPASS)
		{
			switch (p_msg->msg_type)
			{
				case PLAYER_MDWARE_MSG_PLAY_TONE:
				{	
					switch (status)
					{
						case PLAYER_MDWARE_STATUS_IDEL:
						case PLAYER_MDWARE_STATUS_PLAY_TONE:
						case PLAYER_MDWARE_STATUS_RECORD_STOP:
						case PLAYER_MDWARE_STATUS_RECORD_START:
						{
							play_tone(p_msg);
							status = PLAYER_MDWARE_STATUS_PLAY_TONE;
							break;
						}
						default:
						{
							esp32_free(p_msg);
							p_msg = NULL;
							break;
						}
					}
					break;
				}
				case PLAYER_MDWARE_MSG_PLAY_MUSIC:
				{
					break;
				}
				case PLAYER_MDWARE_MSG_RECORD_START:
				{	
					switch (status)
					{
						case PLAYER_MDWARE_STATUS_IDEL:
						case PLAYER_MDWARE_STATUS_PLAY_TONE:
						case PLAYER_MDWARE_STATUS_RECORD_STOP:
						case PLAYER_MDWARE_STATUS_RECORD_START:
						{
							start_record(p_msg);
							status = PLAYER_MDWARE_STATUS_RECORD_START;
							break;
						}
					}
					break;
				}
				case PLAYER_MDWARE_MSG_RECORD_STOP:
				{		
					switch (status)
					{
						case PLAYER_MDWARE_STATUS_RECORD_START:
						{
							stop_record(p_msg);
							status = PLAYER_MDWARE_STATUS_RECORD_STOP;
							break;
						}
						case PLAYER_MDWARE_STATUS_IDEL:
						case PLAYER_MDWARE_STATUS_PLAY_TONE:
						case PLAYER_MDWARE_STATUS_RECORD_STOP:
						{
							esp32_free(p_msg);
							p_msg = NULL;
							break;
						}
					}
					break;
				}
				default:
					break;
			}
		}
	}
	vTaskDelete(NULL);
}

static void task_play_tone(void *pv)
{
	int ret = ESP_OK;
	PLAY_TONE_STATUS_T status = PLAY_TONE_STATUS_IDEL;
	PLAYER_MDWARE_MSG_T *p_msg = NULL; 
	PLAYER_MDWARE_MSG_PLAY_TONE_T msg_play_tone = {0};
	int tone_curr_pos = 0;
	int raw_write_error_count = 0;
	
	while (1) 
	{
		BaseType_t xStatus = xQueueReceive(g_player_mdware_config.queue_play_tone_msg, &p_msg, (TickType_t)50);
		if (xStatus == pdPASS)
		{
			switch (p_msg->msg_type)
			{
				case PLAYER_MDWARE_MSG_PLAY_TONE:
				{
					if (status == PLAY_TONE_STATUS_PLAY)
					{
						ret = EspAudioRawStop(TERMINATION_TYPE_NOW); 
						if (ret != ESP_OK)
						{
							ESP_LOGE(TAG_PLAYER_MDWARE, "task_play_tone EspAudioRawStop 1 failed");
						}
					}
					msg_play_tone = p_msg->msg_play_tone;
					status = PLAY_TONE_STATUS_START;
					break;
				}
				case PLAYER_MDWARE_MSG_STOP_TONE:
				{
					ret = EspAudioRawStop(TERMINATION_TYPE_NOW); 
					if (ret != ESP_OK)
					{
						ESP_LOGE(TAG_PLAYER_MDWARE, "task_play_tone EspAudioRawStop 2 failed");
					}
					status = PLAY_TONE_STATUS_STOP;
					msg_play_tone = p_msg->msg_play_tone;
					break;
				}
				default:
					break;
			}

			esp32_free(p_msg);
			p_msg = NULL;
		}

		switch (status)
		{
			case PLAY_TONE_STATUS_IDEL:
			{
				break;
			}
			case PLAY_TONE_STATUS_START:
			{
				if (msg_play_tone.is_translate)
				{
					//led_ctrl_send_msg(MSG_LED_CTRL_TRANS_YELLOW_ON);
				}

				ret = EspAudioRawStart("raw://from.mp3/to.mp3#i2s");
				if (ret != ESP_OK)
				{
					status = PLAY_TONE_STATUS_STOP;
					ESP_LOGE(TAG_PLAYER_MDWARE, "task_play_tone EspAudioRawStart failed");
					break;
				}
				
				tone_curr_pos = 0;
				status = PLAY_TONE_STATUS_PLAY;
				vTaskDelay(200);
				break;
			}
			case PLAY_TONE_STATUS_PLAY:
			{
				int len = PLAY_TONE_WRITE_SIZE;

				if (msg_play_tone.audio_len - tone_curr_pos < PLAY_TONE_WRITE_SIZE)
				{
					len = msg_play_tone.audio_len - tone_curr_pos;
				}

				ret = EspAudioRawWrite(msg_play_tone.audio_data + tone_curr_pos, len, NULL);
				if (ret == ESP_OK)
				{
					raw_write_error_count = 0;
					tone_curr_pos += len;
					if (tone_curr_pos == msg_play_tone.audio_len)
					{
						ret = EspAudioRawStop(TERMINATION_TYPE_DONE);
						if (ret != ESP_OK)
						{
							ESP_LOGE(TAG_PLAYER_MDWARE, "task_play_tone EspAudioRawStop 3 failed");
						}
						status = PLAY_TONE_STATUS_STOP;
					}
				}
				else
				{
					raw_write_error_count++;
					if (raw_write_error_count >= 5)
					{
						status = PLAY_TONE_STATUS_STOP;
					}
					ESP_LOGE(TAG_PLAYER_MDWARE, "EspAudioRawWrite failed");
				}
				
				break;
			}
			case PLAY_TONE_STATUS_STOP:
			{
				PlayerStatus player_status;
				EspAudioStatusGet(&player_status);
				//ESP_LOGE(TAG_PLAY_TTS, "player_status.status=%d,mode=%d", player_status.status, player_status.mode);
				if (player_status.status == AUDIO_STATUS_PLAYING)
				{
					break;
				}
				
				if (msg_play_tone.is_translate)
				{
					//led_ctrl_send_msg(MSG_LED_CTRL_TRANS_YELLOW_OFF);
				}
				status = PLAY_TONE_STATUS_IDEL;
				break;
			}
			default:
				break;
		}
	}
	vTaskDelete(NULL);
}

static void task_record_audio(void *pv)
{	
	int ret = ESP_OK;
	int len = 0;
	int record_id = 0;
	int record_ms = 0;
	PLAYER_MDWARE_MSG_START_RECORD_T record_params = {0};
	AUDIO_RAW_READ_STATUS_T status = AUDIO_RAW_READ_STATUS_INIT;
	char* audio_raw_data = (char*)esp32_malloc(AUDIO_RECORD_BUFFER_SIZE);
	PLAYER_MDWARE_MSG_T *p_msg = NULL; 
	
    while (1) 
	{		
		BaseType_t xStatus = xQueueReceive(g_player_mdware_config.queue_record_msg, &p_msg, (TickType_t)20);
		if (xStatus == pdPASS)
		{
			switch (p_msg->msg_type)
			{
				case PLAYER_MDWARE_MSG_RECORD_START:
				{
					//ESP_LOGE(TAG_PLAYER_MDWARE, "PLAYER_MDWARE_MSG_RECORD_START!");
					if (record_params.cb != NULL)
					{
						record_id++;
						record_params.cb(record_id*(-1), NULL, 0, record_ms, 0);
					}
					status = AUDIO_RAW_READ_STATUS_START;
					record_params = p_msg->msg_start_recrod;
					break;
				}
				case PLAYER_MDWARE_MSG_RECORD_STOP:
				{
					//ESP_LOGE(TAG_PLAYER_MDWARE, "PLAYER_MDWARE_MSG_RECORD_STOP!");
					if (status == AUDIO_RAW_READ_STATUS_READING
						|| status == AUDIO_RAW_READ_STATUS_START)
					{
						status = AUDIO_RAW_READ_STATUS_STOP;
					}
					break;
				}
				default:
					break;
			}
	
			esp32_free(p_msg);
			p_msg = NULL;
		}

		switch (status)
		{
			case AUDIO_RAW_READ_STATUS_INIT:
			{
				int ret = EspAudioRecorderStart("i2s://16000:1@record.pcm#raw", 10 * 1024, 0);
				if (AUDIO_ERR_NO_ERROR != ret) 
				{
					ESP_LOGE(TAG_PLAYER_MDWARE, "### EspAudioRecorderStart failed, error=%d ###", ret);
					vTaskDelay(1000);
					continue;
				}
				status = AUDIO_RAW_READ_STATUS_IDEL;
			}
			case AUDIO_RAW_READ_STATUS_IDEL:
			{
				break;
			}
			case AUDIO_RAW_READ_STATUS_START:
			{
				record_id = 0;
				record_ms = 0;
				status = AUDIO_RAW_READ_STATUS_READING;
				break;
			}
			case AUDIO_RAW_READ_STATUS_READING:
			{
				len = 0;
				ret = EspAudioRecorderRead((unsigned char*)audio_raw_data, AUDIO_RECORD_BUFFER_SIZE, &len);
				if (AUDIO_ERR_NOT_READY == ret) 
				{
					ESP_LOGE(TAG_PLAYER_MDWARE, "###EspAudioRecorderRead failed, error=%d ###", ret);
					break;
				}

				//ESP_LOGI(TAG_PLAYER_MDWARE, "EspAudioRecorderRead success, len=%d!", len);
				if (len > 0)
				{
					record_id++;
					record_ms += len/AUDIO_PCM_ONE_MS_BYTE;
					
					if (record_params.cb != NULL)
					{
						if (record_ms >= record_params.record_ms)
						{
							status = AUDIO_RAW_READ_STATUS_STOP;
							record_params.cb(record_id, audio_raw_data, len, record_ms, 1);
						}
						else
						{
							record_params.cb(record_id, audio_raw_data, len, record_ms, 0);
						}
					}
				}
				break;
			}
			case AUDIO_RAW_READ_STATUS_STOP:
			{
				if (record_params.cb != NULL)
				{
					record_id++;
					record_params.cb(record_id*(-1), NULL, 0, record_ms, 0);
				}
				record_params.cb = NULL;
				status = AUDIO_RAW_READ_STATUS_IDEL;
				break;
			}
			default:
				break;
		}
    }
	
    vTaskDelete(NULL);
}

void create_player_middleware(void)
{
	g_player_mdware_config.cur_status = PLAYER_MDWARE_STATUS_IDEL;
	g_player_mdware_config.last_status = PLAYER_MDWARE_STATUS_IDEL;
	g_player_mdware_config.queue_player_msg = xQueueCreate(10, sizeof(char *));
	g_player_mdware_config.queue_record_msg = xQueueCreate(10, sizeof(char *));
	g_player_mdware_config.queue_play_tone_msg = xQueueCreate(10, sizeof(char *));

	if (xTaskCreate(task_player_middlerware,
        "task_player_middlerware",
        1024*2,
        NULL,
        4,
        NULL) != pdPASS) 
    {

        ESP_LOGE(TAG_PLAYER_MDWARE, "ERROR creating task_player_middlerware failed! Out of memory?");
    }

	if (xTaskCreate(task_record_audio,
        "task_record_audio",
        1024*2,
        NULL,
        4,
        NULL) != pdPASS) 
    {

        ESP_LOGE(TAG_PLAYER_MDWARE, "ERROR creating task_record_audio failed! Out of memory?");
    }

	if (xTaskCreate(task_play_tone,
        "task_play_tone",
        1024*5,
        NULL,
        4,
        NULL) != pdPASS) 
    {

        ESP_LOGE(TAG_PLAYER_MDWARE, "ERROR creating task_play_tonefailed! Out of memory?");
    }
}

