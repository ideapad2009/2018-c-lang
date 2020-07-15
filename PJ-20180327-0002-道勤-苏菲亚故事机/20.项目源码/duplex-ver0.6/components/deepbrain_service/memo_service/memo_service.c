#include <time.h>
#include <string.h>
#include <stdio.h>
#include <socket.h>
#include "freertos/timers.h"
#include "cJSON.h"
#include "MediaService.h"
#include "mpush_client_api.h"
#include "esp_log.h"
#include "array_music.h"
#include "player_middleware.h"
#include "ctypes.h"
#include "memo_service.h"
#include "auto_play_service.h"
#include "userconfig.h"
#include "low_power_manage.h"

#define MEMO_TAG     "MEMO CLIENT"

//attrName
#define MEMO_TIME    "备忘时间"
#define MEMO_EVENT   "备忘事件"
#define EXECUTETIME  "executeTime"
#define EVENTCONTENT "eventContent"
#define EVENTTYPE    "eventType"

//attrValue
#define OUTDATE      "outDateError"
#define REMINDMEMO   "remindMemo"

MEMO_SERVICE_HANDLE_T *memo_struct_handle = NULL;

static int get_net_time(void)
{	
   	sock_t	 sock	 = INVALID_SOCK;
 	cJSON	*pJson   = NULL;

	TEMP_PARAM_T *temp = (TEMP_PARAM_T *)esp32_malloc(sizeof(TEMP_PARAM_T));
	if (NULL == temp)
	{
		return ESP_FAIL;
	}
	memset(temp, 0, sizeof(TEMP_PARAM_T));
	
	if (sock_get_server_info(DeepBrain_TEST_URL, temp->domain, temp->port, temp->params) != 0)
	{
		ESP_LOGE(MEMO_TAG, "get_net_time, sock_get_server_info fail\r\n");
		goto get_net_time_error;
	}

	sock = sock_connect(temp->domain, temp->port);
	if (sock == INVALID_SOCK)
	{
		ESP_LOGE(MEMO_TAG, "get_net_time, sock_connect fail,%s,%s\r\n", temp->domain, temp->port);
		goto get_net_time_error;
	}

	sock_set_nonblocking(sock);
	
	crypto_generate_request_id(temp->str_nonce, sizeof(temp->str_nonce));
	get_flash_cfg(FLASH_CFG_DEVICE_ID, temp->str_device_id);
	snprintf(temp->str_comm_buf, sizeof(temp->str_comm_buf), 
		"{\"app_id\": \"%s\","
		"\"content\":{},"
		"\"device_id\": \"%s\","
		"\"request_id\": \"%s\","
		"\"robot_id\": \"%s\","
		"\"service\": \"DeepBrainTimingVerificationService\","
		"\"user_id\": \"%s\","
		"\"version\": \"2.0.0.0\"}",
		DEEP_BRAIN_APP_ID, 
		temp->str_device_id, temp->str_nonce, DEEP_BRAIN_ROBOT_ID, temp->str_device_id);
	
	crypto_generate_nonce((uint8_t *)temp->str_nonce, sizeof(temp->str_nonce));
	crypto_time_stamp((unsigned char*)temp->str_timestamp, sizeof(temp->str_timestamp));
	crypto_generate_private_key((uint8_t *)temp->str_private_key, sizeof(temp->str_private_key), temp->str_nonce, temp->str_timestamp, DEEP_BRAIN_ROBOT_ID);
	snprintf(temp->str_buf, sizeof(temp->str_buf), 
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
		temp->params, temp->domain, temp->port, strlen(temp->str_comm_buf), temp->str_nonce, temp->str_timestamp, temp->str_private_key, DEEP_BRAIN_ROBOT_ID);

	//ESP_LOGE(PRINT_TAG, "%s", str_buf);
	//ESP_LOGE(PRINT_TAG, "%s", _pHandler->str_comm_buf);
	if (sock_writen_with_timeout(sock, temp->str_buf, strlen(temp->str_buf), 1000) != strlen(temp->str_buf)) 
	{
		printf("sock_writen http header fail\r\n");
		goto get_net_time_error;
	}
	
	if (sock_writen_with_timeout(sock, temp->str_comm_buf, strlen(temp->str_comm_buf), 3000) != strlen(temp->str_comm_buf)) 
	{
		printf("sock_writen http body fail\r\n");
		goto get_net_time_error;
	}

	/* Read HTTP response */
	memset(temp->str_comm_buf, 0, sizeof(temp->str_comm_buf));
	sock_readn_with_timeout(sock, temp->str_comm_buf, sizeof(temp->str_comm_buf) - 1, 8000);
	sock_close(sock);
	sock = INVALID_SOCK;

	if (http_get_error_code(temp->str_comm_buf) == 200)
	{	
		ESP_LOGE(MEMO_TAG, "str_comm_buf = %s\r\n", temp->str_comm_buf);
		char* pBody = http_get_body(temp->str_comm_buf);
		if (NULL != pBody)
		{
			pJson = cJSON_Parse(pBody);
			if (NULL != pJson) 
			{
				cJSON *pJson_status = cJSON_GetObjectItem(pJson, "statusCode");
				if (NULL == pJson_status || pJson_status->valuestring == NULL)
				{
					ESP_LOGE(MEMO_TAG, "get_net_time, statusCode not found");
					goto get_net_time_error;
				}

				if (strncasecmp(pJson_status->valuestring, "OK", 2) != 0)
				{
					ESP_LOGE(MEMO_TAG, "get_net_time, statusCode:%s", pJson_status->valuestring);
					goto get_net_time_error;
				}
				ESP_LOGI(MEMO_TAG, "get_net_time, statusCode:%s", pJson_status->valuestring);

				/* 获取时间 */
				cJSON *pJson_content = cJSON_GetObjectItem(pJson, "content");
				if (NULL == pJson_content || pJson_content->child == NULL)
				{
					ESP_LOGE(MEMO_TAG, "get_net_time, pJson_content not found");
					goto get_net_time_error;
				}
	 			cJSON *pJson_content_time = cJSON_GetObjectItem(pJson_content, "currentTimeMillis");
				if (NULL == pJson_content_time || (uint)(pJson_content_time->valuedouble/1000) == 0)
				{
					ESP_LOGE(MEMO_TAG, "get_net_time, pJson_content_time not found");
					goto get_net_time_error;
				}

				memo_struct_handle->initial_time_usec = (uint)pJson_content_time->valuedouble;
				memo_struct_handle->initial_time_sec  = (uint)(pJson_content_time->valuedouble/1000);
				
				ESP_LOGE(MEMO_TAG, "valuedouble = [%f]",pJson_content_time->valuedouble);
			}
			else
			{
				ESP_LOGE(MEMO_TAG, "get_net_time, invalid json[%s]", temp->str_comm_buf);
			}
			
			if (NULL != pJson) {
				cJSON_Delete(pJson);
				pJson = NULL;
			}
		}
	}
	else
	{
		ESP_LOGE(MEMO_TAG, "get_net_time, http reply error[%s]", temp->str_comm_buf);
		goto get_net_time_error;
	}

	if (NULL != temp)
	{
		esp32_free(temp);
		temp = NULL;
	}
	
	return ESP_OK;
	
get_net_time_error:
	
	if (sock != INVALID_SOCK) 
	{
		sock_close(sock);
	}
	
	if (NULL != pJson) 
	{
		cJSON_Delete(pJson);
		pJson = NULL;
	}

	if (NULL != temp)
	{
		esp32_free(temp);
		temp = NULL;
	}
	
	return ESP_FAIL;
}

