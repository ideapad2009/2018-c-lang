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
#include "device_api.h"
#include "socket.h"
#include "lwip/netdb.h"
#include "EspAudio.h"
#include "userconfig.h"
#include "keyboard_manage.h"
#include "keyboard_service.h"
#include "led_ctrl.h"
#include "player_middleware.h"
#include "db_vad.h"
#include "free_talk.h"
#include "flash_config_manage.h"
#include "asr_service.h"
#include "nlp_service.h"
#include "auto_play_service.h"
#include "geling_edu.h"
#include "translate_manage.h"
#include "low_power_manage.h"
#include "mcu_serial_comm.h"

#if MODULE_KEYWORD_WAKEUP
#include "keyword_wakeup.h"
#endif

//#define DEBUG_PRINT
#define LOG_TAG 				"free talk"

#define FREE_TALK_MAX_RECORD_TIME_MS	(10*1000)
#define FREE_TALK_MIN_RECORD_TIME_MS	(2000)
#define FREE_TALK_MAX_ASR_FAIL_COUNT 	3	//语音识别连续失败次数
#define FREE_TALK_MAX_ASR_NO_RET_COUNT 	5	//语音识别连续没有反馈结果次数
#define FREE_TALK_MAX_VAD_TIME			100	//自由对话录音是否启动

typedef enum
{
	FREE_TALK_MSG_RECORD_START,
	FREE_TALK_MSG_RECORD_READ,
	FREE_TALK_MSG_RECORD_STOP,
	FREE_TALK_MSG_QUIT,
	FREE_TALK_MSG_KEYWORD_WAKEUP,
}FREE_TALK_MSG_TYPE_T;

typedef enum
{
	FREE_TALK_REC_MODE_IDEL,
	FREE_TALK_REC_MODE_KEYWORD,			//唤醒词唤醒
	FREE_TALK_REC_MODE_VAD,				//静音检测唤醒
	FREE_TALK_REC_MODE_SPEECH_DETECT,	//检测说话
	FREE_TALK_REC_MODE_SPEECH_END,		//检测说话
	FREE_TALK_REC_MODE_STOP,			//停止
}FREE_TALK_REC_MODE_T;

//录音状态
typedef struct
{
	FREE_TALK_REC_MODE_T mode;
	int record_quiet_ms;
	int record_total_ms;
	int keyword_record_ms;
	int record_index;
}FREE_TALK_REC_STATUS_T;

typedef struct
{
	int data_len;
	char data[AUDIO_RECORD_BUFFER_SIZE];
}FREE_TALK_ENCODE_BUFF_T;

//自由对话状态
typedef struct
{
	xQueueHandle msg_queue;
	int record_sn;//录音id,用于标志录音是否有效
	int is_auto_start;
	FREE_TALK_STATE_T state;
	FREE_TALK_REC_STATUS_T rec_status;
	FREE_TALK_RUN_STATUS_T run_status;
	int cur_play_index;
	NLP_RESULT_T nlp_result;
	int asr_no_result_count;
	MediaService *service;
	void *vad_handle;
	void *wakeup_handle;
	FREE_TALK_TONE_STATE_T talk_tone_switch;
}FREE_TALK_HANDLE_T;

//录音数据
typedef struct 
{
	int record_sn;
	int record_id;
	int record_len;
	int record_ms;
	int is_max_ms;
	char data[AUDIO_RECORD_BUFFER_SIZE];
}FREE_TALK_RECORD_DATA_T;

//语音识别结果
typedef struct
{
	char asr_result[512];
}FREE_TALK_ASR_RESULT_T;

//自由对话消息
typedef struct
{
	FREE_TALK_MSG_TYPE_T msg_type;
	FREE_TALK_RECORD_DATA_T *p_record_data;
}FREE_TALK_MSG_T;

static FREE_TALK_HANDLE_T *g_free_talk_handle = NULL;
static xSemaphoreHandle g_talk_mode_mux = NULL;
static xSemaphoreHandle g_record_sn_mux = NULL;

void free_talk_nlp_result_cb(NLP_RESULT_T *result);

static void set_free_talk_rec_mode(int mode)
{
	if (g_free_talk_handle == NULL)
	{
		return;
	}
	
	xSemaphoreTake(g_talk_mode_mux, portMAX_DELAY);
	g_free_talk_handle->rec_status.mode = mode;
	xSemaphoreGive(g_talk_mode_mux);
}

static int get_free_talk_rec_mode(void)
{
	int mode = FREE_TALK_REC_MODE_IDEL;

	if (g_free_talk_handle == NULL)
	{
		return mode;
	}
	
	xSemaphoreTake(g_talk_mode_mux, portMAX_DELAY);
	mode = g_free_talk_handle->rec_status.mode;
	xSemaphoreGive(g_talk_mode_mux);
	
	return mode;
}


