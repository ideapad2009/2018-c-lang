
#include "mpush_client_api.h"
#include "toneuri.h"
#include "EspAudio.h"
#include "mpush_service.h"
#include "player_middleware.h"

#define	PRINT_TAG "MPUSH CLIENT"
const char *str_public_key = "-----BEGIN PUBLIC KEY-----\nMIGfMA0GCSqGSIb3DQEBAQUAA4GNADCBiQKBgQCghPCWCobG8nTD24juwSVataW7\niViRxcTkey/B792VZEhuHjQvA3cAJgx2Lv8GnX8NIoShZtoCg3Cx6ecs+VEPD2f\nBcg2L4JK7xldGpOJ3ONEAyVsLOttXZtNXvyDZRijiErQALMTorcgi79M5uVX9/j\nMv2Ggb2XAeZhlLD28fHwIDAQAB\n-----END PUBLIC KEY-----";
extern QueueHandle_t g_mpush_msg_queue;

int mpush_get_run_status(MPUSH_CLIENT_HANDLER_t *_pHandler)
{
	xSemaphoreTake(_pHandler->lock_handle, portMAX_DELAY);
	int status = _pHandler->status;
	xSemaphoreGive(_pHandler->lock_handle);
	
	return status;
}

void mpush_set_run_status(MPUSH_CLIENT_HANDLER_t *_pHandler, int status)
{
	xSemaphoreTake(_pHandler->lock_handle, portMAX_DELAY);
	_pHandler->status = status;
	xSemaphoreGive(_pHandler->lock_handle);
	
	return;
}

int mpush_get_server_sock(MPUSH_CLIENT_HANDLER_t *_pHandler)
{
	xSemaphoreTake(_pHandler->lock_handle, portMAX_DELAY);
	int sock = _pHandler->server_sock;
	xSemaphoreGive(_pHandler->lock_handle);
	
	return sock;
}

static int _mpush_connect(MPUSH_CLIENT_HANDLER_t *_pHandler)
{
	MPUSH_SERVER_INFO_T *server_info = NULL;

	if (_pHandler->server_sock != INVALID_SOCK)
	{
		sock_close(_pHandler->server_sock);
		_pHandler->server_sock = INVALID_SOCK;
	}

	server_info = &_pHandler->server_list[_pHandler->server_current_index];
	printf("\r\nmpush connect %s:%s\r\n", 
				server_info->str_server_addr,server_info->str_server_port);
	if (strlen(server_info->str_server_addr) > 0 && strlen(server_info->str_server_port) > 0)
	{
		_pHandler->server_sock = sock_connect(server_info->str_server_addr, server_info->str_server_port);
		if (_pHandler->server_sock == INVALID_SOCK)
		{
			printf("\r\nmpush sock_connect %s:%s failed\r\n", 
				server_info->str_server_addr,server_info->str_server_port);
			return MPUSH_ERROR_CONNECT_FAIL;
		}
		sock_set_nonblocking(_pHandler->server_sock);
	}
	else
	{
		return MPUSH_ERROR_INVALID_PARAMS;
	}

	return MPUSH_ERROR_CONNECT_OK;
}


static int _mpush_handshake(MPUSH_CLIENT_HANDLER_t *_pHandler)
{
	int ret = 0;
	size_t len = 0;
	MPUSH_MSG_HEADER_T head = {0};
	
	mpush_make_handshake_msg(_pHandler->str_comm_buf, &len, &_pHandler->msg_cfg, _pHandler->rsa);
	
	ret = sock_write(_pHandler->server_sock, _pHandler->str_comm_buf, len);
	if (len != ret)
	{
		printf("sock_write failed,ret=%d\r\n", ret);
		return MPUSH_ERROR_HANDSHAKE_FAIL;
	}
	//printf("sock_write success,ret=%d\r\n", ret);

	return MPUSH_ERROR_HANDSHAKE_OK;
}

static int _mpush_heartbeat(MPUSH_CLIENT_HANDLER_t *_pHandler)
{
	int ret = 0;
	size_t len = 0;
	MPUSH_MSG_HEADER_T head = {0};
	
	mpush_make_heartbeat_msg(_pHandler->str_comm_buf, &len);
	
	ret = sock_write(_pHandler->server_sock, _pHandler->str_comm_buf, len);
	if (len != ret)
	{
		printf("sock_write failed,ret=%d\r\n", ret);
		return MPUSH_ERROR_HEART_BEAT_FAIL;
	}
	//printf("sock_write success,ret=%d\r\n", ret);

	return MPUSH_ERROR_HEART_BEAT_OK;
}

static int _mpush_bind_user(MPUSH_CLIENT_HANDLER_t *_pHandler)
{
	int ret = 0;
	size_t len = 0;
	MPUSH_MSG_HEADER_T head = {0};
	
	mpush_make_bind_user_msg(_pHandler->str_comm_buf, &len, &_pHandler->msg_cfg, _pHandler->rsa);
	
	ret = sock_write(_pHandler->server_sock, _pHandler->str_comm_buf, len);
	if (len != ret)
	{
		printf("sock_write failed,ret=%d\r\n", ret);
		return MPUSH_ERROR_BIND_USER_FAIL;
	}
	//printf("sock_write success,ret=%d\r\n", ret);

	return MPUSH_ERROR_BIND_USER_OK;
}

void mpush_client_process_msg(
	MPUSH_CLIENT_HANDLER_t *_pHandler, int msg_type, MPUSH_MSG_HEADER_T *_head)
{
	switch (msg_type)
	{
		case MPUSH_MSG_CMD_HEARTBEAT:
		{
			break;
		}
	    case MPUSH_MSG_CMD_HANDSHAKE:
		{
			printf("recv MPUSH_MSG_CMD_HANDSHAKE reply\r\n");
			mpush_set_run_status(_pHandler, MPUSH_STAUTS_HANDSHAK_OK);
			break;
		}
	    case MPUSH_MSG_CMD_LOGIN:
		{
			printf("recv MPUSH_MSG_CMD_LOGIN reply\r\n");
			break;
		}
	    case MPUSH_MSG_CMD_LOGOUT:
		{
			printf("recv MPUSH_MSG_CMD_LOGOUT reply\r\n");
			break;
		}
	    case MPUSH_MSG_CMD_BIND:
		{
			printf("recv MPUSH_MSG_CMD_BIND reply\r\n");
			mpush_set_run_status(_pHandler, MPUSH_STAUTS_BINDING_OK);
			break;
		}
	    case MPUSH_MSG_CMD_UNBIND:
		{
			printf("recv MPUSH_MSG_CMD_UNBIND reply\r\n");
			break;
		}
	    case MPUSH_MSG_CMD_FAST_CONNECT:
		{
			printf("recv MPUSH_MSG_CMD_FAST_CONNECT reply\r\n");
			break;
		}
	    case MPUSH_MSG_CMD_PAUSE:
		{
			printf("recv MPUSH_MSG_CMD_PAUSE reply\r\n");
			break;
		}
	    case MPUSH_MSG_CMD_RESUME:
		{
			printf("recv MPUSH_MSG_CMD_RESUME reply\r\n");
			break;
		}
	    case MPUSH_MSG_CMD_ERROR:
		{
			printf("recv MPUSH_MSG_CMD_ERROR reply\r\n");
			break;
		}
	    case MPUSH_MSG_CMD_OK:
		{
			printf("recv MPUSH_MSG_CMD_OK reply\r\n");
			break;
		}
	    case MPUSH_MSG_CMD_HTTP_PROXY:
		{
			printf("recv MPUSH_MSG_CMD_HTTP_PROXY reply\r\n");
			break;
		}
	    case MPUSH_MSG_CMD_KICK:
		{
			printf("recv MPUSH_MSG_CMD_KICK reply\r\n");
			break;
		}
	    case MPUSH_MSG_CMD_GATEWAY_KICK:
		{
			printf("recv MPUSH_MSG_CMD_GATEWAY_KICK reply\r\n");
			break;
		}
	    case MPUSH_MSG_CMD_PUSH:
		{
			printf("recv MPUSH_MSG_CMD_PUSH reply\r\n");
			MPUSH_CLIENT_MSG_INFO_T *from_msg = _pHandler->str_push_msg;
			
			char buf[32] = {0};
			size_t len = 0;
			mpush_make_ack_msg(buf, &len, _head->sessionId);
			if (sock_writen_with_timeout(_pHandler->server_sock, buf, len, 1000) != len)
			{
				printf("sock_writen_with_timeout MPUSH_MSG_CMD_PUSH ack failed\r\n");
			}
			
			if (strlen(from_msg->msg_content) > 0)
			{
				MPUSH_CLIENT_MSG_INFO_T *to_msg = esp32_malloc(sizeof(MPUSH_CLIENT_MSG_INFO_T));

				if (to_msg != NULL)
				{
					memcpy(to_msg, from_msg, sizeof(MPUSH_CLIENT_MSG_INFO_T));
					if (xQueueSend(g_mpush_msg_queue, &to_msg, 0) != pdTRUE)
					{
						esp32_free(to_msg);
					}
				}
			}
			
			break;
		}
	    case MPUSH_MSG_CMD_GATEWAY_PUSH:
		{
			printf("recv MPUSH_MSG_CMD_GATEWAY_PUSH reply\r\n");
			break;
		}
	    case MPUSH_MSG_CMD_NOTIFICATION:
		{
			printf("recv MPUSH_MSG_CMD_NOTIFICATION reply\r\n");
			break;
		}
	    case MPUSH_MSG_CMD_GATEWAY_NOTIFICATION:
		{
			printf("recv MPUSH_MSG_CMD_GATEWAY_NOTIFICATION reply\r\n");
			break;
		}
	    case MPUSH_MSG_CMD_CHAT:
		{
			printf("recv MPUSH_MSG_CMD_CHAT reply\r\n");
			break;
		}
	    case MPUSH_MSG_CMD_GATEWAY_CHAT:
		{
			printf("recv MPUSH_MSG_CMD_GATEWAY_CHAT reply\r\n");
			break;
		}
	    case MPUSH_MSG_CMD_GROUP:
		{
			printf("recv MPUSH_MSG_CMD_GROUP reply\r\n");
			break;
		}
	    case MPUSH_MSG_CMD_GATEWAY_GROUP:
		{
			printf("recv MPUSH_MSG_CMD_GATEWAY_GROUP reply\r\n");
			break;
		}
	    case MPUSH_MSG_CMD_ACK:
		{
			printf("recv MPUSH_MSG_CMD_ACK reply\r\n");
			break;
		}
		case MPUSH_HEARTBEAT_BYTE_REPLY:
		{
			printf("recv MPUSH_HEARTBEAT reply\r\n");
			break;
		}
	    case MPUSH_MSG_CMD_UNKNOWN:
		{
			printf("recv MPUSH_MSG_CMD_UNKNOWN reply\r\n");
			break;
		}
		default:
			printf("recv MPUSH_MSG_CMD_UNKNOWN(%d) reply\r\n", msg_type);
			break;
	}	
}