int get_memo_result(char *str_body)
{
	time_t time = 0;
	struct tm tm = {0};
	int arry_size = 0;
	int memo_status = 0;
			
	cJSON *pJson_body = cJSON_Parse(str_body);
	if (NULL == pJson_body) 
	{
		ESP_LOGE(MEMO_TAG, "str_body cJSON_Parse fail");
		goto get_memo_result_error;
	}

	//命令名称
	cJSON *pJson_cmd_data = cJSON_GetObjectItem(pJson_body, "commandData");
	if (NULL == pJson_cmd_data) 
	{
		ESP_LOGE(MEMO_TAG, "pJson_body, pJson_cmd_data not found\n");
		goto get_memo_result_error;
	}
	
	cJSON *pJson_cmd_name = cJSON_GetObjectItem(pJson_cmd_data, "commandName");
	if (NULL == pJson_cmd_name) 
	{
		ESP_LOGE(MEMO_TAG, "str_body_buf,commandName not found\n");
		goto get_memo_result_error;
	}			
	ESP_LOGE(MEMO_TAG, "pJson_cmd_name = %s", pJson_cmd_name->valuestring);

	//JSON数组
	cJSON *pJson_array = cJSON_GetObjectItem(pJson_cmd_data, "commandAttrs");
	if (NULL == pJson_array) 
	{
		ESP_LOGE(MEMO_TAG, "str_body_buf,pJson_array not found\n");
		goto get_memo_result_error;
	}

	//JSON数组大小
	arry_size = cJSON_GetArraySize(pJson_array);
	if (arry_size == 0)
	{
		ESP_LOGE(MEMO_TAG, "arry_size[%d], error", arry_size);
		goto get_memo_result_error;
	}

	for (int i = 0; i < arry_size; i++)
	{
		//获取数组成员
		cJSON *pJson_array_item = cJSON_GetArrayItem(pJson_array, i);
		if (NULL == pJson_array_item) 
		{
			ESP_LOGE(MEMO_TAG, "pJson_array, pJson_array_item not found\n");
			goto get_memo_result_error;
		}

		//成员 attrName
		cJSON *pJson_array_attrName = cJSON_GetObjectItem(pJson_array_item, "attrName");
		if (NULL == pJson_array_attrName) 
		{
			ESP_LOGE(MEMO_TAG, "pJson_array_item, pJson_array_attrName not found\n");
			goto get_memo_result_error;
		}

		//成员 attrValue
		cJSON *pJson_array_attrValue = cJSON_GetObjectItem(pJson_array_item, "attrValue");
		if (NULL == pJson_array_attrValue) 
		{
			ESP_LOGE(MEMO_TAG, "pJson_array_item,pJson_array_attrValue not found\n");
			goto get_memo_result_error;
		}
		
		ESP_LOGE(MEMO_TAG, "pJson_array_attrName = %s", pJson_array_attrName->valuestring);
		ESP_LOGE(MEMO_TAG, "pJson_array_attrValue = %s", pJson_array_attrValue->valuestring);
		
		if (strncmp(pJson_array_attrName->valuestring, EXECUTETIME, sizeof(EXECUTETIME)) == 0)//执行时间
		{		
			//将特定格式的字符时间转换为struct tm格式
			strptime(pJson_array_attrValue->valuestring, "%Y,%m,%d,%H,%M,%S", &tm);
			time = mktime(&tm) - 28800;
			ESP_LOGE(MEMO_TAG, "**********time = [%ld]", time);
		}
		else if (strncmp(pJson_array_attrName->valuestring, EVENTTYPE, sizeof(EVENTTYPE)) == 0)//事件类型
		{		
			if(strncmp(pJson_array_attrValue->valuestring, OUTDATE, sizeof(OUTDATE)) == 0) //日期过期			
			{
				memo_status = 0;				
			}
			else if (strncmp(pJson_array_attrValue->valuestring, REMINDMEMO, sizeof(REMINDMEMO)) == 0)//记录备忘
			{
				memo_status = 1;				
			}
		}
		else if (strncmp(pJson_array_attrName->valuestring, EVENTCONTENT, sizeof(EVENTCONTENT)) == 0)//事件内容
		{		
			snprintf(memo_struct_handle->memo_event.str, sizeof(memo_struct_handle->memo_event.str), pJson_array_attrValue->valuestring);
		}		
		
		pJson_array_item      = NULL;
		pJson_array_attrName  = NULL;
		pJson_array_attrValue = NULL;
	}

	if (memo_status)
	{
		memo_struct_handle->memo_event.time = time;
		memo_struct_handle->memo_status = MEMO_ADD;	
		play_tts("备忘已添加");		
	}
	else
	{
		play_tts(memo_struct_handle->memo_event.str);
		memset(memo_struct_handle->memo_event.str, 0, sizeof(memo_struct_handle->memo_event.str));
	}
	
	if (NULL != pJson_body) 
	{
		cJSON_Delete(pJson_body);
		pJson_body = NULL;
	}

	return ESP_OK;		

get_memo_result_error:	

	if (NULL != pJson_body) 
	{
		cJSON_Delete(pJson_body);
		pJson_body = NULL;
	}
	
	return ESP_FAIL;
}