static void get_prev_music(AUTO_PLAY_MUSIC_SOURCE_T *src)
{
	if (g_free_talk_handle == NULL)
	{
		return;
	}

	NLP_RESULT_LINKS_T *p_links = &g_free_talk_handle->nlp_result.link_result;

	memset(src, 0, sizeof(AUTO_PLAY_MUSIC_SOURCE_T));
	if (p_links->link_size == 0)
	{
		return;
	}

	if (g_free_talk_handle->cur_play_index > 0)
	{
		g_free_talk_handle->cur_play_index--;
	}

	ESP_LOGI(LOG_TAG, "get_prev_music [%d][%s][%s]", 
		g_free_talk_handle->cur_play_index+1,
		p_links->link[g_free_talk_handle->cur_play_index].link_name,
		p_links->link[g_free_talk_handle->cur_play_index].link_url);
	
	src->need_play_name = 1;
	snprintf(src->music_name, sizeof(src->music_name), "%s", p_links->link[g_free_talk_handle->cur_play_index].link_name);
	snprintf(src->music_url, sizeof(src->music_url), "%s", p_links->link[g_free_talk_handle->cur_play_index].link_url);
}


static void get_next_music(AUTO_PLAY_MUSIC_SOURCE_T *src)
{
	if (g_free_talk_handle == NULL)
	{
		return;
	}

	NLP_RESULT_LINKS_T *p_links = &g_free_talk_handle->nlp_result.link_result;

	memset(src, 0, sizeof(AUTO_PLAY_MUSIC_SOURCE_T));
	if (p_links->link_size == 0)
	{
		return;
	}

	if (g_free_talk_handle->cur_play_index < p_links->link_size - 1)
	{
		g_free_talk_handle->cur_play_index++;
	}
	else if (g_free_talk_handle->cur_play_index == p_links->link_size - 1)
	{
		if (strlen(g_free_talk_handle->nlp_result.input_text) > 0 && p_links->link_size == NLP_SERVICE_LINK_MAX_NUM)
		{
			nlp_service_send_request(g_free_talk_handle->nlp_result.input_text, free_talk_nlp_result_cb);
		}
		return;
	}

	ESP_LOGI(LOG_TAG, "get_next_music [%d][%s][%s]", 
		g_free_talk_handle->cur_play_index+1,
		p_links->link[g_free_talk_handle->cur_play_index].link_name,
		p_links->link[g_free_talk_handle->cur_play_index].link_url);
	
	src->need_play_name = 1;
	snprintf(src->music_name, sizeof(src->music_name), "%s", p_links->link[g_free_talk_handle->cur_play_index].link_name);
	snprintf(src->music_url, sizeof(src->music_url), "%s", p_links->link[g_free_talk_handle->cur_play_index].link_url);
}

static void get_current_music(AUTO_PLAY_MUSIC_SOURCE_T *src)
{
	if (g_free_talk_handle == NULL)
	{
		return;
	}

	NLP_RESULT_LINKS_T *p_links = &g_free_talk_handle->nlp_result.link_result;

	memset(src, 0, sizeof(AUTO_PLAY_MUSIC_SOURCE_T));
	if (p_links->link_size == 0)
	{
		return;
	}

	ESP_LOGI(LOG_TAG, "get_current_music [%d][%s][%s]", 
		g_free_talk_handle->cur_play_index+1,
		p_links->link[g_free_talk_handle->cur_play_index].link_name,
		p_links->link[g_free_talk_handle->cur_play_index].link_url);
	
	src->need_play_name = 1;
	snprintf(src->music_name, sizeof(src->music_name), "%s", p_links->link[g_free_talk_handle->cur_play_index].link_name);
	snprintf(src->music_url, sizeof(src->music_url), "%s", p_links->link[g_free_talk_handle->cur_play_index].link_url);
}