void mpush_client_receive(void *_handler)
{
	fd_set readfds;
	MPUSH_MSG_HEADER_T msg_head = {0};
	struct timeval timeout;
	int sock = INVALID_SOCK;
	int ret = 0;
	MPUSH_STATUS_T status = MPUSH_STAUTS_INIT;
	MPUSH_SERVICE_T *service = (MPUSH_SERVICE_T *)_handler;
	MPUSH_CLIENT_HANDLER_t *pHandler = (MPUSH_CLIENT_HANDLER_t *)service->_mpush_handler;
	
	while (1)
	{
		if (service->Based._blocking == 1)
		{
			vTaskDelay(2000);
			continue;
		}
		
		vTaskDelay(100);
		sock = mpush_get_server_sock(pHandler);
		if (sock == INVALID_SOCK)
		{
			vTaskDelay(100);
			continue;
		}

		FD_ZERO(&readfds);
		timeout.tv_sec  = 2;
		timeout.tv_usec = 0;
		FD_SET(sock, &readfds);

		int ret = select(sock + 1, &readfds, NULL, NULL, &timeout);
		if (ret == -1) 
		{
			printf("select failed(%d)\r\n", errno);
			vTaskDelay(100);
			continue;
		} 
		else if (ret == 0) 
		{
			vTaskDelay(100);
			continue;
		}
		else
		{
			if (FD_ISSET(sock, &readfds))
			{
				printf("mpush msg is arrived\r\n");
			}
			else
			{
				continue;			
			}
		}

		char *recv_buf = (char *)&msg_head;
		
		//recv msg header
		memset((char*)&msg_head, 0, sizeof(msg_head));

		//recv one byte
		ret = sock_readn(sock, recv_buf, 1);
		if (ret != 1)
		{
			printf("sock_readn msg head first byte failed, ret = %d\r\n", ret);
			continue;
		}

		if (*recv_buf != 0)
		{
			mpush_client_process_msg(pHandler, *recv_buf, &msg_head);
			continue;
		}

		//recv other byte
		ret = sock_readn(sock, recv_buf + 1, sizeof(msg_head) - 1);
		if (ret != sizeof(msg_head) - 1)
		{
			printf("sock_readn msg head last 12 bytes failed, ret = %d\r\n", ret);
			continue;
		}

		convert_msg_head_to_host(&msg_head);
		print_msg_head(&msg_head);
		
		memset(pHandler->str_recv_buf, 0, sizeof(pHandler->str_recv_buf));
		//recv msg body
		if (msg_head.length <= sizeof(pHandler->str_recv_buf))
		{
			ret = sock_readn(sock, pHandler->str_recv_buf, msg_head.length);
			if (ret != msg_head.length)
			{
				printf("sock_readn msg body failed, ret = %d\r\n", ret);
				continue;
			}
		}
		else
		{
			printf("msg is length is too large,length=%d\r\n", msg_head.length);
			int send_len = sizeof(pHandler->str_recv_buf);
			int send_total_len = 0;
			while (send_len > 0)
			{
				ret = sock_readn(sock, pHandler->str_recv_buf, send_len);
				if (ret != msg_head.length)
				{
					printf("sock_readn msg body failed, ret = %d\r\n", ret);
					break;
				}
				else
				{
					send_total_len += ret;
					send_len = msg_head.length - send_total_len;
					if (send_len >= sizeof(pHandler->str_recv_buf))
					{
						send_len = sizeof(pHandler->str_recv_buf);
					}
				}
			}
			continue;
		}		

		ret = mpush_msg_decode(&msg_head, pHandler->str_recv_buf, 
			pHandler->str_push_msg, sizeof(pHandler->str_push_msg));
		mpush_client_process_msg(pHandler, ret, &msg_head);
	}

	vTaskDelete(NULL);
}

