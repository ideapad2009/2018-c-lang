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
#include "led_ctrl.h"
#include "player_middleware.h"
#include "interf_enc.h"
#include "mpush_service.h"
#include "bind_device.h"
#include "speech_wakeup.h"
#include "db_vad.h"
#include "free_talk.h"
#include "ReSampling.h"
#include "nlp_service.h"
#include "auto_play_service.h"
#include "amrwb_encode.h"
#include "functional_running_state_check.h"

//#define DEBUG_PRINT
#define LOG_TAG "free talk"

#define FREE_TALK_MAX_RECORD_TIME_MS	(24*60*60*1000)
#define FREE_TALK_MAX_ASR_FAIL_COUNT 3	//语音识别连续失败次数
#define FREE_TALK_MAX_ASR_NO_RET_COUNT 5//语音识别连续没有反馈结果次数

typedef enum
{
	FREE_TALK_MSG_RECORD_START,
	FREE_TALK_MSG_RECORD_READ,
	FREE_TALK_MSG_RECORD_STOP,
	FREE_TALK_MSG_QUIT,
}FREE_TALK_MSG_TYPE_T;

typedef struct
{
	int record_quiet_ms;
	int record_total_ms;
	int keyword_record_ms;
	int record_index;
}FREE_TALK_STATUS_T;

typedef struct
{
	int data_len;
	char data[AUDIO_RECORD_BUFFER_SIZE];
}FREE_TALK_ENCODE_BUFF_T;
	
typedef struct
{
	xQueueHandle msg_queue;
	int is_auto_start;
	FREE_TALK_MODE_T mode;
	FREE_TALK_STATE_T state;
	FREE_TALK_STATUS_T status;
	int cur_play_index;
	NLP_RESULT_T nlp_result;
	int asr_no_result_count;
	MediaService *service;
	void *vad_handle;
	void *wakeup_handle;
	FREE_TALK_STATE_FLAG_T free_talk_state_flag;	//连续对话状态标记
}FREE_TALK_HANDLE_T;

typedef struct 
{
	int record_id;
	int record_len;
	int record_ms;
	int is_max_ms;
	char data[AUDIO_RECORD_BUFFER_SIZE];
}FREE_TALK_RECORD_DATA_T;

typedef struct
{
	char asr_result[512];
}FREE_TALK_ASR_RESULT_T;

typedef struct
{
	FREE_TALK_MSG_TYPE_T msg_type;
	FREE_TALK_RECORD_DATA_T *p_record_data;
}FREE_TALK_MSG_T;

static FREE_TALK_HANDLE_T *g_free_talk_handle = NULL;
static xSemaphoreHandle g_talk_mode_mux = NULL;
static FREE_TALK_STATE_FLAG_T g_free_talk_flag = FREE_TALK_STATE_FLAG_INIT_STATE;

extern void free_talk_nlp_result_cb(NLP_RESULT_T *result);

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

void free_talk_nlp_result_cb(NLP_RESULT_T *result)
{
	NLP_RESULT_LINKS_T *p_links = &result->link_result;
	AUTO_PLAY_CALL_BACK_T call_backs = {get_prev_music, get_next_music, get_current_music};

	if (g_free_talk_handle == NULL)
	{
		return;
	}
	
	switch (result->type)
	{
		case NLP_RESULT_TYPE_NONE:
		{
			color_led_flicker_stop();
			color_led_on();
			player_mdware_play_tone(FLASH_MUSIC_B_24_NOT_HEAR_CLEARLY_PLEASE_REPEAT);
			vTaskDelay(3*1000);
			set_free_talk_resume();//启动连续对话
			break;
		}
		case NLP_RESULT_TYPE_CHAT:
		{
			//play tts
			color_led_flicker_stop();
			color_led_on();
			auto_play_pause();
			ESP_LOGE(LOG_TAG, "chat_result=[%s]\r\n", result->chat_result.text);
			set_free_talk_resume();//启动连续对话
			play_tts(result->chat_result.text);
			break;
		}
		case NLP_RESULT_TYPE_LINK:
		{
			if (p_links->link_size > 0)
			{
				color_led_flicker_stop();
				color_led_on();
				set_free_talk_pause();
//				free_talk_set_mode(FREE_TALK_MODE_KEYWORD_WAKEUP);
				free_talk_set_mode(FREE_TALK_MODE_STOP);
				set_free_talk_flag(FREE_TALK_STATE_FLAG_STOP_STATE);
				free_talk_stop();
				g_free_talk_handle->nlp_result = *result;
				g_free_talk_handle->cur_play_index = 0;
				set_auto_play_cb(&call_backs);
			}
			else
			{
				set_free_talk_resume();
				player_mdware_play_tone(FLASH_MUSIC_DYY_NO_MUSIC_PLAY);
			}
			break;
		}
		case NLP_RESULT_TYPE_CMD:
		{
			break;
		}
		default:
			break;
	}
}