void set_free_talk_record_data(
	int record_sn, int id, char *data, size_t data_len, int record_ms, int is_max)
{
	FREE_TALK_MSG_T msg = {0};

	if (g_free_talk_handle == NULL || g_free_talk_handle->msg_queue == NULL)
	{
		ESP_LOGE(LOG_TAG, "set_free_talk_record_data invalid parmas");
		return;
	}

	msg.p_record_data = (FREE_TALK_RECORD_DATA_T *)esp32_malloc(sizeof(FREE_TALK_RECORD_DATA_T));
	if (msg.p_record_data == NULL)
	{
		ESP_LOGE(LOG_TAG, "set_free_talk_record_data esp32_malloc failed");
		return;
	}

	memset(msg.p_record_data, 0, sizeof(FREE_TALK_RECORD_DATA_T));
	if (id == 1)
	{
		msg.msg_type = FREE_TALK_MSG_RECORD_START;
		if (g_free_talk_handle->service->_blocking == 1)
		{
			//led_ctrl_set_mode(LED_CTRL_EYE_FLASHING_SLOW);
		}
		else
		{
			//led_ctrl_set_mode(LED_CTRL_EYE_FLASHING_FAST);
		}
	}
	else if (id > 1)
	{
		msg.msg_type = FREE_TALK_MSG_RECORD_READ;
	}
	else
	{
		msg.msg_type = FREE_TALK_MSG_RECORD_STOP;
		if (g_free_talk_handle->service->_blocking == 0)
		{
			//led_ctrl_set_mode(LED_CTRL_EYE_LIGHT);
		}		
	}

	msg.p_record_data->record_sn = record_sn;
	msg.p_record_data->record_id = id;
	msg.p_record_data->record_len = data_len;
	msg.p_record_data->record_ms = record_ms;
	msg.p_record_data->is_max_ms = is_max;
	
	if (data_len > 0)
	{
		memcpy(msg.p_record_data->data, data, data_len);
	}

	if (xQueueSend(g_free_talk_handle->msg_queue, &msg, 0) != pdPASS)
	{
		ESP_LOGE(LOG_TAG, "xQueueSend g_free_talk_handle->msg_queue failed");
		esp32_free(msg.p_record_data);
		msg.p_record_data = NULL;
	}
}

static int free_talk_rec_sn_auto_increase(void)
{
	int sn = 0;
	
	if (g_free_talk_handle == NULL)
	{
		return 0;
	}
	
	xSemaphoreTake(g_record_sn_mux, portMAX_DELAY);
	sn = ++g_free_talk_handle->record_sn;
	xSemaphoreGive(g_record_sn_mux);

	return sn;
}

static int get_free_talk_rec_sn(void)
{
	int sn = 0;

	if (g_free_talk_handle == NULL)
	{
		return sn;
	}
	
	xSemaphoreTake(g_record_sn_mux, portMAX_DELAY);
	sn = g_free_talk_handle->record_sn;
	xSemaphoreGive(g_record_sn_mux);
	
	return sn;
}

static void free_talk_rec_start(void)
{
	int sn = free_talk_rec_sn_auto_increase();
	player_mdware_start_record(sn, FREE_TALK_MAX_RECORD_TIME_MS, set_free_talk_record_data);
}

static void free_talk_rec_stop(void)
{
	player_mdware_stop_record(set_free_talk_record_data);
}

static void free_talk_init_record_status(FREE_TALK_REC_STATUS_T *p_status)
{
	if (p_status == NULL)
	{
		return;
	}

	p_status->record_total_ms = 0;
	p_status->record_quiet_ms = 0;
	p_status->record_index = 0;
}