//初始化时钟
static void init_clock(MEMO_SERVICE_HANDLE_T *handle)
{
	while(1)
	{	
 		if ((handle->service->_blocking == 0) && (get_net_time() == 0))
		{ 
			break;
 		}
		vTaskDelay(3*1000);
	}

	//校准系统时钟
 	struct timeval tv = { 
		.tv_sec  = handle->initial_time_sec,
		.tv_usec = handle->initial_time_usec
	};
	settimeofday(&tv, NULL);

	get_flash_cfg(FLASH_CFG_MEMO_PARAMS, &(handle->memo_array));
	if(handle->memo_array.current > 0)
	{
		handle->memo_status = MEMO_LOOP;
	}
	else
	{
		handle->memo_status = MEMO_IDLE;
	}
}

//添加提醒
static void add_memo(MEMO_SERVICE_HANDLE_T *handle)
{		
	int  num_min  = 0;
	uint time_min = 0;
	
	//备忘已满将覆盖最小日期
	if (handle->memo_array.current >= MEMO_EVEVT_MAX)
	{
		for (int num = 0; num < MEMO_EVEVT_MAX; num++) 
		{
			if (time_min > handle->memo_array.event[num].time)
			{
				num_min  = num;
				time_min = handle->memo_array.event[num].time;
			}
		}
		handle->memo_array.event[num_min].time = handle->memo_event.time;
		snprintf(handle->memo_array.event[num_min].str, sizeof(handle->memo_event.str), handle->memo_event.str);
		memset(&handle->memo_event, 0, sizeof(MEMO_EVENT_T));
		set_flash_cfg(FLASH_CFG_MEMO_PARAMS, &handle->memo_array);
	}
	else
	{
		for (int num = 0; num < MEMO_EVEVT_MAX; num++) 
		{
			if (handle->memo_array.event[num].time == 0)
			{
				handle->memo_array.event[num].time = handle->memo_event.time;
				snprintf(handle->memo_array.event[num].str, sizeof(handle->memo_event.str), handle->memo_event.str);
				handle->memo_array.current++;
				memset(&handle->memo_event, 0, sizeof(MEMO_EVENT_T));
				set_flash_cfg(FLASH_CFG_MEMO_PARAMS, &handle->memo_array);
				break;
			}
		}
	}
	memo_struct_handle->memo_status = MEMO_LOOP;
}

