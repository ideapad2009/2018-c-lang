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
#include "geling_edu.h"
#include "player_middleware.h"
#include "auto_play_service.h"

#define LOG_TAG "geling edu service" 
#define M3U8_SEG_START "#EXTINF:,\r\n"
#define M3U8_SEG_END "\r\n"

static const char *GET_M3U8_REQUEST_HEADER = "GET %s HTTP/1.0\r\n"
    "Host: %s:%s\r\n"
    "User-Agent: esp-idf/1.0 esp32\r\n"
    "Content-Type: application/json; charset=utf-8\r\n\r\n";


static GELING_EDU_SERVICE_STATUS_T *g_geling_edu_service_status = NULL;

static void geling_edu_lock(void)
{
	if (g_geling_edu_service_status != NULL)
	{
		xSemaphoreTake(g_geling_edu_service_status->data_lock, portMAX_DELAY);
	}
}

static void geling_edu_unlock(void)
{
	if (g_geling_edu_service_status != NULL)
	{
		xSemaphoreGive(g_geling_edu_service_status->data_lock);
	}
}

static void get_prev_music(AUTO_PLAY_MUSIC_SOURCE_T *src)
{
	if (g_geling_edu_service_status == NULL)
	{
		return;
	}

	geling_edu_lock();
	GELING_EDU_LINKS_T *p_links = &g_geling_edu_service_status->edu_links;
	memset(src, 0, sizeof(AUTO_PLAY_MUSIC_SOURCE_T));
	if (p_links->link_size == 0)
	{
		return;
	}

	if (p_links->cur_play_index > 0)
	{
		p_links->cur_play_index--;
	}

	ESP_LOGI(LOG_TAG, "get_prev_music [%d][%s]", 
		p_links->cur_play_index+1,
		p_links->link[p_links->cur_play_index].link_url);
	
	src->need_play_name = 0;
	snprintf(src->music_url, sizeof(src->music_url), "%s", p_links->link[p_links->cur_play_index].link_url);
	geling_edu_unlock();
}

static void get_next_music(AUTO_PLAY_MUSIC_SOURCE_T *src)
{
	if (g_geling_edu_service_status == NULL)
	{
		return;
	}

	geling_edu_lock();
	GELING_EDU_LINKS_T *p_links = &g_geling_edu_service_status->edu_links;
	memset(src, 0, sizeof(AUTO_PLAY_MUSIC_SOURCE_T));
	if (p_links->link_size == 0)
	{
		return;
	}

	if (p_links->cur_play_index < p_links->link_size - 1)
	{
		p_links->cur_play_index++;

		ESP_LOGI(LOG_TAG, "get_next_music [%d][%s]", 
		p_links->cur_play_index+1,
		p_links->link[p_links->cur_play_index].link_url);
		src->need_play_name = 0;		
		snprintf(src->music_url, sizeof(src->music_url), "%s", p_links->link[p_links->cur_play_index].link_url);
	}

	geling_edu_unlock();
}

static void get_current_music(AUTO_PLAY_MUSIC_SOURCE_T *src)
{
	if (g_geling_edu_service_status == NULL)
	{
		return;
	}

	geling_edu_lock();
	GELING_EDU_LINKS_T *p_links = &g_geling_edu_service_status->edu_links;
	memset(src, 0, sizeof(AUTO_PLAY_MUSIC_SOURCE_T));
	if (p_links->link_size != 0)
	{
		ESP_LOGI(LOG_TAG, "get_current_music [%d][%s]", 
			p_links->cur_play_index+1,
			p_links->link[p_links->cur_play_index].link_url);
		
		src->need_play_name = 0;
		snprintf(src->music_url, sizeof(src->music_url), "%s", p_links->link[p_links->cur_play_index].link_url);
	}
	geling_edu_unlock();
}