void free_talk_nlp_result_cb(NLP_RESULT_T *result)
{
	static int asr_no_result_count = 0;
	//static int asr_fail_count = 0;
	NLP_RESULT_LINKS_T *p_links = &result->link_result;
	AUTO_PLAY_CALL_BACK_T call_backs = {get_prev_music, get_next_music, get_current_music};

	if (g_free_talk_handle == NULL)
	{
		return;
	}

	if (get_free_talk_status() == FREE_TALK_RUN_STATUS_STOP)
	{
		return;
	}

	if (result->request_sn > 0 && result->request_sn != get_free_talk_rec_sn())
	{
		ESP_LOGE(LOG_TAG, "request_sn [%d]!=[%d]", get_free_talk_rec_sn(), result->request_sn);
		return;
	}
	set_talk_tone_state(FREE_TALK_TONE_STATE_DISABLE_PLAY);
	switch (result->type)
	{
		case NLP_RESULT_TYPE_NONE:
		case NLP_RESULT_TYPE_ERROR:
		{
			uart_esp32_status_mode(UART_ESP32_STATUS_MODE_CHAT_FAILED_STATU);//向MCU发送ESP32无语音输入或输入超时状态
			asr_no_result_count++;
			if (asr_no_result_count >= FREE_TALK_MAX_ASR_NO_RET_COUNT)
			{
				free_talk_autostart_disable();
				asr_no_result_count = 0;
				set_free_talk_status(FREE_TALK_RUN_STATUS_STOP);
			}
			else
			{
				free_talk_autostart_enable();
				player_mdware_play_tone(FLASH_MUSIC_NOT_HEAR_CLEARLY_PLEASE_REPEAT);
				set_talk_tone_state(FREE_TALK_TONE_STATE_ENABLE_PLAY);
			}
			break;
		}
		case NLP_RESULT_TYPE_NO_ASR:
		{
			uart_esp32_status_mode(UART_ESP32_STATUS_MODE_CHAT_FAILED_STATU);//向MCU发送ESP32无语音输入或输入超时状态
			asr_no_result_count++;
			if (asr_no_result_count >= FREE_TALK_MAX_ASR_NO_RET_COUNT)
			{
				free_talk_autostart_disable();
				asr_no_result_count = 0;
				set_free_talk_status(FREE_TALK_RUN_STATUS_STOP);
			}
			else
			{
				free_talk_rec_start();
			}
			break;
		}
		case NLP_RESULT_TYPE_CHAT:
		{
			asr_no_result_count = 0;
			//play tts
			free_talk_autostart_enable();
			auto_play_pause();
			if (strcmp(&result->input_text, "当前版本") == 0)
			{
				play_tts(ESP32_FW_VERSION);
			}
			else if (strcmp(&result->input_text, "萌萌") == 0)
			{
				uart_esp32_status_mode(UART_ESP32_STATUS_MODE_WAKE_UP);//向MCU发送ESP32唤醒状态
				//free_talk_autostart_enable();
				play_tts("你好");
			}
			else if (strlen(result->chat_result.link) > 0)
			{
				player_mdware_play_audio(result->chat_result.link);
			}
			else
			{
				play_tts(result->chat_result.text);
			}
			set_talk_tone_state(FREE_TALK_TONE_STATE_ENABLE_PLAY);
			break;
		}
		case NLP_RESULT_TYPE_LINK:
		{	
			asr_no_result_count = 0;
			if (p_links->link_size > 0)
			{
				free_talk_autostart_disable();
				set_free_talk_status(FREE_TALK_RUN_STATUS_STOP);
				g_free_talk_handle->nlp_result = *result;
				g_free_talk_handle->cur_play_index = 0;
				set_auto_play_cb(&call_backs);
			}
			else
			{
				set_talk_tone_state(FREE_TALK_TONE_STATE_ENABLE_PLAY);
				free_talk_autostart_enable();
				player_mdware_play_tone(FLASH_MUSIC_NO_MUSIC);
			}
			break;
		}
		case NLP_RESULT_TYPE_CMD:
		{
			asr_no_result_count = 0;
			set_talk_tone_state(FREE_TALK_TONE_STATE_ENABLE_PLAY);
			free_talk_autostart_enable();
			break;
		}
		default:
			break;
	}
}

static void free_talk_asr_result_cb(ASR_RESULT_T *result)
{
	static int asr_no_result_count = 0;
	static int asr_fail_count = 0;

	if (get_free_talk_status() == FREE_TALK_RUN_STATUS_STOP)
	{
		return;
	}

	if (result->record_sn != get_free_talk_rec_sn())
	{
		ESP_LOGE(LOG_TAG, "record_sn [%d]!=[%d]", get_free_talk_rec_sn(), result->record_sn);
		return;
	}
	
	switch (result->type)
	{
		case ASR_RESULT_TYPE_SHORT_AUDIO:
		{
			//语音太短，继续对话
			uart_esp32_status_mode(UART_ESP32_STATUS_MODE_CHAT_FAILED_STATU);//向MCU发送ESP32无语音输入或输入超时状态
			set_talk_tone_state(FREE_TALK_TONE_STATE_DISABLE_PLAY);
			free_talk_rec_start();
			break;
		}
		case ASR_RESULT_TYPE_SUCCESS:
		{
			set_talk_tone_state(FREE_TALK_TONE_STATE_ENABLE_PLAY);
			free_talk_autostart_enable();
			
			nlp_service_send_translate_request(result->asr_text, free_talk_nlp_result_cb);
			asr_fail_count = 0;
			asr_no_result_count = 0;
			break;
		}
		case ASR_RESULT_TYPE_FAIL:
		{
			uart_esp32_status_mode(UART_ESP32_STATUS_MODE_CHAT_FAILED_STATU);//向MCU发送ESP32无语音输入或输入超时状态
			set_talk_tone_state(FREE_TALK_TONE_STATE_DISABLE_PLAY);
			asr_fail_count++;
			if (asr_fail_count >= FREE_TALK_MAX_ASR_FAIL_COUNT)
			{
				ESP_LOGE(LOG_TAG, "free_talk_asr_result_cb asr fail count[%d]", asr_fail_count);
				free_talk_autostart_disable();
				ctrl_send_standby_state();
				player_mdware_play_tone(FLASH_MUSIC_POOR_NETWORK_SIGNAL);
				asr_fail_count = 0;
				set_free_talk_status(FREE_TALK_RUN_STATUS_STOP);
				//led_ctrl_set_mode(LED_CTRL_EYE_LIGHT);
			}
			else
			{
				free_talk_rec_start();
			}
			break;
		}
		case ASR_RESULT_TYPE_NO_RESULT:
		{
			uart_esp32_status_mode(UART_ESP32_STATUS_MODE_CHAT_FAILED_STATU);//向MCU发送ESP32无语音输入或输入超时状态
			set_talk_tone_state(FREE_TALK_TONE_STATE_DISABLE_PLAY);
			asr_no_result_count++;
			if (asr_no_result_count >= FREE_TALK_MAX_ASR_NO_RET_COUNT)
			{
				free_talk_autostart_disable();
				asr_no_result_count = 0;
				ctrl_send_standby_state();
				set_free_talk_status(FREE_TALK_RUN_STATUS_STOP);
				//led_ctrl_set_mode(LED_CTRL_EYE_LIGHT);
			}
			else
			{
				free_talk_rec_start();
			}
			break;
		}
		default:
			break;
	}
	
	return;
}