static void free_talk_init_record_status(FREE_TALK_STATUS_T *p_status)
{
	if (p_status == NULL)
	{
		return;
	}

	p_status->record_total_ms = 0;
	p_status->record_quiet_ms = 0;
	p_status->record_index = 0;
}

static void free_talk_asr_result_cb(ASR_RESULT_T *result)
{
	static int asr_no_result_count = 0;
	static int asr_fail_count = 0;
		
	switch (result->type)
	{
		case ASR_RESULT_TYPE_SHORT_AUDIO:
		{
			//语音太短，继续对话
			set_free_talk_flag(FREE_TALK_STATE_FLAG_START_STATE);
			free_talk_start();
			break;
		}
		case ASR_RESULT_TYPE_SUCCESS:
		{
			if (strcmp(result->asr_text, "当前版本") == 0)
			{
				play_tts(ESP32_FW_VERSION);
			}
			else
			{
				//普通对话
				nlp_service_send_request(result->asr_text, free_talk_nlp_result_cb);
			}
			asr_fail_count = 0;
			asr_no_result_count = 0;
			free_talk_set_mode(FREE_TALK_MODE_VAD_WAKEUP);
			free_talk_start();
			break;
		}
		case ASR_RESULT_TYPE_FAIL:
		{
			asr_fail_count++;
			if (asr_fail_count >= FREE_TALK_MAX_ASR_FAIL_COUNT)
			{
				ESP_LOGE(LOG_TAG, "free_talk_asr_result_cb asr fail count[%d]", asr_fail_count);
				color_led_on();
				free_talk_set_mode(FREE_TALK_MODE_VAD_WAKEUP);
				//player_mdware_play_tone(FLASH_MUSIC_B_21_ERROR_PLEASE_TRY_AGAIN_LATER);
				//vTaskDelay(3000);
				//set_free_talk_resume();//启动连续对话
				asr_fail_count = 0;
			}
			free_talk_set_mode(FREE_TALK_MODE_VAD_WAKEUP);
			free_talk_start();
			//set_free_talk_resume();//启动连续对话
			break;
		}
		case ASR_RESULT_TYPE_NO_RESULT:
		{
			asr_no_result_count++;
			if (asr_no_result_count >= FREE_TALK_MAX_ASR_NO_RET_COUNT)
			{//超过次数，退出对话
				asr_no_result_count = 0;				
				set_free_talk_pause();
				set_functional_tyigger_key_state(FUNCTIONAL_TYIGGER_KEY_INITIAL_STATE);
				free_talk_set_mode(FREE_TALK_MODE_STOP);
				set_free_talk_flag(FREE_TALK_STATE_FLAG_STOP_STATE);
				free_talk_stop();
				color_led_flicker_stop();
				color_led_off();
				player_mdware_play_tone(FLASH_MUSIC_B_28_SHUT_DOWN_CONTINUOUS_CHAT);
				vTaskDelay(3000);
			}
			else
			{
				free_talk_set_mode(FREE_TALK_MODE_VAD_WAKEUP);				
				free_talk_start();
			}

			break;
		}
		default:
			break;
	}
	
	return;
}

void set_free_talk_record_data(
	int id, char *data, size_t data_len, int record_ms, int is_max)
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
		set_free_talk_pause();
		msg.msg_type = FREE_TALK_MSG_RECORD_START;
//		if (g_free_talk_handle->service->_blocking == 1)
//		{
//			led_ctrl_set_mode(LED_CTRL_EYE_FLASHING_SLOW);
//		}
//		else
//		{
//			if (free_talk_get_mode() == FREE_TALK_MODE_KEYWORD_WAKEUP)
//			{
//				led_ctrl_set_mode(LED_CTRL_EYE_LIGHT);
//			}
//			else
//			{
//				led_ctrl_set_mode(LED_CTRL_EYE_FLASHING_FAST);
//			}
//		}
	}
	else if (id > 1)
	{
		msg.msg_type = FREE_TALK_MSG_RECORD_READ;
	}
	else
	{
		msg.msg_type = FREE_TALK_MSG_RECORD_STOP;
//		if (g_free_talk_handle->service->_blocking == 0)
//		{
//			led_ctrl_set_mode(LED_CTRL_EYE_LIGHT);
//		}		
	}
	
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