static int _mpush_client_get_device_sn(MPUSH_CLIENT_HANDLER_t *_pHandler)
{
	char	domain[64]	= {0};
	char	port[6] 	= {0};
	char	params[128] = {0};
	char	str_nonce[64] = {0};
	char	str_timestamp[64] = {0};
	char	str_private_key[64] = {0};
	char	str_device_id[32] = {0};
	char	str_buf[512]= {0};
	sock_t	sock		= INVALID_SOCK;
	cJSON	*pJson = NULL;
	
	if (_pHandler == NULL)
	{
		goto _mpush_client_get_device_sn_error;
	}
	
	if (sock_get_server_info(MPUSH_GET_OPEN_SERVICE_URL, &domain, &port, &params) != 0)
	{
		printf("sock_get_server_info fail\r\n");
		goto _mpush_client_get_device_sn_error;
	}

	sock = sock_connect(domain, port);
	if (sock == INVALID_SOCK)
	{
		ESP_LOGE(PRINT_TAG, "sock_connect fail,%s,%s\r\n", domain, port);
		goto _mpush_client_get_device_sn_error;
	}
	else
	{
		ESP_LOGE(PRINT_TAG, "sock_connect success,%s,%s\r\n", domain, port);
	}

	sock_set_nonblocking(sock);
	// make http head
	device_get_mac_str(str_device_id, sizeof(str_device_id));
	crypto_generate_request_id(str_nonce, sizeof(str_nonce));
	snprintf(str_buf, sizeof(str_buf), 
		"{\"app_id\": \"%s\","
  		"\"content\":" 
		"{\"projectCode\": \"%s\",\"mac\": \"%s\"},"
  		"\"device_id\": \"%s\","
  		"\"request_id\": \"%s\","
  		"\"robot_id\": \"%s\","
  		"\"service\": \"generateDeviceSNService\","
  		"\"user_id\": \"%s\","
 		"\"version\": \"2.0\"}", 
 		DEEP_BRAIN_APP_ID, DEVICE_SN_PREFIX, str_device_id, 
 		str_device_id, str_nonce, DEEP_BRAIN_ROBOT_ID, str_device_id);
	
	crypto_generate_nonce((uint8_t *)str_nonce, sizeof(str_nonce));
	crypto_time_stamp((unsigned char*)str_timestamp, sizeof(str_timestamp));
	crypto_generate_private_key((uint8_t *)str_private_key, sizeof(str_private_key), str_nonce, str_timestamp, DEEP_BRAIN_ROBOT_ID);
	snprintf(_pHandler->str_comm_buf, sizeof(_pHandler->str_comm_buf), 
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
		"Connection:close\r\n\r\n%s", 
		params, domain, port, strlen(str_buf), str_nonce, str_timestamp, str_private_key, DEEP_BRAIN_ROBOT_ID, str_buf);

	//ESP_LOGE(PRINT_TAG, "%s", _pHandler->str_comm_buf);
	if (sock_writen_with_timeout(sock, _pHandler->str_comm_buf, strlen(_pHandler->str_comm_buf), 1000) != strlen(_pHandler->str_comm_buf)) 
	{
		printf("sock_writen http header fail\r\n");
		goto _mpush_client_get_device_sn_error;
	}

	/* Read HTTP response */
	memset(_pHandler->str_comm_buf, 0, sizeof(_pHandler->str_comm_buf));
	sock_readn_with_timeout(sock, _pHandler->str_comm_buf, sizeof(_pHandler->str_comm_buf) - 1, 2000);
	sock_close(sock);
	sock = INVALID_SOCK;

	if (http_get_error_code(_pHandler->str_comm_buf) == 200)
	{	
		char* pBody = http_get_body(_pHandler->str_comm_buf);
		if (NULL != pBody)
		{
			pJson = cJSON_Parse(pBody);
			if (NULL != pJson) 
			{
				cJSON *pJson_status = cJSON_GetObjectItem(pJson, "statusCode");
				if (NULL == pJson_status || pJson_status->valuestring == NULL)
				{
					ESP_LOGE(PRINT_TAG, "statusCode not found");
					goto _mpush_client_get_device_sn_error;
				}
				
				if (strncasecmp(pJson_status->valuestring, "OK", 2) != 0)
				{
					ESP_LOGE(PRINT_TAG, "statusCode:%s", pJson_status->valuestring);
					goto _mpush_client_get_device_sn_error;
				}
	
				cJSON *pJson_content = cJSON_GetObjectItem(pJson, "content");
				if (NULL == pJson_content)
				{
					ESP_LOGE(PRINT_TAG, "content not found");
					goto _mpush_client_get_device_sn_error;
				}

				cJSON *pJson_sn = cJSON_GetObjectItem(pJson_content, "sn");
				if (NULL == pJson_sn
					|| pJson_sn->valuestring == NULL
					|| strlen(pJson_sn->valuestring) <= 0)
				{
					ESP_LOGE(PRINT_TAG, "sn not found");
					goto _mpush_client_get_device_sn_error;
				}

				ESP_LOGE(PRINT_TAG, "get new sn:%s", pJson_sn->valuestring);
				if (set_flash_cfg(FLASH_CFG_DEVICE_ID, pJson_sn->valuestring) == ESP_OK)
				{
					snprintf((char*)&_pHandler->msg_cfg.str_device_id, sizeof(_pHandler->msg_cfg.str_device_id), "%s", pJson_sn->valuestring);
					DEBUG_LOGI(PRINT_TAG, "save device id[%s] success", pJson_sn->valuestring);
				}
				else
				{
					DEBUG_LOGE(PRINT_TAG, "save device id[%s] fail", pJson_sn->valuestring);
					goto _mpush_client_get_device_sn_error;
				}
			}
			else
			{
				ESP_LOGE(PRINT_TAG, "#1#%s", _pHandler->str_comm_buf);
			}
			
			if (NULL != pJson)
			{
				cJSON_Delete(pJson);
				pJson = NULL;
			}
		}
	}
	else
	{
		ESP_LOGE(PRINT_TAG, "#2#%s", _pHandler->str_comm_buf);
		goto _mpush_client_get_device_sn_error;
	}
	
	return MPUSH_ERROR_GET_SN_OK;
	
_mpush_client_get_device_sn_error:
	
	if (sock != INVALID_SOCK)
	{
		sock_close(sock);
	}

	if (NULL != pJson)
	{
		cJSON_Delete(pJson);
		pJson = NULL;
	}
	
	return MPUSH_ERROR_GET_SN_FAIL;
}