static void set_free_talk_asr_record_data(
	FREE_TALK_HANDLE_T *free_talk_handle, FREE_TALK_RECORD_DATA_T *p_record_data, int is_last)
{
	static int last_flag = 0;
	ASR_RECORD_MSG_T *p_msg = NULL;

	if (p_record_data->record_id == 1)
	{
		last_flag = 0;
	}

	if (last_flag == 1)
	{
		return;
	}

	p_msg = (ASR_RECORD_MSG_T *)esp32_malloc(sizeof(ASR_RECORD_MSG_T));
	if (p_msg == NULL)
	{
		ESP_LOGE(LOG_TAG, "set_free_talk_asr_record_data esp32_malloc failed");
		return;
	}

	memset(p_msg, 0, sizeof(ASR_RECORD_MSG_T));
	
	free_talk_handle->rec_status.record_index++;
	//计算录音时间
	if (free_talk_handle->rec_status.record_index == 1)
	{
		free_talk_handle->rec_status.record_total_ms = p_record_data->record_len/AUDIO_PCM_ONE_MS_BYTE;
	}
	else
	{
		free_talk_handle->rec_status.record_total_ms += p_record_data->record_len/AUDIO_PCM_ONE_MS_BYTE;
	}
	
	//计算录音状态
	if (free_talk_handle->rec_status.record_index == 1)
	{
		p_msg->msg_type = ASR_SERVICE_RECORD_START;
	}
	else if (free_talk_handle->rec_status.record_index > 1)
	{
		p_msg->msg_type = ASR_SERVICE_RECORD_READ;
	}
	
	//最后一条数据
	if (is_last)
	{
		p_msg->msg_type = ASR_SERVICE_RECORD_STOP;
		last_flag = 1;
	}

	p_msg->record_data.record_sn = p_record_data->record_sn;
	p_msg->record_data.record_id = free_talk_handle->rec_status.record_index;
	p_msg->record_data.record_len = p_record_data->record_len;
	p_msg->record_data.record_ms = free_talk_handle->rec_status.record_total_ms;
	p_msg->record_data.is_max_ms = p_record_data->is_max_ms;
	get_flash_cfg(FLASH_CFG_ASR_MODE, &p_msg->record_data.asr_engine);
	p_msg->record_data.time_stamp 	= get_time_of_day();
	switch (free_talk_get_state())
	{
		case FREE_TALK_STATE_NORMAL:
		{
			p_msg->record_data.language_type = ASR_LANGUAGE_TYPE_CHINESE;
			p_msg->nlp_call_back = free_talk_nlp_result_cb;
			break;
		}
		case FREE_TALK_STATE_TRANSLATE_CH_EN:
		{
			p_msg->record_data.language_type = ASR_LANGUAGE_TYPE_CHINESE;
			p_msg->asr_call_back = free_talk_asr_result_cb;
			break;
		}
		case FREE_TALK_STATE_TRANSLATE_EN_CH:
		{
			p_msg->record_data.language_type = ASR_LANGUAGE_TYPE_ENGLISH;
			p_msg->asr_call_back = free_talk_asr_result_cb;
			break;
		}
		default:
			break;
	}	
	
	if (p_record_data->record_len > 0)
	{
		memcpy(p_msg->record_data.record_data, p_record_data->data, p_record_data->record_len);
	}

	if (asr_service_send_request(p_msg) == ESP_FAIL)
	{
		ESP_LOGE(LOG_TAG, "set_wechat_record_data asr_service_send_request failed");
		esp32_free(p_msg);
		p_msg = NULL;
	}
}