static char* find_m3u8_link(char *link, const size_t link_size, char *str_body)
{
	int len = 0;
	char *p_start = NULL;
	char *p_end = NULL;
	
	p_start = strstr(str_body, M3U8_SEG_START);
	if (p_start == NULL)
	{
		ESP_LOGE(LOG_TAG, "find_m3u8_link start failed");
		return NULL;
	}
	p_start += strlen(M3U8_SEG_START);

	p_end = strstr(p_start, M3U8_SEG_END);
	if (p_end == NULL)
	{
		ESP_LOGE(LOG_TAG, "find_m3u8_link end failed");
		return NULL;
	}

	len = p_end - p_start;
	if (len > link_size - 1)
	{
		ESP_LOGE(LOG_TAG, "find_m3u8_link len[%d],link_size[%d]", len, link_size);
		return NULL;
	}

	memset(link, 0, link_size);
	memcpy(link, p_start, len);
	ESP_LOGE(LOG_TAG, "edu link[%s]", link);

	return p_end;
}

static int geling_edu_result_decode(char* str_reply, GELING_EDU_SERVICE_STATUS_T *status)
{	
	int i = 0;
	char* str_body = http_get_body(str_reply);
	if (NULL == str_body)
	{
		return ESP_FAIL;
	}
	
	geling_edu_lock();
	char *p = str_body;
	memset(&status->edu_links, 0, sizeof(status->edu_links));
	for (i=0; i<GELING_EDU_LINK_MAX_NUM; i++)
	{
		p = find_m3u8_link(&status->edu_links.link[i], sizeof(status->edu_links.link[i]), p);
		if (p != NULL)
		{
			status->edu_links.link_size++;
		}
		else
		{
			break;
		}
	}
	geling_edu_unlock();
	
	if (status->edu_links.link_size == 0)
	{
		return ESP_FAIL;
	}
	
	return ESP_OK;
}

static int http_get_m3u8_link(
	char* str_reply_buff,
	GELING_EDU_HTTP_BUFF_T *http_buffer,
	uint32_t reply_buff_len,
	const char* str_edu_url)
{
	sock_t 	sock		= INVALID_SOCK;

	if (sock_get_server_info(str_edu_url, &http_buffer->domain, &http_buffer->port, &http_buffer->params) != 0)
	{
		DEBUG_LOGE(LOG_TAG, "sock_get_server_info fail");
		goto HTTP_POST_METHOD_ERROR;
	}

	sock = sock_connect(http_buffer->domain, http_buffer->port);
	if (sock == INVALID_SOCK)
	{
		DEBUG_LOGE(LOG_TAG, "sock_connect %s:%s failed", http_buffer->domain, http_buffer->port);
		goto HTTP_POST_METHOD_ERROR;
	}
	sock_set_nonblocking(sock);

	snprintf(http_buffer->header, sizeof(http_buffer->header), GET_M3U8_REQUEST_HEADER, 
		http_buffer->params, http_buffer->domain, http_buffer->port);
//	DEBUG_LOGE(TAG, "header is %s", header);
	if (sock_writen_with_timeout(sock, http_buffer->header, strlen(http_buffer->header), 1000) != strlen(http_buffer->header)) 
	{
		DEBUG_LOGE(LOG_TAG, "sock_write http header fail");
		goto HTTP_POST_METHOD_ERROR;
	}
	
	memset(str_reply_buff, 0, reply_buff_len);
	sock_readn_with_timeout(sock, str_reply_buff, reply_buff_len - 1, 5000);
	sock_close(sock);

	if (http_get_error_code(str_reply_buff) == 200)
	{
		return DB_ERROR_CODE_SUCCESS;
	}
	else
	{
		DEBUG_LOGE(LOG_TAG, "%s", str_reply_buff);
		return DB_ERROR_CODE_HTTP_FAIL;
	}
	
	return ESP_OK;
HTTP_POST_METHOD_ERROR:

	if (sock > 0)
	{
		sock_close(sock);
	}
	return ESP_FAIL;
}