static int _mpush_client_get_acount_info(MPUSH_CLIENT_HANDLER_t *_pHandler)
{
	char	domain[64]	= {0};
	char	port[6] 	= {0};
	char	params[128] = {0};
	char	str_nonce[64] = {0};
	char	str_timestamp[64] = {0};
	char	str_private_key[64] = {0};
	char	str_device_id[32] = {0};
	char	str_buf[512]= {0};
	sock_t	sock		= INVALID_SOCK;
	cJSON	*pJson = NULL;
	
	if (_pHandler == NULL)
	{
		goto _mpush_client_get_acount_info_error;
	}
	
	if (sock_get_server_info(MPUSH_GET_OPEN_SERVICE_URL, &domain, &port, &params) != 0)
	{
		printf("sock_get_server_info fail\r\n");
		goto _mpush_client_get_acount_info_error;
	}

	sock = sock_connect(domain, port);
	if (sock == INVALID_SOCK)
	{
		ESP_LOGE(PRINT_TAG, "sock_connect fail,%s,%s\r\n", domain, port);
		goto _mpush_client_get_acount_info_error;
	}
	else
	{
		ESP_LOGE(PRINT_TAG, "sock_connect success,%s,%s\r\n", domain, port);
	}

	sock_set_nonblocking(sock);
	// make http head
	get_flash_cfg(FLASH_CFG_DEVICE_ID, str_device_id);
	crypto_generate_request_id(str_nonce, sizeof(str_nonce));
	snprintf(str_buf, sizeof(str_buf), 
		"{\"app_id\": \"%s\","
  		"\"content\":{},"
  		"\"device_id\": \"%s\","
  		"\"request_id\": \"%s\","
  		"\"robot_id\": \"%s\","
  		"\"service\": \"initResourceService\","
  		"\"user_id\": \"%s\","
 		"\"version\": \"2.0\"}", 
 		DEEP_BRAIN_APP_ID, 
 		str_device_id, str_nonce, DEEP_BRAIN_ROBOT_ID, str_device_id);
	
	crypto_generate_nonce((uint8_t *)str_nonce, sizeof(str_nonce));
	crypto_time_stamp((unsigned char*)str_timestamp, sizeof(str_timestamp));
	crypto_generate_private_key((uint8_t *)str_private_key, sizeof(str_private_key), str_nonce, str_timestamp, DEEP_BRAIN_ROBOT_ID);
	snprintf(_pHandler->str_comm_buf, sizeof(_pHandler->str_comm_buf), 
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
		"Connection:close\r\n\r\n%s", 
		params, domain, port, strlen(str_buf), str_nonce, str_timestamp, str_private_key, DEEP_BRAIN_ROBOT_ID, str_buf);

	//ESP_LOGE(PRINT_TAG, "%s", _pHandler->str_comm_buf);
	if (sock_writen_with_timeout(sock, _pHandler->str_comm_buf, strlen(_pHandler->str_comm_buf), 1000) != strlen(_pHandler->str_comm_buf)) 
	{
		printf("sock_writen http header fail\r\n");
		goto _mpush_client_get_acount_info_error;
	}

	/* Read HTTP response */
	memset(_pHandler->str_comm_buf, 0, sizeof(_pHandler->str_comm_buf));
	sock_readn_with_timeout(sock, _pHandler->str_comm_buf, sizeof(_pHandler->str_comm_buf) - 1, 2000);
	sock_close(sock);
	sock = INVALID_SOCK;
	
	//ESP_LOGE(PRINT_TAG, "#2#%s", _pHandler->str_comm_buf);
	memset(&_pHandler->ai_acounts, 0, sizeof(_pHandler->ai_acounts));
	if (http_get_error_code(_pHandler->str_comm_buf) == 200)
	{	
		char* pBody = http_get_body(_pHandler->str_comm_buf);
		if (NULL != pBody)
		{
			pJson = cJSON_Parse(pBody);
			if (NULL != pJson) 
			{
				cJSON *pJson_status = cJSON_GetObjectItem(pJson, "statusCode");
				if (NULL == pJson_status || pJson_status->valuestring == NULL)
				{
					ESP_LOGE(PRINT_TAG, "statusCode not found");
					goto _mpush_client_get_acount_info_error;
				}
				
				if (strncasecmp(pJson_status->valuestring, "OK", 2) != 0)
				{
					ESP_LOGE(PRINT_TAG, "statusCode:%s", pJson_status->valuestring);
					goto _mpush_client_get_acount_info_error;
				}
	
				cJSON *pJson_content = cJSON_GetObjectItem(pJson, "content");
				if (NULL == pJson_content)
				{
					ESP_LOGE(PRINT_TAG, "content not found");
					goto _mpush_client_get_acount_info_error;
				}

				cJSON *pJson_string = cJSON_GetObjectItem(pJson_content, "BAIDU_DEFAULT_ASR_URL");
				if (NULL == pJson_string
					|| pJson_string->valuestring == NULL
					|| strlen(pJson_string->valuestring) <= 0)
				{
					ESP_LOGE(PRINT_TAG, "BAIDU_DEFAULT_ASR_URL not found");
					goto _mpush_client_get_acount_info_error;
				}
				ESP_LOGE(PRINT_TAG, "BAIDU_DEFAULT_ASR_URL:%s", pJson_string->valuestring);
				snprintf(_pHandler->ai_acounts.st_baidu_acount.asr_url, sizeof(_pHandler->ai_acounts.st_baidu_acount.asr_url),
					"%s", pJson_string->valuestring);
				
				pJson_string = cJSON_GetObjectItem(pJson_content, "BAIDU_DEFAULT_TTS_URL");
				if (NULL == pJson_string
					|| pJson_string->valuestring == NULL
					|| strlen(pJson_string->valuestring) <= 0)
				{
					ESP_LOGE(PRINT_TAG, "BAIDU_DEFAULT_TTS_URL not found");
					goto _mpush_client_get_acount_info_error;
				}
				ESP_LOGE(PRINT_TAG, "BAIDU_DEFAULT_TTS_URL:%s", pJson_string->valuestring);
				snprintf(_pHandler->ai_acounts.st_baidu_acount.tts_url, sizeof(_pHandler->ai_acounts.st_baidu_acount.tts_url),
					"%s", pJson_string->valuestring);

				pJson_string = cJSON_GetObjectItem(pJson_content, "BAIDU_DEFAULT_APP_ID");
				if (NULL == pJson_string
					|| pJson_string->valuestring == NULL
					|| strlen(pJson_string->valuestring) <= 0)
				{
					ESP_LOGE(PRINT_TAG, "BAIDU_DEFAULT_APP_ID not found");
					goto _mpush_client_get_acount_info_error;
				}
				ESP_LOGE(PRINT_TAG, "BAIDU_DEFAULT_APP_ID:%s", pJson_string->valuestring);
				snprintf(_pHandler->ai_acounts.st_baidu_acount.app_id, sizeof(_pHandler->ai_acounts.st_baidu_acount.app_id),
					"%s", pJson_string->valuestring);

				pJson_string = cJSON_GetObjectItem(pJson_content, "BAIDU_DEFAULT_APP_KEY");
				if (NULL == pJson_string
					|| pJson_string->valuestring == NULL
					|| strlen(pJson_string->valuestring) <= 0)
				{
					ESP_LOGE(PRINT_TAG, "BAIDU_DEFAULT_APP_KEY not found");
					goto _mpush_client_get_acount_info_error;
				}
				ESP_LOGE(PRINT_TAG, "BAIDU_DEFAULT_APP_KEY:%s", pJson_string->valuestring);
				snprintf(_pHandler->ai_acounts.st_baidu_acount.app_key, sizeof(_pHandler->ai_acounts.st_baidu_acount.app_key),
					"%s", pJson_string->valuestring);

				pJson_string = cJSON_GetObjectItem(pJson_content, "BAIDU_DEFAULT_SECRET_KEY");
				if (NULL == pJson_string
					|| pJson_string->valuestring == NULL
					|| strlen(pJson_string->valuestring) <= 0)
				{
					ESP_LOGE(PRINT_TAG, "BAIDU_DEFAULT_SECRET_KEY not found");
					goto _mpush_client_get_acount_info_error;
				}
				ESP_LOGE(PRINT_TAG, "BAIDU_DEFAULT_SECRET_KEY:%s", pJson_string->valuestring);
				snprintf(_pHandler->ai_acounts.st_baidu_acount.secret_key, sizeof(_pHandler->ai_acounts.st_baidu_acount.secret_key),
					"%s", pJson_string->valuestring);

				pJson_string = cJSON_GetObjectItem(pJson_content, "SINOVOICE_DEFAULT_ASR_URL");
				if (NULL == pJson_string
					|| pJson_string->valuestring == NULL
					|| strlen(pJson_string->valuestring) <= 0)
				{
					ESP_LOGE(PRINT_TAG, "SINOVOICE_DEFAULT_ASR_URL not found");
					goto _mpush_client_get_acount_info_error;
				}
				ESP_LOGE(PRINT_TAG, "SINOVOICE_DEFAULT_ASR_URL:%s", pJson_string->valuestring);
				snprintf(_pHandler->ai_acounts.st_sinovoice_acount.asr_url, sizeof(_pHandler->ai_acounts.st_sinovoice_acount.asr_url),
					"%s", pJson_string->valuestring);

				pJson_string = cJSON_GetObjectItem(pJson_content, "SINOVOICE_DEFAULT_TTS_URL");
				if (NULL == pJson_string
					|| pJson_string->valuestring == NULL
					|| strlen(pJson_string->valuestring) <= 0)
				{
					ESP_LOGE(PRINT_TAG, "SINOVOICE_DEFAULT_TTS_URL not found");
					goto _mpush_client_get_acount_info_error;
				}
				ESP_LOGE(PRINT_TAG, "SINOVOICE_DEFAULT_TTS_URL:%s", pJson_string->valuestring);
				snprintf(_pHandler->ai_acounts.st_sinovoice_acount.tts_url, sizeof(_pHandler->ai_acounts.st_sinovoice_acount.tts_url),
					"%s", pJson_string->valuestring);

				pJson_string = cJSON_GetObjectItem(pJson_content, "SINOVOICE_DEFAULT_APP_KEY");
				if (NULL == pJson_string
					|| pJson_string->valuestring == NULL
					|| strlen(pJson_string->valuestring) <= 0)
				{
					ESP_LOGE(PRINT_TAG, "SINOVOICE_DEFAULT_APP_KEY not found");
					goto _mpush_client_get_acount_info_error;
				}
				ESP_LOGE(PRINT_TAG, "SINOVOICE_DEFAULT_APP_KEY:%s", pJson_string->valuestring);
				snprintf(_pHandler->ai_acounts.st_sinovoice_acount.app_key, sizeof(_pHandler->ai_acounts.st_sinovoice_acount.app_key),
					"%s", pJson_string->valuestring);
				
				pJson_string = cJSON_GetObjectItem(pJson_content, "SINOVOICE_DEFAULT_DEV_KEY");
				if (NULL == pJson_string
					|| pJson_string->valuestring == NULL
					|| strlen(pJson_string->valuestring) <= 0)
				{
					ESP_LOGE(PRINT_TAG, "SINOVOICE_DEFAULT_DEV_KEY not found");
					goto _mpush_client_get_acount_info_error;
				}
				ESP_LOGE(PRINT_TAG, "SINOVOICE_DEFAULT_DEV_KEY:%s", pJson_string->valuestring);
				snprintf(_pHandler->ai_acounts.st_sinovoice_acount.dev_key, sizeof(_pHandler->ai_acounts.st_sinovoice_acount.dev_key),
					"%s", pJson_string->valuestring);

				set_ai_acount(AI_ACOUNT_ALL, &_pHandler->ai_acounts);
			}
			else
			{
				ESP_LOGE(PRINT_TAG, "#1#%s", _pHandler->str_comm_buf);
			}
			
			if (NULL != pJson)
			{
				cJSON_Delete(pJson);
				pJson = NULL;
			}
		}
	}
	else
	{
		ESP_LOGE(PRINT_TAG, "#2#%s", _pHandler->str_comm_buf);
		goto _mpush_client_get_acount_info_error;
	}
	
	return MPUSH_ERROR_GET_ACOUNT_OK;
	
_mpush_client_get_acount_info_error:
	
	if (sock != INVALID_SOCK)
	{
		sock_close(sock);
	}

	if (NULL != pJson)
	{
		cJSON_Delete(pJson);
		pJson = NULL;
	}
	
	return MPUSH_ERROR_GET_ACOUNT_FAIL;
}