static void free_talk_record_process(
	FREE_TALK_HANDLE_T *free_talk_handle, FREE_TALK_MSG_T *msg)
{
	int ret = 0;
	
	switch (msg->msg_type)
	{
		case FREE_TALK_MSG_RECORD_START:
		{
			update_device_sleep_time();
			free_talk_autostart_disable();
			if (get_talk_tone_state() == FREE_TALK_TONE_STATE_ENABLE_PLAY)
			{
				player_mdware_play_tone(FLASH_MUSIC_KEY_PRESS);
				vTaskDelay(300);
			}
			free_talk_init_record_status(&free_talk_handle->rec_status);
			set_free_talk_rec_mode(FREE_TALK_REC_MODE_SPEECH_DETECT);
		}
		case FREE_TALK_MSG_RECORD_READ:
		{
			int frame_len = msg->p_record_data->record_len;
			int frame_start = 0;

			switch (get_free_talk_rec_mode())
			{
				case FREE_TALK_REC_MODE_SPEECH_DETECT:
				{
					//静音检测,是否结束说话		
					while (frame_len >= AUDIO_PCM_10MS_FRAME_SIZE)
					{
						ret = DB_Vad_Process(free_talk_handle->vad_handle, AUDIO_PCM_RATE, AUDIO_PCM_10MS_FRAME_SIZE/2, (int16_t*)(msg->p_record_data->data + frame_start));
#ifdef DEBUG_PRINT
						DEBUG_LOGE(LOG_TAG, "vad[%d]", ret);								
#endif
						if (ret == 1)
						{					
							free_talk_handle->rec_status.record_quiet_ms = 0;
						}
						else
						{
							free_talk_handle->rec_status.record_quiet_ms += 10;
						}
						frame_len -= AUDIO_PCM_10MS_FRAME_SIZE;
						frame_start += AUDIO_PCM_10MS_FRAME_SIZE;

						if (free_talk_handle->rec_status.record_quiet_ms >= FREE_TALK_MAX_VAD_TIME 
							&& free_talk_handle->rec_status.record_total_ms >= FREE_TALK_MIN_RECORD_TIME_MS)
						{
							set_free_talk_rec_mode(FREE_TALK_REC_MODE_SPEECH_END);
							free_talk_rec_stop();
							break;
						}
					}

					if (get_free_talk_rec_mode() == FREE_TALK_REC_MODE_SPEECH_END)
					{
						set_free_talk_asr_record_data(free_talk_handle, msg->p_record_data, 1); 
					}
					else
					{
						set_free_talk_asr_record_data(free_talk_handle, msg->p_record_data, 0);	
					}
					break;
				}
				case FREE_TALK_REC_MODE_SPEECH_END:
				{
					set_free_talk_asr_record_data(free_talk_handle, msg->p_record_data, 0);	
					break;
				}
				default:
					break;
			}
#ifdef DEBUG_PRINT
			DEBUG_LOGE(LOG_TAG, "record_total_ms=%d, record_quiet_ms=%d", 
				free_talk_handle->rec_status.record_total_ms, free_talk_handle->rec_status.record_quiet_ms);
#endif					
			break;
		}
		case FREE_TALK_MSG_RECORD_STOP:
		{
			set_free_talk_asr_record_data(free_talk_handle, msg->p_record_data, 1);	
			break;
		}
		case FREE_TALK_MSG_KEYWORD_WAKEUP: //free_talk开启状态下语音唤醒会进入此内
		{
			if (free_talk_handle->service->_blocking == 1)
			{
				player_mdware_play_tone(FLASH_MUSIC_NETWORK_CONNECT_BEFORE_USE);
				break;
			}

			free_talk_rec_sn_auto_increase();//如果正在录音，使当前录音结果失效
			free_talk_set_state(FREE_TALK_STATE_NORMAL);
			PlayerStatus player_status;
			EspAudioStatusGet(&player_status);
			
			if (player_status.status == AUDIO_STATUS_PLAYING)
			{
				free_talk_autostart_disable();
				auto_play_stop();
				vTaskDelay(100);
			}
			uart_esp32_status_mode(UART_ESP32_STATUS_MODE_WAKE_UP);//向MCU发送ESP32唤醒状态
			free_talk_tone();//使唤醒后每次的提示音不同
			set_talk_tone_state(FREE_TALK_TONE_STATE_DISABLE_PLAY);//不允许录音时播放“嘟”声
			free_talk_start();
			break;
		}
		default:
			break;
	}
}