void task_geling_edu(void *pv)
{
	int ret 			= ESP_FAIL;
	int start_time		= 0;
	int end_time		= 0;
	int nlp_req_count	= 0;
	int nlp_req_total_ms = 0;
	GELING_EDU_MSG_T *p_msg = NULL;
	GELING_EDU_SERVICE_STATUS_T *status = (GELING_EDU_SERVICE_STATUS_T *)pv;
	AUTO_PLAY_CALL_BACK_T call_backs = {get_prev_music, get_next_music, get_current_music};

	while (1) 
	{
		if (xQueueReceive(status->msg_queue, &p_msg, portMAX_DELAY) == pdPASS) 
		{	
			if (p_msg->msg_type == NLP_SERVICE_MSG_TYPE_QUIT)
			{
				esp32_free(p_msg);
				p_msg = NULL;
				break;
			}
			
			if (strlen(p_msg->edu_url) > 0)
			{
				memset(status->http_reply_buffer, 0, sizeof(status->http_reply_buffer));
				ret = http_get_m3u8_link(status->http_reply_buffer, &status->http_buffer, sizeof(status->http_reply_buffer), p_msg->edu_url);	
				if (ret == ESP_OK)
				{
					//DEBUG_LOGE(LOG_TAG, "%s", status->http_reply_buffer);
					auto_play_stop();
					ret = geling_edu_result_decode(status->http_reply_buffer, status);
					if (ret == ESP_OK)
					{
						set_auto_play_cb(&call_backs);
					}
				}
			}
			
			if (ret != ESP_OK)
			{
				player_mdware_play_tone(FLASH_MUSIC_NO_MUSIC);
			}

			esp32_free(p_msg);
			p_msg = NULL;
		}
	}

	esp32_free(status);
	vTaskDelete(NULL);
}

static void geling_edu_service_send_msg(
	GL_NLP_SERVICE_MSG_TYPE_T msg_type, 
	char *text)
{
	if (g_geling_edu_service_status == NULL || g_geling_edu_service_status->msg_queue == NULL)
	{
		return;
	}

	GELING_EDU_MSG_T *p_msg = (GELING_EDU_MSG_T *)esp32_malloc(sizeof(GELING_EDU_MSG_T));
	if (p_msg == NULL)
	{
		ESP_LOGE(LOG_TAG, "geling_edu_service_send_msg malloc failed, out of memory");
		return;
	}
	p_msg->msg_type = msg_type;
	if (text != NULL)
	{
		snprintf(p_msg->edu_url, sizeof(p_msg->edu_url), "%s", text);
	}

	if (xQueueSend(g_geling_edu_service_status->msg_queue, &p_msg, 0) != pdPASS)
	{
		ESP_LOGE(LOG_TAG, "geling_edu_service_send_msg xQueueSend failed");
		esp32_free(p_msg);
		p_msg = NULL;
	}
}

void geling_edu_service_create(void)
{
	g_geling_edu_service_status = (GELING_EDU_SERVICE_STATUS_T *)esp32_malloc(sizeof(GELING_EDU_SERVICE_STATUS_T));
	if (g_geling_edu_service_status == NULL)
	{
		ESP_LOGE(LOG_TAG, "geling_edu_service_create failed, out of memory");
		return;
	}
	memset((char*)g_geling_edu_service_status, 0, sizeof(GELING_EDU_SERVICE_STATUS_T));
	g_geling_edu_service_status->msg_queue = xQueueCreate(1, sizeof(char *));
	g_geling_edu_service_status->data_lock = xSemaphoreCreateMutex();
	
    if (xTaskCreate(task_geling_edu,
                    "task_geling_edu",
                    512*4,
                    g_geling_edu_service_status,
                    4,
                    NULL) != pdPASS) {

        ESP_LOGE(LOG_TAG, "ERROR creating task_geling_edu task! Out of memory?");
    }
}

void geling_edu_service_delete(void)
{
	geling_edu_service_send_msg(NLP_SERVICE_MSG_TYPE_QUIT, NULL);
}

void geling_edu_service_send_request(const char *text)
{
	geling_edu_service_send_msg(NLP_SERVICE_MSG_TYPE_REQUEST, text);
}