static int _mpush_client_get_server_list(MPUSH_CLIENT_HANDLER_t *_pHandler)
{
	char	domain[64]	= {0};
	char	port[6] 	= {0};
	char	params[128] = {0};
	char 	str_nonce[64] = {0};
	char 	str_timestamp[64] = {0};
	char 	str_private_key[64] = {0};
	char 	str_device_id[32] = {0};
	char	str_buf[512]= {0};
	sock_t	sock		= INVALID_SOCK;
	cJSON 	*pJson = NULL;
	
	if (_pHandler == NULL)
	{
		goto mpush_client_get_server_list_error;
	}
	
	if (sock_get_server_info(MPUSH_GET_SERVER_URL, &domain, &port, &params) != 0)
	{
		printf("sock_get_server_info fail\r\n");
		goto mpush_client_get_server_list_error;
	}

	sock = sock_connect(domain, port);
	if (sock == INVALID_SOCK)
	{
		ESP_LOGE(PRINT_TAG, "sock_connect fail,%s,%s\r\n", domain, port);
		goto mpush_client_get_server_list_error;
	}
	else
	{
		ESP_LOGE(PRINT_TAG, "sock_connect success,%s,%s\r\n", domain, port);
	}

	sock_set_nonblocking(sock);
	// make http head
	get_flash_cfg(FLASH_CFG_DEVICE_ID, str_device_id);		
	crypto_generate_nonce((uint8_t *)str_nonce, sizeof(str_nonce));
	crypto_time_stamp((unsigned char*)str_timestamp, sizeof(str_timestamp));
	crypto_generate_private_key((uint8_t *)str_private_key, sizeof(str_private_key), str_nonce, str_timestamp, DEEP_BRAIN_ROBOT_ID);
	snprintf(_pHandler->str_comm_buf, sizeof(_pHandler->str_comm_buf), 
		"POST %s HTTP/1.0\r\n"
		"Host: %s:%s\r\n"
		"Accept: application/json\r\n"
		"Accept-Language: zh-cn\r\n"
		"Content-Length: 0\r\n"
		"Content-Type: application/json\r\n"
		"Nonce: %s\r\n"
		"CreatedTime: %s\r\n"
		"PrivateKey: %s\r\n"
		"Key: %s\r\n"
		"Connection:close\r\n\r\n", 
		params, domain, port, str_nonce, str_timestamp, str_private_key, DEEP_BRAIN_ROBOT_ID);

	//ESP_LOGE(PRINT_TAG, "%s", _pHandler->str_comm_buf);
	if (sock_writen_with_timeout(sock, _pHandler->str_comm_buf, strlen(_pHandler->str_comm_buf), 1000) != strlen(_pHandler->str_comm_buf)) 
	{
		printf("sock_writen http header fail\r\n");
		goto mpush_client_get_server_list_error;
	}

	/* Read HTTP response */
	memset(_pHandler->str_comm_buf, 0, sizeof(_pHandler->str_comm_buf));
	sock_readn_with_timeout(sock, _pHandler->str_comm_buf, sizeof(_pHandler->str_comm_buf) - 1, 1000);
	sock_close(sock);
	sock = INVALID_SOCK;

	if (http_get_error_code(_pHandler->str_comm_buf) == 200)
	{	
		char* pBody = http_get_body(_pHandler->str_comm_buf);
		//snprintf(str_buf, sizeof(str_buf), "{\"serverlist\": [{\"server\": \"192.168.20.91\",\"port\": \"3000\"},{\"server\": \"192.168.20.91\",\"port\": \"3000\"}]}");
		//pBody = &str_buf;
		if (NULL != pBody)
		{
			pJson = cJSON_Parse(pBody);
		    if (NULL != pJson) 
			{
				cJSON *pJson_status = cJSON_GetObjectItem(pJson, "statusCode");
				if (NULL == pJson_status || pJson_status->valuestring == NULL)
				{
					ESP_LOGE(PRINT_TAG, "statusCode not found");
					goto mpush_client_get_server_list_error;
				}
				
				if (strncasecmp(pJson_status->valuestring, "OK", 2) != 0)
				{
					ESP_LOGE(PRINT_TAG, "statusCode:%s", pJson_status->valuestring);
					goto mpush_client_get_server_list_error;
				}
	
		        cJSON *pJson_content = cJSON_GetObjectItem(pJson, "content");
				if (NULL == pJson_content)
				{
					ESP_LOGE(PRINT_TAG, "content not found");
					goto mpush_client_get_server_list_error;
				}

				cJSON *pJson_data = cJSON_GetObjectItem(pJson_content, "data");
				if (NULL == pJson_data)
				{
					ESP_LOGE(PRINT_TAG, "data not found");
					goto mpush_client_get_server_list_error;
				}

				int list_size = cJSON_GetArraySize(pJson_data);
				if (list_size <= 0)
				{
					ESP_LOGE(PRINT_TAG, "list_size is 0");
					goto mpush_client_get_server_list_error;
				}

				int i = 0;
				int n = 0;
				for (i = 0; i < list_size && n < MPUSH_SERVER_NUM; i++)
				{
					cJSON *pJson_item = cJSON_GetArrayItem(pJson_data, i);
					cJSON *pJson_server = cJSON_GetObjectItem(pJson_item, "ip");
					cJSON *pJson_port = cJSON_GetObjectItem(pJson_item, "port");
					if (NULL == pJson_server
						|| NULL == pJson_server->valuestring
						|| NULL == pJson_port
						|| NULL == pJson_port->valuestring
						|| strlen(pJson_server->valuestring) <= 0
						|| strlen(pJson_port->valuestring) <= 0)
					{
						continue;
					}

					_pHandler->server_list[n].isValid = 1;
					snprintf(_pHandler->server_list[n].str_server_addr, sizeof(_pHandler->server_list[n].str_server_addr),
						"%s", pJson_server->valuestring);
					snprintf(_pHandler->server_list[n].str_server_port, sizeof(_pHandler->server_list[n].str_server_port),
					"%s", pJson_port->valuestring);
					ESP_LOGI(PRINT_TAG, "server:%s\tport:%s", 
						pJson_server->valuestring, pJson_port->valuestring);
					n++;
				}

				if (n <= 0)
				{
					goto mpush_client_get_server_list_error;
				}

				_pHandler->server_list_num = n;
		    }
			else
			{
				ESP_LOGE(PRINT_TAG, "#1#%s", _pHandler->str_comm_buf);
			}
			
			if (NULL != pJson)
			{
				cJSON_Delete(pJson);
				pJson = NULL;
			}
		}
	}
	else
	{
		ESP_LOGE(PRINT_TAG, "#2#%s", _pHandler->str_comm_buf);
		goto mpush_client_get_server_list_error;
	}
	
	return MPUSH_ERROR_GET_SERVER_OK;
	
mpush_client_get_server_list_error:
	
	if (sock != INVALID_SOCK)
	{
		sock_close(sock);
	}

	if (NULL != pJson)
	{
		cJSON_Delete(pJson);
		pJson = NULL;
	}
	
	return MPUSH_ERROR_GET_SERVER_FAIL;
}