static void task_free_talk(void *pv)
{
	int ret = 0;
	int i = 0;
	FREE_TALK_HANDLE_T *free_talk_handle = (FREE_TALK_HANDLE_T *)pv;
	MediaService *service = free_talk_handle->service;
	FREE_TALK_MSG_T msg = {0};

	free_talk_handle->vad_handle = DB_Vad_Create();
	if (free_talk_handle->vad_handle == NULL)
	{
		DEBUG_LOGE(LOG_TAG, "task_free_talk DB_Vad_Create failed");
		return;
	}

	while (1) 
	{		
		if (xQueueReceive(free_talk_handle->msg_queue, &msg, portMAX_DELAY) == pdPASS) 
		{			
#ifdef DEBUG_PRINT
			if (msg.p_record_data != NULL)
			{
				DEBUG_LOGE(LOG_TAG, "id=%d, len=%d, time=%dms", 
					msg.p_record_data->record_id, msg.p_record_data->record_len, msg.p_record_data->record_ms);
			}
#endif		
			switch (get_free_talk_status())
			{
				case FREE_TALK_RUN_STATUS_START:
				{
					free_talk_record_process(free_talk_handle, &msg);
					break;
				}
				case FREE_TALK_RUN_STATUS_STOP:
				{
#if MODULE_KEYWORD_WAKEUP
					if (msg.msg_type == FREE_TALK_MSG_KEYWORD_WAKEUP)
					{
						if (service->_blocking == 1)
						{
							player_mdware_play_tone(FLASH_MUSIC_NETWORK_CONNECT_BEFORE_USE);
							break;
						}
						
						PlayerStatus player_status;
						EspAudioStatusGet(&player_status);
						free_talk_autostart_disable();
						auto_play_stop();
						vTaskDelay(100);
						uart_esp32_status_mode(UART_ESP32_STATUS_MODE_WAKE_UP);//向MCU发送ESP32唤醒状态
						free_talk_set_state(FREE_TALK_STATE_NORMAL);//设置为对话模式
						if (player_status.status == AUDIO_STATUS_PLAYING)
						{
							free_talk_tone();//使唤醒后每次的提示音不同
						}
						else
						{
							free_talk_tone();//使唤醒后每次的提示音不同
						}
						free_talk_start();
					}
#endif
					break;
				}
				default:
					break;
			}

			if (msg.msg_type == FREE_TALK_MSG_QUIT)
			{
				if (msg.p_record_data != NULL)
				{
					esp32_free(msg.p_record_data);
					msg.p_record_data = NULL;
				}
				break;
			}

			if (msg.p_record_data != NULL)
			{
				esp32_free(msg.p_record_data);
				msg.p_record_data = NULL;
			}
		}
	}
	
	DB_Vad_Free(free_talk_handle->vad_handle);
	free_talk_handle->vad_handle = NULL;
	vTaskDelete(NULL);
}

#if MODULE_KEYWORD_WAKEUP
void keyword_wakeup_event(KWW_EVENT_T event)
{
	FREE_TALK_MSG_T msg = {0};
	msg.msg_type = FREE_TALK_MSG_KEYWORD_WAKEUP;
	
	if (g_free_talk_handle != NULL && g_free_talk_handle->msg_queue != NULL)
	{
		xQueueSend(g_free_talk_handle->msg_queue, &msg, 0);
	}
}
#endif

void free_talk_create(MediaService *self)
{
	g_free_talk_handle = (FREE_TALK_HANDLE_T *)esp32_malloc(sizeof(FREE_TALK_HANDLE_T));
	if (g_free_talk_handle == NULL)
	{
		return;
	}
	memset(g_free_talk_handle, 0, sizeof(FREE_TALK_HANDLE_T));
	
	g_free_talk_handle->talk_tone_switch = FREE_TALK_TONE_STATE_INIT;
	g_free_talk_handle->service = self;
	g_free_talk_handle->msg_queue = xQueueCreate(20, sizeof(FREE_TALK_MSG_T));
	g_talk_mode_mux = xSemaphoreCreateMutex();
	g_record_sn_mux = xSemaphoreCreateMutex();
	g_free_talk_handle->is_auto_start = 0;
#if MODULE_KEYWORD_WAKEUP
	keyword_wakeup_register_callback(keyword_wakeup_event);
#endif
	if (xTaskCreate(task_free_talk,
					"task_free_talk",
					512*5,
					g_free_talk_handle,
					4,
					NULL) != pdPASS) {

		ESP_LOGE(LOG_TAG, "ERROR creating task_free_talk task! Out of memory?");
	}
}


void free_talk_delete(void)
{
	FREE_TALK_MSG_T msg = {0};
	msg.msg_type = FREE_TALK_MSG_QUIT;
	
	if (g_free_talk_handle != NULL && g_free_talk_handle->msg_queue != NULL)
	{
		xQueueSend(g_free_talk_handle->msg_queue, &msg, 0);
	}
}


void free_talk_set_state(int state)
{
	if (g_free_talk_handle == NULL)
	{
		return;
	}
	
	xSemaphoreTake(g_talk_mode_mux, portMAX_DELAY);
	g_free_talk_handle->state = state;
	xSemaphoreGive(g_talk_mode_mux);
}