static void set_free_talk_asr_record_data(
	FREE_TALK_HANDLE_T *free_talk_handle, FREE_TALK_RECORD_DATA_T *p_record_data, int is_last)
{
	ASR_RECORD_MSG_T *p_msg = NULL;

	p_msg = (ASR_RECORD_MSG_T *)esp32_malloc(sizeof(ASR_RECORD_MSG_T));
	if (p_msg == NULL)
	{
		ESP_LOGE(LOG_TAG, "set_wechat_record_data esp32_malloc failed");
		return;
	}

	memset(p_msg, 0, sizeof(ASR_RECORD_MSG_T));
	
	free_talk_handle->status.record_index++;
	//计算录音时间
	if (free_talk_handle->status.record_index == 1)
	{
		free_talk_handle->status.record_total_ms = p_record_data->record_len/AUDIO_PCM_ONE_MS_BYTE;
	}
	else
	{
		free_talk_handle->status.record_total_ms += p_record_data->record_len/AUDIO_PCM_ONE_MS_BYTE;
	}
	//计算录音状态
	if (free_talk_handle->status.record_index == 1)
	{
		p_msg->msg_type = ASR_SERVICE_RECORD_START;
	}
	else if (free_talk_handle->status.record_index > 1)
	{
		p_msg->msg_type = ASR_SERVICE_RECORD_READ;
	}
	//最后一条数据
	if (is_last)
	{
		p_msg->msg_type = ASR_SERVICE_RECORD_STOP;
	}
	
	p_msg->record_data.record_id = free_talk_handle->status.record_index;
	p_msg->record_data.record_len = p_record_data->record_len;
	p_msg->record_data.record_ms = free_talk_handle->status.record_total_ms;
	p_msg->record_data.is_max_ms = p_record_data->is_max_ms;
	p_msg->record_data.record_type = ASR_RECORD_TYPE_AMRWB;	//	百度
	//p_msg->record_data.record_type = ASR_RECORD_TYPE_PCM_UNISOUND_16K; //云之声
	//p_msg->record_data.record_type = ASR_RECORD_TYPE_SPXWB; //讯飞
	p_msg->call_back = free_talk_asr_result_cb;
	
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

static void task_free_talk(void *pv)
{
	int ret = 0;
	int i = 0;
	FREE_TALK_HANDLE_T *free_talk_handle = (FREE_TALK_HANDLE_T *)pv;
	MediaService *service = free_talk_handle->service;
	FREE_TALK_MSG_T msg = {0};

	//有网络或者8秒超时退出循环
	uint64_t start_time = get_time_of_day();
	while (1)
	{
		if (service->_blocking == 0)
		{
			vTaskDelay(1000/portTICK_RATE_MS);
			break;
		}

		if (abs(get_time_of_day() - start_time) >= 8*1000)
		{
			break;
		}
		vTaskDelay(500/portTICK_RATE_MS);
	}

	while (1) 
	{		
		if (xQueueReceive(free_talk_handle->msg_queue, &msg, portMAX_DELAY) == pdPASS) 
		{			
#ifdef DEBUG_PRINT
			if (msg.p_record_data != NULL)
			{
				ESP_LOGI(LOG_TAG, "id=%d, len=%d, time=%dms", 
				msg.p_record_data->record_id, msg.p_record_data->record_len, msg.p_record_data->record_ms);
			}
#endif			
			switch (msg.msg_type)
			{
				case FREE_TALK_MSG_RECORD_START:
				{
					free_talk_init_record_status(&free_talk_handle->status);
					free_talk_handle->status.keyword_record_ms = 0;
					
					if (free_talk_handle->vad_handle != NULL)
					{						
						DB_Vad_Free(free_talk_handle->vad_handle);
						free_talk_handle->vad_handle = NULL;
					}
					free_talk_handle->vad_handle = DB_Vad_Create();
					color_led_flicker_start();
				}
				case FREE_TALK_MSG_RECORD_READ:
				{
#ifdef DEBUG_PRINT
					printf("free_talk 2 ! RECORD_READ\n");
#endif
					int frame_len = msg.p_record_data->record_len;
					int frame_start = 0;
					
					switch (free_talk_get_mode())
					{
						case FREE_TALK_MODE_VAD_WAKEUP:
						{
#ifdef DEBUG_PRINT
							printf("free_talk 3 ! VAD_WAKEUP\n");
#endif
							//静音检测，是否有人说话 
							while (frame_len >= AUDIO_PCM_10MS_FRAME_SIZE)
							{
								ret = DB_Vad_Process(free_talk_handle->vad_handle, AUDIO_PCM_RATE, AUDIO_PCM_10MS_FRAME_SIZE/2, (int16_t*)(msg.p_record_data->data + frame_start));
								if (ret == 1)
								{
									ESP_LOGE(LOG_TAG, "vad wakeup now");
									free_talk_set_mode(FREE_TALK_MODE_SPEECH_DETECT); 	
									set_free_talk_asr_record_data(free_talk_handle, msg.p_record_data, 0);
									break;
								}
								frame_len -= AUDIO_PCM_10MS_FRAME_SIZE;
								frame_start += AUDIO_PCM_10MS_FRAME_SIZE;
							}

							if (ret == 0)
							{
								free_talk_init_record_status(&free_talk_handle->status);
								set_free_talk_asr_record_data(free_talk_handle, msg.p_record_data, 0);
							}

							if (abs(msg.p_record_data->record_ms - free_talk_handle->status.keyword_record_ms) >= 30*1000)
							{
//								led_ctrl_set_mode(LED_CTRL_EYE_LIGHT);
								free_talk_set_mode(FREE_TALK_MODE_KEYWORD_WAKEUP);
							}
							break;
						}
						case FREE_TALK_MODE_SPEECH_DETECT:
						{
#ifdef DEBUG_PRINT
							printf("free_talk 4 ! SPEECH_DETECT\n");
#endif
							//静音检测,是否结束说话		
							set_free_talk_asr_record_data(free_talk_handle, msg.p_record_data, 0);
							while (frame_len >= AUDIO_PCM_10MS_FRAME_SIZE)
							{
								ret = DB_Vad_Process(free_talk_handle->vad_handle, AUDIO_PCM_RATE, AUDIO_PCM_10MS_FRAME_SIZE/2, (int16_t*)(msg.p_record_data->data + frame_start));
								//printf("*********Vad state = [ %d ]***********\n", ret);
								if (ret == 1)
								{					
									free_talk_handle->status.record_quiet_ms = 0;
								}
								else
								{
									free_talk_handle->status.record_quiet_ms += 10;
								}
								frame_len -= AUDIO_PCM_10MS_FRAME_SIZE;
								frame_start += AUDIO_PCM_10MS_FRAME_SIZE;

								if (free_talk_handle->status.record_quiet_ms >= 200 
									&& free_talk_handle->status.record_total_ms >= 3000)
								{
#ifdef DEBUG_PRINT
									printf("###################### record_quiet_ms = [ %d ]###############\n###################### record_total_ms = [ %d ]###############\n",
										free_talk_handle->status.record_quiet_ms, free_talk_handle->status.record_total_ms);
#endif
									free_talk_stop();
									free_talk_set_mode(FREE_TALK_MODE_SPEECH_END);
									break;
								}
								
								if (free_talk_handle->status.record_total_ms >= ASR_AMR_MAX_TIME_MS)
								{
#ifdef DEBUG_PRINT
									printf("###################### record_total_ms = [ %d ]###############", free_talk_handle->status.record_total_ms);
#endif
									free_talk_stop();
									free_talk_set_mode(FREE_TALK_MODE_SPEECH_END);
									break;
								}
							}
								
							break;
						}
						case FREE_TALK_MODE_SPEECH_END:
						{
#ifdef DEBUG_PRINT
							printf("free_talk 5 ! SPEECH_END\n");
#endif
							set_free_talk_asr_record_data(free_talk_handle, msg.p_record_data, 0);
							free_talk_stop();
							break;
						}
						default:
							break;
					}
#ifdef DEBUG_PRINT
					ESP_LOGI(LOG_TAG, "record_total_ms=%d, record_quiet_ms=%d", 
						free_talk_handle->status.record_total_ms, free_talk_handle->status.record_quiet_ms);
#endif					
					break;
				}
				case FREE_TALK_MSG_RECORD_STOP:
				{
#ifdef DEBUG_PRINT
					printf("free_talk 6 ! RECORD_STOP\n");
#endif
					if (free_talk_get_mode() != FREE_TALK_MODE_SPEECH_END)
					{
						break;
					}
					color_led_flicker_stop();
					color_led_on();
					set_free_talk_asr_record_data(free_talk_handle, msg.p_record_data, 1);	
				
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


void free_talk_create(MediaService *self)
{
	g_free_talk_handle = (FREE_TALK_HANDLE_T *)esp32_malloc(sizeof(FREE_TALK_HANDLE_T));
	if (g_free_talk_handle == NULL)
	{
		return;
	}
	memset(g_free_talk_handle, 0, sizeof(FREE_TALK_HANDLE_T));

	g_free_talk_handle->service = self;
	g_free_talk_handle->msg_queue = xQueueCreate(20, sizeof(FREE_TALK_MSG_T));
	g_talk_mode_mux = xSemaphoreCreateMutex();
	g_free_talk_handle->is_auto_start = 0;
	
	if (xTaskCreate(task_free_talk,
					"task_free_talk",
					1024*3,
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

void free_talk_set_mode(int _mode)
{
	if (g_free_talk_handle == NULL)
	{
		return;
	}
	
	xSemaphoreTake(g_talk_mode_mux, portMAX_DELAY);
	g_free_talk_handle->mode = _mode;
	xSemaphoreGive(g_talk_mode_mux);
}

int free_talk_get_mode(void)
{
	int mode = -1;

	if (g_free_talk_handle == NULL)
	{
		return;
	}
	
	xSemaphoreTake(g_talk_mode_mux, portMAX_DELAY);
	mode = g_free_talk_handle->mode;
	xSemaphoreGive(g_talk_mode_mux);
	
	return mode;
}

void set_free_talk_flag(int free_talk_flag)
{
	if (g_free_talk_handle == NULL)
	{
		return;
	}
	
	//xSemaphoreTake(g_talk_mode_mux, portMAX_DELAY);
	g_free_talk_flag = free_talk_flag;
	//xSemaphoreGive(g_talk_mode_mux);
}

int get_free_talk_flag()
{
	int flag = 0;

	if (g_free_talk_handle == NULL)
	{
		return;
	}
	
	//xSemaphoreTake(g_talk_mode_mux, portMAX_DELAY);
	flag = g_free_talk_flag;
	///xSemaphoreGive(g_talk_mode_mux);
	
	return flag;
}

void free_talk_start(void)
{
	if (get_free_talk_flag() == FREE_TALK_STATE_FLAG_START_STATE)
	{
		player_mdware_start_record(1, FREE_TALK_MAX_RECORD_TIME_MS, set_free_talk_record_data);
	}
}

void free_talk_stop(void)
{
	player_mdware_stop_record();
}

void set_free_talk_pause(void)
{
	if (g_free_talk_handle == NULL)
	{
		return;
	}
	
	xSemaphoreTake(g_talk_mode_mux, portMAX_DELAY);
	g_free_talk_handle->is_auto_start = 0;
	xSemaphoreGive(g_talk_mode_mux);
}

void set_free_talk_resume(void)
{
	if (g_free_talk_handle == NULL)
	{
		return;
	}
	
	xSemaphoreTake(g_talk_mode_mux, portMAX_DELAY);
	g_free_talk_handle->is_auto_start = 1;
	xSemaphoreGive(g_talk_mode_mux);
}

void free_talk_start_after_music(void)
{
	if (g_free_talk_handle == NULL)
	{
		return;
	}
	
	xSemaphoreTake(g_talk_mode_mux, portMAX_DELAY);
	if (g_free_talk_handle->is_auto_start == 1)
	{
		free_talk_start();
	}
	xSemaphoreGive(g_talk_mode_mux);
}

void free_talk_stop_before_music(void)
{
	if (g_free_talk_handle == NULL)
	{
		return;
	}
	
	xSemaphoreTake(g_talk_mode_mux, portMAX_DELAY);
	if (g_free_talk_handle->is_auto_start == 1)
	{
		free_talk_stop();
	}
	xSemaphoreGive(g_talk_mode_mux);
}