int _mpush_client_get_offline_msg(MPUSH_CLIENT_HANDLER_t *_pHandler)
{
	char	domain[64]	= {0};
	char	port[6] 	= {0};
	char	params[128] = {0};
	char	str_nonce[64] = {0};
	char	str_timestamp[64] = {0};
	char	str_private_key[64] = {0};
	char	str_device_id[32] = {0};
	char	str_buf[512]= {0};
	sock_t	sock		= INVALID_SOCK;
	cJSON	*pJson = NULL;
	
	if (_pHandler == NULL)
	{
		return;
	}
	
	if (sock_get_server_info(MPUSH_GET_OFFLINE_MSG_URL, &domain, &port, &params) != 0)
	{
		printf("sock_get_server_info fail\r\n");
		goto mpush_client_get_offline_msg_error;
	}

	sock = sock_connect(domain, port);
	if (sock == INVALID_SOCK)
	{
		ESP_LOGE(PRINT_TAG, "sock_connect fail,%s,%s\r\n", domain, port);
		goto mpush_client_get_offline_msg_error;
	}
	else
	{
		ESP_LOGE(PRINT_TAG, "sock_connect success,%s,%s\r\n", domain, port);
	}

	sock_set_nonblocking(sock);
	// make http head
	crypto_generate_nonce((uint8_t *)str_nonce, sizeof(str_nonce));
	get_flash_cfg(FLASH_CFG_DEVICE_ID, str_device_id);
	snprintf(str_buf, sizeof(str_buf), 
		"{\"content\":" 
		"{\"fromUserId\": \"\",\"userId\": \"%s\"},"
		"\"osType\": \"esp32\","
		"\"protocolVersion\": \"2.0.0.0\","
		"\"userId\": \"%s\","
		"\"requestId\": \"%s\","
		"\"appId\": \"%s\","
		"\"robotId\": \"%s\"}", 
		str_device_id, str_device_id, str_nonce, DEEP_BRAIN_APP_ID, DEEP_BRAIN_ROBOT_ID);
	
	crypto_generate_nonce((uint8_t *)str_nonce, sizeof(str_nonce));
	crypto_time_stamp((unsigned char*)str_timestamp, sizeof(str_timestamp));
	crypto_generate_private_key((uint8_t *)str_private_key, sizeof(str_private_key), str_nonce, str_timestamp, DEEP_BRAIN_ROBOT_ID);
	snprintf(_pHandler->str_comm_buf, sizeof(_pHandler->str_comm_buf), 
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
		"Connection:close\r\n\r\n%s", 
		params, domain, port, strlen(str_buf), str_nonce, str_timestamp, str_private_key, DEEP_BRAIN_ROBOT_ID, str_buf);

	//ESP_LOGE(PRINT_TAG, "%s", _pHandler->str_comm_buf);
	if (sock_writen_with_timeout(sock, _pHandler->str_comm_buf, strlen(_pHandler->str_comm_buf), 1000) != strlen(_pHandler->str_comm_buf)) 
	{
		printf("sock_writen http header fail\r\n");
		goto mpush_client_get_offline_msg_error;
	}

	/* Read HTTP response */
	memset(_pHandler->str_comm_buf, 0, sizeof(_pHandler->str_comm_buf));
	sock_readn_with_timeout(sock, _pHandler->str_comm_buf, sizeof(_pHandler->str_comm_buf) - 1, 2000);
	sock_close(sock);
	sock = INVALID_SOCK;

	if (http_get_error_code(_pHandler->str_comm_buf) == 200)
	{	
		char* pBody = http_get_body(_pHandler->str_comm_buf);
		if (NULL != pBody)
		{
			pJson = cJSON_Parse(pBody);
			if (NULL != pJson) 
			{
				cJSON *pJson_status = cJSON_GetObjectItem(pJson, "statusCode");
				if (NULL == pJson_status || pJson_status->valuestring == NULL)
				{
					ESP_LOGE(PRINT_TAG, "statusCode not found");
					goto mpush_client_get_offline_msg_error;
				}
				
				if (strncasecmp(pJson_status->valuestring, "OK", 2) != 0)
				{
					ESP_LOGE(PRINT_TAG, "statusCode:%s", pJson_status->valuestring);
					goto mpush_client_get_offline_msg_error;
				}
	
				cJSON *pJson_content = cJSON_GetObjectItem(pJson, "content");
				if (NULL == pJson_content)
				{
					ESP_LOGE(PRINT_TAG, "content not found");
					goto mpush_client_get_offline_msg_error;
				}

				cJSON *pJson_data = cJSON_GetObjectItem(pJson_content, "data");
				if (NULL == pJson_data)
				{
					ESP_LOGE(PRINT_TAG, "data not found");
					goto mpush_client_get_offline_msg_error;
				}

				int list_size = cJSON_GetArraySize(pJson_data);
				if (list_size <= 0)
				{
					ESP_LOGE(PRINT_TAG, "list_size is 0, no offline msg");
				}
				else
				{
					int i = 0;
					for (i = 0; i < list_size; i++)
					{
						cJSON *pJson_item = cJSON_GetArrayItem(pJson_data, i);
						cJSON *pJson_msg_type = cJSON_GetObjectItem(pJson_item, "msgType");
						cJSON *pJson_data = cJSON_GetObjectItem(pJson_item, "data");
						if (NULL == pJson_msg_type
							|| NULL == pJson_msg_type->valuestring
							|| NULL == pJson_data
							|| NULL == pJson_data->valuestring
							|| strlen(pJson_msg_type->valuestring) <= 0
							|| strlen(pJson_data->valuestring) <= 0)
						{
							continue;
						}

						int msg_type = MPUSH_SEND_MSG_TYPE_TEXT;
						if (strncasecmp(pJson_msg_type->valuestring, "text", 4) == 0)
						{
							msg_type = MPUSH_SEND_MSG_TYPE_TEXT;
						}
						else if (strncasecmp(pJson_msg_type->valuestring, "hylink", 6) == 0)
						{
							msg_type = MPUSH_SEND_MSG_TYPE_LINK;
						}
						else
						{
							ESP_LOGE(PRINT_TAG, "not support msg type:%s", pJson_msg_type->valuestring);
							continue;
						}

						MPUSH_CLIENT_MSG_INFO_T *to_msg = esp32_malloc(sizeof(MPUSH_CLIENT_MSG_INFO_T));
						if (to_msg != NULL)
						{
							to_msg->msg_type = msg_type;
							snprintf(to_msg->msg_content, sizeof(to_msg->msg_content), "%s", pJson_data->valuestring);
							if (xQueueSend(g_mpush_msg_queue, &to_msg, 0) != pdTRUE)
							{
								esp32_free(to_msg);
							}
						}
					}
				}
			}
			else
			{
				ESP_LOGE(PRINT_TAG, "#1#%s", _pHandler->str_comm_buf);
			}
			
			if (NULL != pJson)
			{
				cJSON_Delete(pJson);
				pJson = NULL;
			}
		}
	}
	else
	{
		ESP_LOGE(PRINT_TAG, "#2#%s", _pHandler->str_comm_buf);
		goto mpush_client_get_offline_msg_error;
	}
	
	return MPUSH_ERROR_GET_OFFLINE_MSG_OK;
	
mpush_client_get_offline_msg_error:
	
	if (sock != INVALID_SOCK)
	{
		sock_close(sock);
	}

	if (NULL != pJson)
	{
		cJSON_Delete(pJson);
		pJson = NULL;
	}
	
	return MPUSH_ERROR_GET_OFFLINE_MSG_FAIL;
}

