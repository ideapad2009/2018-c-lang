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
#include "baidu_api.h"
#include "http_api.h"
#include "deepbrain_api.h"
#include "device_api.h"
#include "socket.h"
#include "lwip/netdb.h"
#include "EspAudio.h"
#include "toneuri.h"
#include "userconfig.h"
#include "flash_config_manage.h"
#include "bind_device.h"

#define BIND_TAG "BIND DEVICE"

static int g_bind_device_quit = 0;

static void save_user_id(char *_content)
{
	int ret = 0;
	
	cJSON *pJson_body = cJSON_Parse(_content);
    if (NULL != pJson_body) 
	{
        cJSON *pJson_phone_num = cJSON_GetObjectItem(pJson_body, "phoneNumber");
		if (NULL == pJson_phone_num 
			|| pJson_phone_num->valuestring == NULL 
			|| strlen(pJson_phone_num->valuestring) <= 0)
		{
			goto save_user_id_error;
		}

		if (set_flash_cfg(FLASH_CFG_USER_ID, pJson_phone_num->valuestring) == ESP_OK)
		{
			DEBUG_LOGI(BIND_TAG, "save user id[%s] success", pJson_phone_num->valuestring);
		}
		else
		{
			DEBUG_LOGE(BIND_TAG, "save user id[%s] fail", pJson_phone_num->valuestring);
		}
		
    }
	else
	{
		DEBUG_LOGE(BIND_TAG, "cJSON_Parse error");
	}

save_user_id_error:

	if (NULL != pJson_body)
	{
		cJSON_Delete(pJson_body);
	}

	return;
}

void device_bind_process(void)
{  
	static char *DEVICE_BIND_INFO = "{\"deviceSN\": \"%s\",\"deviceVersion\": \"%s\"}";
	BIND_DEVICE_HANDLE_T *handle = esp32_malloc(sizeof(BIND_DEVICE_HANDLE_T));
	int listenfd = -1;
	int connectfd = -1;
	struct sockaddr_in server;	//服务器地址信息结构体  
	struct sockaddr_in client;	//客户端地址信息结构体  
	int sin_size;  
	
	while(!g_bind_device_quit)
	{
		vTaskDelay(100);
		listenfd = socket(AF_INET, SOCK_STREAM, 0);
		if (listenfd == -1) 
		{ //调用socket，创建监听客户端的socket  
			continue;  
		}  
		  
		int opt = SO_REUSEADDR; 		 
		setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));	  //设置socket属性，端口可以重用  
		//初始化服务器地址结构体  
		bzero(&server,sizeof(server));	
		server.sin_family=AF_INET;	
		server.sin_port=htons(9099);  
		server.sin_addr.s_addr = htonl(INADDR_ANY);  
		
		//调用bind，绑定地址和端口  
		if (bind(listenfd, (struct sockaddr *)&server, sizeof(struct sockaddr)) == -1) 
		{
			goto _task_process_device_bind_error; 
		}	  
		DEBUG_LOGE(BIND_TAG, "bind success");

		//调用listen，开始监听	 
		if (listen(listenfd, 2) == -1)
		{ 	 
			goto _task_process_device_bind_error;
		}  
		DEBUG_LOGE(BIND_TAG, "listen success");
		
		sin_size=sizeof(struct sockaddr_in); 
		unsigned int recv_len = 0;

		connectfd = accept(listenfd,(struct sockaddr *)&client,(socklen_t *)&sin_size);
		if (connectfd == -1) 
		{ 					 
			//调用accept，返回与服务器连接的客户端描述符	
			goto _task_process_device_bind_error;
		}  
		sock_set_nonblocking(connectfd);
		DEBUG_LOGE(BIND_TAG, "accept one client %d", connectfd);
		
		//读取绑定信息
		memset(handle->comm_buffer, 0, sizeof(handle->comm_buffer));
		DEBUG_LOGE(BIND_TAG, "recving info");
		int ret = sock_readn_with_timeout(connectfd, (char*)&recv_len, sizeof(recv_len), 5000);
		if (ret == sizeof(recv_len))
		{		
			recv_len = ntohl(recv_len);
			if (recv_len > sizeof(handle->comm_buffer) - 1)
			{
				DEBUG_LOGE(BIND_TAG, "recv_len:[%d] too long, max size = %d", recv_len, sizeof(handle->comm_buffer) -1);
				goto _task_process_device_bind_error;
			}
			
			int ret = sock_readn_with_timeout(connectfd, handle->comm_buffer, recv_len, 2000);
			if (ret == recv_len)
			{
				DEBUG_LOGE(BIND_TAG, "recv info:[%d]%s", ret, handle->comm_buffer);
				save_user_id(handle->comm_buffer);
				
				char device_id[32]= {0};
				get_flash_cfg(FLASH_CFG_DEVICE_ID, &device_id);
				snprintf(handle->comm_buffer, sizeof(handle->comm_buffer), DEVICE_BIND_INFO, device_id, ESP32_FW_VERSION);
				DEBUG_LOGE(BIND_TAG, "write info:[%d]%s", strlen(handle->comm_buffer), handle->comm_buffer);
				recv_len = htonl(strlen(handle->comm_buffer));
				if (sock_writen_with_timeout(connectfd, (void*)&recv_len, sizeof(recv_len), 2000) != sizeof(recv_len))
				{
					DEBUG_LOGE(BIND_TAG, "sock_write header %d bytes fail", sizeof(recv_len));
				}
				
				if (sock_writen_with_timeout(connectfd, handle->comm_buffer, strlen(handle->comm_buffer), 2000) != strlen(handle->comm_buffer))
				{
					DEBUG_LOGE(BIND_TAG, "sock_write body %d bytes fail", strlen(handle->comm_buffer));
				}
			}
			else
			{
				DEBUG_LOGE(BIND_TAG, "data len =%d, recv len=%d", recv_len, ret);
			}
		}
		else
		{
			DEBUG_LOGE(BIND_TAG, "recv info:ret = %d, is not 4 bytes length", ret);
		}
		
_task_process_device_bind_error:			
		//关闭此次连接
		close(connectfd);
		close(listenfd);   //关闭监听socket 	   
	}
	vTaskDelete(NULL);
	DEBUG_LOGE(BIND_TAG, "device bind server exit");
}

void bind_device_create(void)
{
    if (xTaskCreate(device_bind_process,
                    "device_bind_process",
                    1024*3,
                    NULL,
                    4,
                    NULL) != pdPASS) {

        ESP_LOGE(BIND_TAG, "ERROR creating device_bind_process task! Out of memory?");
    }
}

void bind_device_delete(void)
{
	g_bind_device_quit = 1;
}