//执行提醒
static void memo_event_handle(MEMO_ARRAY_T *handle)
{
	time_t time_sec = time(NULL);

	for(int num = 0; num < MEMO_EVEVT_MAX; num++) 
	{
		if (handle->event[num].time != 0) 
		{
			//清除过期备忘
			if (handle->event[num].time < time_sec - 30)
			{
				memset(&handle->event[num], 0, sizeof(MEMO_EVENT_T));
				handle->current--;
				set_flash_cfg(FLASH_CFG_MEMO_PARAMS, handle);
				continue;
			}

			//执行备忘提醒，并删除
			if ((handle->event[num].time - 30) < time_sec &&
				(handle->event[num].time + 30) > time_sec) 
			{
				play_tts(handle->event[num].str);
				memset(&handle->event[num], 0, sizeof(MEMO_EVENT_T));
				handle->current--;
				set_flash_cfg(FLASH_CFG_MEMO_PARAMS, handle);
			}

			if(handle->current == 0)
			{
				memo_struct_handle->memo_status = MEMO_IDLE;
			}
		}
	}
}

void show_memo(void)
{
	ESP_LOGE("TERM_CTRL", "current time = %ld\n", time(NULL));
	ESP_LOGE("TERM_CTRL", "Memo Number  = %d\n", memo_struct_handle->memo_array.current);
	for(int num = 0; num < MEMO_EVEVT_MAX; num++) 
	{
		ESP_LOGE("TERM_CTRL", "event[%d].time = %d\r\n", num, memo_struct_handle->memo_array.event[num].time);
		ESP_LOGE("TERM_CTRL", "event[%d].str  = %s\r\n", num, memo_struct_handle->memo_array.event[num].str);
	}
}

static void task_memo(void *pv)
{
	MEMO_SERVICE_HANDLE_T *handle = (MEMO_SERVICE_HANDLE_T *) pv;
	handle->memo_status = MEMO_INIT;
	
	while(1)
	{
		switch (handle->memo_status)
		{
			case MEMO_IDLE:
			{
				break;
			}			
			case MEMO_INIT:
			{
				init_clock(handle);
				break;
			}
			case MEMO_ADD:
			{
				add_memo(handle);
				break;
			}		
			case MEMO_LOOP:
			{
				memo_event_handle(&handle->memo_array);
				break;
			}	

			default:
				break;
		}		
		vTaskDelay(1000);
	}
}

void memo_service_create(MediaService *self)
{
	memo_struct_handle = (MEMO_SERVICE_HANDLE_T *)esp32_malloc(sizeof(MEMO_SERVICE_HANDLE_T));
	if (memo_struct_handle == NULL) {
		ESP_LOGE(MEMO_TAG, "esp32_malloc memo_struct_handle fail!");
		return;
	}	
	memset(memo_struct_handle, 0, sizeof(MEMO_SERVICE_HANDLE_T));
	memo_struct_handle->service = self;
	//set_flash_cfg(FLASH_CFG_MEMO_PARAMS, &memo_struct_handle->memo_array);

    if (xTaskCreate(task_memo,
                    "task_memo",
                    1024*2.5,
                    memo_struct_handle,
                    4,
                    NULL) != pdPASS) {

    	ESP_LOGE("BIND DEVICE", "ERROR creating task_memo task! Out of memory?");
		esp32_free(memo_struct_handle);
    }

	ESP_LOGE(MEMO_TAG, "memo_service_create\r\n");
}