void mpush_client_process(void *_handler)
{
	MPUSH_STATUS_T status = MPUSH_STAUTS_INIT;
	MPUSH_SERVICE_T *service = (MPUSH_SERVICE_T *)_handler;
	MPUSH_CLIENT_HANDLER_t *pHandler = (MPUSH_CLIENT_HANDLER_t *)service->_mpush_handler;
	uint32_t heart_time = time(NULL);

	while (1)
	{
		if (service->Based._blocking == 1)
		{
			if (pHandler->server_sock != INVALID_SOCK)
			{
				sock_close(pHandler->server_sock);
				pHandler->server_sock = INVALID_SOCK;
			}
			mpush_set_run_status(pHandler, MPUSH_STAUTS_INIT);
			vTaskDelay(2000);
			continue;
		}
		/*
		while (get_ota_upgrade_state() == OTA_UPGRADE_STATE_UPGRADING)
		{
			vTaskDelay(3000);
		}
		*/
		
		status = mpush_get_run_status(pHandler);
		switch (status)
		{
			case MPUSH_STAUTS_INIT:
			{
				char device_sn[32] = {0};
				get_flash_cfg(FLASH_CFG_DEVICE_ID, device_sn);
				if (strlen(device_sn) > 0)
				{
					snprintf((char*)&pHandler->msg_cfg.str_device_id, sizeof(pHandler->msg_cfg.str_device_id), "%s", device_sn);
					mpush_set_run_status(pHandler, MPUSH_STAUTS_GET_ACOUNT_INFO);
					break;
				}
				
				//设备激活操作，获取设备唯一序列号
				if (_mpush_client_get_device_sn(pHandler) == MPUSH_ERROR_GET_SN_OK)
				{
					ESP_LOGE(PRINT_TAG, "_mpush_client_get_device_sn success");
					mpush_set_run_status(pHandler, MPUSH_STAUTS_GET_ACOUNT_INFO);
					//to do 增加语音提示，激活成功
					player_mdware_play_tone(FLASH_MUSIC_DYY_DEVICE_ACTIVATE_OK);
					vTaskDelay(2000);
				}
				else
				{
					ESP_LOGE(PRINT_TAG, "_mpush_client_get_device_sn fail");
					player_mdware_play_tone(FLASH_MUSIC_DYY_DEVICE_ACTIVATE_FAIL);
					device_sleep(10, 0);
				}
				break;
			}
			case MPUSH_STAUTS_GET_ACOUNT_INFO:
			{
				if (_mpush_client_get_acount_info(pHandler) == MPUSH_ERROR_GET_ACOUNT_OK)
				{
					printf("_mpush_client_get_acount_info success\r\n");
					mpush_set_run_status(pHandler, MPUSH_STAUTS_GET_SERVER_LIST);
				}
				else
				{
					printf("_mpush_client_get_acount_info failed\r\n");
					device_sleep(5, 0);
				}
				break;
			}
			case MPUSH_STAUTS_GET_SERVER_LIST:
			{
				if (_mpush_client_get_server_list(pHandler) == MPUSH_ERROR_GET_SERVER_OK)
				{
					printf("mpush_client_get_server_list success\r\n");
					mpush_set_run_status(pHandler, MPUSH_STAUTS_GET_OFFLINE_MSG);
				}
				else
				{
					printf("mpush_client_get_server_list failed\r\n");
					device_sleep(5, 0);
				}
				break;
			}
			case MPUSH_STAUTS_GET_OFFLINE_MSG:
			{
				if (_mpush_client_get_offline_msg(pHandler) == MPUSH_ERROR_GET_OFFLINE_MSG_OK)
				{
					printf("_mpush_client_get_offline_msg success\r\n");
					mpush_set_run_status(pHandler, MPUSH_STAUTS_CONNECTING);
				}
				else
				{
					printf("_mpush_client_get_offline_msg failed\r\n");
					device_sleep(5, 0);
				}
				break;
			}
			case MPUSH_STAUTS_CONNECTING:
			{
				srand(pHandler->server_list_num);
				pHandler->server_current_index = rand();
				if (pHandler->server_current_index >= MPUSH_SERVER_NUM)
				{
					pHandler->server_current_index = 0;
				}
				
				if (_mpush_connect(pHandler) == MPUSH_ERROR_CONNECT_OK)
				{
					printf("_mpush_connect success\r\n");
					mpush_set_run_status(pHandler, MPUSH_STAUTS_HANDSHAKING);
				}
				else
				{
					printf("_mpush_connect failed\r\n");
					device_sleep(5, 0);
				}
				heart_time = time(NULL);
				break;
			}
			case MPUSH_STAUTS_HANDSHAKING:
			{
				if (_mpush_handshake(pHandler) == MPUSH_ERROR_HANDSHAKE_OK)
				{
					printf("_mpush_handshake success\r\n");
					mpush_set_run_status(pHandler, MPUSH_STAUTS_HANDSHAK_WAIT_REPLY);
				}
				else
				{
					printf("_mpush_handshake failed\r\n");
					device_sleep(5, 0);
					mpush_set_run_status(pHandler, MPUSH_STAUTS_CONNECTING);
				}
				break;
			}
			case MPUSH_STAUTS_HANDSHAK_WAIT_REPLY:
			{
				break;
			}
			case MPUSH_STAUTS_HANDSHAK_OK:
			{
				mpush_set_run_status(pHandler, MPUSH_STAUTS_BINDING);
				break;
			}
			case MPUSH_STAUTS_HANDSHAK_FAIL:
			{
				break;
			}
			case MPUSH_STAUTS_BINDING:
			{
				
				if (_mpush_bind_user(pHandler) == MPUSH_ERROR_BIND_USER_OK)
				{
					printf("_mpush_bind_user success\r\n");
					mpush_set_run_status(pHandler, MPUSH_STAUTS_BINDING_WAIT_REPLY);
				}
				else
				{
					printf("_mpush_bind_user failed\r\n");
					device_sleep(5, 0);
					mpush_set_run_status(pHandler, MPUSH_STAUTS_CONNECTING);
				}
				break;
			}
			case MPUSH_STAUTS_BINDING_WAIT_REPLY:
			{
				break;
			}
			case MPUSH_STAUTS_BINDING_OK:
			{
				mpush_set_run_status(pHandler, MPUSH_STAUTS_CONNECTED);
				break;
			}
			case MPUSH_STAUTS_BINDING_FAIL:
			{
				break;
			}
			case MPUSH_STAUTS_CONNECTED:
			{
				if (abs(time(NULL) - heart_time) >= MPUSH_MAX_HEART_BEAT)
				{
					if (_mpush_heartbeat(pHandler) == MPUSH_ERROR_HEART_BEAT_OK)
					{
						printf("_mpush_heartbeat success\r\n");
					}
					else
					{
						printf("_mpush_heartbeat failed\r\n");
						device_sleep(5, 0);
						mpush_set_run_status(pHandler, MPUSH_STAUTS_CONNECTING);
					}
					heart_time = time(NULL);
				}
				break;
			}
			case MPUSH_STAUTS_DISCONNECT:
			{
				break;
			}
			default:
			{
				break;	
			}
		}

		vTaskDelay(100*5);
	}

	vTaskDelete(NULL);
}

void *mpush_client_create(sint8_t *_str_version, sint8_t *_str_device_id)
{
	MPUSH_CLIENT_HANDLER_t *pHandler = NULL;

	pHandler = (MPUSH_CLIENT_HANDLER_t *)esp32_malloc(sizeof(MPUSH_CLIENT_HANDLER_t));
	if (pHandler == NULL)
	{
		return NULL;
	}

	memset(pHandler, 0, sizeof(MPUSH_CLIENT_HANDLER_t));
	pHandler->server_sock = INVALID_SOCK;
	pHandler->lock_handle = xSemaphoreCreateMutex();
	
	snprintf((char*)pHandler->msg_cfg.str_client_version, sizeof(pHandler->msg_cfg.str_client_version), "%s", _str_version);
	snprintf((char*)pHandler->msg_cfg.str_device_id, sizeof(pHandler->msg_cfg.str_device_id), "%s", _str_device_id);
	snprintf((char*)pHandler->msg_cfg.str_os_name, sizeof(pHandler->msg_cfg.str_os_name), "ESP32");
	snprintf((char*)pHandler->msg_cfg.str_os_version, sizeof(pHandler->msg_cfg.str_os_version), "%s", _str_version);
	/*
	mbedtls_pk_context cxt;
	memset(&cxt, 0, sizeof(cxt));
	mbedtls_pk_parse_public_key(&cxt, (unsigned char*)str_public_key, strlen(str_public_key)+1);
	pHandler->rsa = mbedtls_pk_rsa(cxt);
	int ret = mbedtls_rsa_check_pubkey(pHandler->rsa);
	printf("mbedtls_rsa_check_pubkey %d\r\n", ret);
	
	mbedtls_ctr_drbg_init(&pHandler->ctr_drbg);
	ret = mbedtls_rsa_pkcs1_encrypt(pHandler->rsa, mbedtls_ctr_drbg_random,
                                            &ctr_drbg, MBEDTLS_RSA_PUBLIC,
                                            strlen("123"), "123", buf );
	printf("mbedtls_rsa_check_pubkey %d\r\n", ret);
	*/
	
	return (void*)pHandler;
}