int free_talk_get_state(void)
{
	int state = -1;

	if (g_free_talk_handle == NULL)
	{
		return;
	}
	
	xSemaphoreTake(g_talk_mode_mux, portMAX_DELAY);
	state = g_free_talk_handle->state;
	xSemaphoreGive(g_talk_mode_mux);
	
	return state;
}

void free_talk_start(void)
{
	set_free_talk_status(FREE_TALK_RUN_STATUS_START);
	free_talk_rec_start();
}

void free_talk_stop(void)
{
	free_talk_autostart_disable();
	free_talk_rec_stop();
	set_free_talk_status(FREE_TALK_RUN_STATUS_STOP);
}

int is_free_talk_running(void)
{
	if (get_free_talk_status() == FREE_TALK_RUN_STATUS_START)
	{
		return 1;
	}
	else
	{
		return 0;
	}
}

//获取free talk 运行状态
int get_free_talk_status(void)
{
	int status = 0;
	
	if (g_free_talk_handle == NULL)
	{
		return;
	}
	
	xSemaphoreTake(g_talk_mode_mux, portMAX_DELAY);
	status = g_free_talk_handle->run_status;
	xSemaphoreGive(g_talk_mode_mux);

	return status;
}

//设置 free talk 运行状态
void set_free_talk_status(int status)
{
	if (g_free_talk_handle == NULL)
	{
		return;
	}
	
	xSemaphoreTake(g_talk_mode_mux, portMAX_DELAY);
	g_free_talk_handle->run_status = status;
	xSemaphoreGive(g_talk_mode_mux);
}

void free_talk_autostart_disable(void)
{
	if (g_free_talk_handle == NULL)
	{
		return;
	}
	
	xSemaphoreTake(g_talk_mode_mux, portMAX_DELAY);
	g_free_talk_handle->is_auto_start = 0;
	xSemaphoreGive(g_talk_mode_mux);
}

void free_talk_autostart_enable(void)
{
	if (g_free_talk_handle == NULL)
	{
		return;
	}
	
	xSemaphoreTake(g_talk_mode_mux, portMAX_DELAY);
	g_free_talk_handle->is_auto_start = 1;
	xSemaphoreGive(g_talk_mode_mux);
}

void free_talk_auto_start(void)
{
	if (g_free_talk_handle == NULL)
	{
		return;
	}
	
	xSemaphoreTake(g_talk_mode_mux, portMAX_DELAY);
	if (g_free_talk_handle->is_auto_start == 1)
	{
		free_talk_rec_start();
	}
	xSemaphoreGive(g_talk_mode_mux);
}

void free_talk_auto_stop(void)
{
	if (g_free_talk_handle == NULL)
	{
		return;
	}
	
	xSemaphoreTake(g_talk_mode_mux, portMAX_DELAY);
	if (g_free_talk_handle->is_auto_start == 1)
	{
		free_talk_rec_stop();
	}
	xSemaphoreGive(g_talk_mode_mux);
}

//对话前提示音是否播放接口，state为播放选项
void set_talk_tone_state(int state)
{
	if (g_free_talk_handle == NULL)
	{
		return;
	}
	
	xSemaphoreTake(g_talk_mode_mux, portMAX_DELAY);
	g_free_talk_handle->talk_tone_switch = state;
	xSemaphoreGive(g_talk_mode_mux);
}

int get_talk_tone_state()
{
	int state = 0;
	if (g_free_talk_handle == NULL)
	{
		return;
	}
	
	xSemaphoreTake(g_talk_mode_mux, portMAX_DELAY);
	state = g_free_talk_handle->talk_tone_switch;
	xSemaphoreGive(g_talk_mode_mux);
	
	return state;
}

void free_talk_tone()
{
	static FREE_TALK_TONE_NUM_T tone_num = FREE_TALK_TONE_NUM_ONE;
	
	switch (tone_num)
	{
		case FREE_TALK_TONE_NUM_ONE:
		{
			player_mdware_play_tone(FLASH_MUSIC_HELLO_LITTLE_FRIEND);
			tone_num++;
			break;
		}
		case FREE_TALK_TONE_NUM_TWO:
		{
			player_mdware_play_tone(FLASH_MUSIC_WHY_DO_U_LOOK_FOR_ME);
			tone_num++;
			break;
		}
		case FREE_TALK_TONE_NUM_THREE:
		{
			player_mdware_play_tone(FLASH_MUSIC_I_AM_HERE_YOU_SAY);
			tone_num = FREE_TALK_TONE_NUM_ONE;
			break;
		}
		default:
		{
			player_mdware_play_tone(FLASH_MUSIC_I_AM_HERE_YOU_SAY);
			tone_num = FREE_TALK_TONE_NUM_ONE;
			break;
		}
	}
	vTaskDelay(1300);
}