int mpush_client_stop(void *handler)
{
	MPUSH_CLIENT_HANDLER_t * pHandler = (MPUSH_CLIENT_HANDLER_t *)handler;
	if (pHandler->server_sock != INVALID_SOCK)
	{
		sock_close(pHandler->server_sock);
		pHandler->server_sock = INVALID_SOCK;
	}

	return 0;
}

int mpush_client_destroy(void *handler)
{
	return -1;
}

int mpush_client_send_msg(
	MPUSH_CLIENT_HANDLER_t *_pHandler, 
	const int _msg_type, 
	const char *_data, 
	const size_t data_len,
	const char *_nlp_text,
	const char *_file_type,
	const char *_to_user)
{
	char 	str_type[8] = {0};
	char	domain[64]	= {0};
	char	port[6] 	= {0};
	char	params[128] = {0};
	char	str_nonce[64] = {0};
	char	str_timestamp[64] = {0};
	char	str_private_key[64] = {0};
	char	str_device_id[32] = {0};
	char	str_buf[512]= {0};
	sock_t	sock		= INVALID_SOCK;
	cJSON	*pJson = NULL;
	
	if (_pHandler == NULL)
	{
		return;
	}
	
	if (sock_get_server_info(MPUSH_SEND_MSG_URL, &domain, &port, &params) != 0)
	{
		ESP_LOGE(PRINT_TAG, "mpush_client_send_msg,sock_get_server_info fail\r\n");
		goto mpush_client_send_msg_error;
	}

	sock = sock_connect(domain, port);
	if (sock == INVALID_SOCK)
	{
		ESP_LOGE(PRINT_TAG, "mpush_client_send_msg,sock_connect fail,%s,%s\r\n", domain, port);
		goto mpush_client_send_msg_error;
	}

	sock_set_nonblocking(sock);

	if (_msg_type == MPUSH_SEND_MSG_TYPE_FILE)
	{
		snprintf(str_type, sizeof(str_type), "file");
	}
	else if (_msg_type == MPUSH_SEND_MSG_TYPE_TEXT)
	{
		snprintf(str_type, sizeof(str_type), "text");
	}
	else
	{
		ESP_LOGE(PRINT_TAG, "mpush_client_send_msg,wrong msg type(%d)", _msg_type);
		goto mpush_client_send_msg_error;
	}

	size_t len = 0;
	memset(_pHandler->str_comm_buf1, 0, sizeof(_pHandler->str_comm_buf1));
	if (_msg_type == MPUSH_SEND_MSG_TYPE_FILE)
	{
		crypto_base64_encode((uint8_t*)_pHandler->str_comm_buf1, sizeof(_pHandler->str_comm_buf1), &len, (uint8_t*)_data, data_len);
	}
	else
	{
		snprintf(_pHandler->str_comm_buf1, sizeof(_pHandler->str_comm_buf1), "%s", _data);
	}
	
	crypto_generate_request_id(str_nonce, sizeof(str_nonce));
	get_flash_cfg(FLASH_CFG_DEVICE_ID, str_device_id);
	snprintf(_pHandler->str_comm_buf, sizeof(_pHandler->str_comm_buf),
		"{\"content\":"
			"{\"data\": \"%s\","
				"\"encrypt\": %s,"
		   		"\"fileType\":\"%s\","
		   		"\"inputTxt\":\"%s\","
		   		"\"msgType\":\"%s\","
		   		"\"toUserId\":\"%s\","
		   		"\"userId\": \"%s\","
		   		"\"userName\":\"%s\"},"
		  "\"appId\":\"%s\","
		  "\"osType\":\"esp32\","
		  "\"protocolVersion\": \"2.0.0.0\","
		  "\"requestId\": \"%s\","
		  "\"robotId\": \"%s\","
		  "\"userId\": \"%s\"}",
		_pHandler->str_comm_buf1, 
		"false", 
		_file_type,
		_nlp_text, 
		str_type,
		_to_user,
		str_device_id,
		str_device_id,
		DEEP_BRAIN_APP_ID,
		str_nonce, 
		DEEP_BRAIN_ROBOT_ID,
		str_device_id);
	
	crypto_generate_nonce((uint8_t *)str_nonce, sizeof(str_nonce));
	crypto_time_stamp((unsigned char*)str_timestamp, sizeof(str_timestamp));
	crypto_generate_private_key((uint8_t *)str_private_key, sizeof(str_private_key), str_nonce, str_timestamp, DEEP_BRAIN_ROBOT_ID);
	snprintf(str_buf, sizeof(str_buf), 
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
		params, domain, port, strlen(_pHandler->str_comm_buf), str_nonce, str_timestamp, str_private_key, DEEP_BRAIN_ROBOT_ID);

	//ESP_LOGE(PRINT_TAG, "%s", str_buf);
	//ESP_LOGE(PRINT_TAG, "%s", _pHandler->str_comm_buf);
	if (sock_writen_with_timeout(sock, str_buf, strlen(str_buf), 1000) != strlen(str_buf)) 
	{
		printf("sock_writen http header fail\r\n");
		goto mpush_client_send_msg_error;
	}
	
	if (sock_writen_with_timeout(sock, _pHandler->str_comm_buf, strlen(_pHandler->str_comm_buf), 3000) != strlen(_pHandler->str_comm_buf)) 
	{
		printf("sock_writen http body fail\r\n");
		goto mpush_client_send_msg_error;
	}

	/* Read HTTP response */
	memset(_pHandler->str_comm_buf, 0, sizeof(_pHandler->str_comm_buf));
	sock_readn_with_timeout(sock, _pHandler->str_comm_buf, sizeof(_pHandler->str_comm_buf) - 1, 8000);
	sock_close(sock);
	sock = INVALID_SOCK;
	
	if (http_get_error_code(_pHandler->str_comm_buf) == 200)
	{	
		char* pBody = http_get_body(_pHandler->str_comm_buf);
		if (NULL != pBody)
		{
			pJson = cJSON_Parse(pBody);
			if (NULL != pJson) 
			{
				cJSON *pJson_status = cJSON_GetObjectItem(pJson, "statusCode");
				if (NULL == pJson_status || pJson_status->valuestring == NULL)
				{
					ESP_LOGE(PRINT_TAG, "mpush_client_send_msg,statusCode not found");
					goto mpush_client_send_msg_error;
				}

				if (strncasecmp(pJson_status->valuestring, "OK", 2) != 0)
				{
					ESP_LOGE(PRINT_TAG, "mpush_client_send_msg,statusCode:%s", pJson_status->valuestring);
					goto mpush_client_send_msg_error;
				}
				ESP_LOGI(PRINT_TAG, "mpush_client_send_msg,statusCode:%s", pJson_status->valuestring);
			}
			else
			{
				ESP_LOGE(PRINT_TAG, "mpush_client_send_msg,invalid json[%s]", _pHandler->str_comm_buf);
			}
			
			if (NULL != pJson)
			{
				cJSON_Delete(pJson);
				pJson = NULL;
			}
		}
	}
	else
	{
		ESP_LOGE(PRINT_TAG, "mpush_client_send_msg,http reply error[%s]", _pHandler->str_comm_buf);
		goto mpush_client_send_msg_error;
	}
	
	return MPUSH_ERROR_SEND_MSG_OK;
	
mpush_client_send_msg_error:
	
	if (sock != INVALID_SOCK)
	{
		sock_close(sock);
	}

	if (NULL != pJson)
	{
		cJSON_Delete(pJson);
		pJson = NULL;
	}
	
	return MPUSH_ERROR_SEND_MSG_FAIL;
}


int mpush_client_send_text(
	MPUSH_CLIENT_HANDLER_t * _pHandler,
	const char * _text, 
	const char * _nlp_text,
	const char *_to_user)
{
	return mpush_client_send_msg(_pHandler, MPUSH_SEND_MSG_TYPE_TEXT, _text, strlen(_text), _nlp_text, "", _to_user);
}

int mpush_client_send_file(
	MPUSH_CLIENT_HANDLER_t * _pHandler,
	const char * _data, 
	const size_t data_len, 
	const char * _nlp_text,
	const char *_file_type,
	const char *_to_user)
{
	return mpush_client_send_msg(_pHandler, MPUSH_SEND_MSG_TYPE_FILE, _data, data_len, _nlp_text, _file_type, _to_user);
}




