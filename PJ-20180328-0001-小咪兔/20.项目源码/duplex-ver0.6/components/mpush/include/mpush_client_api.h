#ifndef __MPUSH_CLIENT_API_H__
#define __MPUSH_CLIENT_API_H__

#include <stdio.h>
#include <string.h>
#include "debug_log.h"
#include "socket.h"
#include "ctypes.h"
#include "device_api.h"
#include "memory_manage.h"
#include "mpush_message.h"
#include "mpush_client_api.h"
#include "mbedtls/pk.h"
#include "mbedtls/rsa.h"
#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include "lwip/netdb.h"
#include "lwip/dns.h"
#include "http_api.h"
#include "cJSON.h"
#include "flash_config_manage.h"
#include "userconfig.h"
#include "crypto_api.h"

#ifdef __cplusplus
extern "C" {
#endif /* C++ */

#define MPUSH_DEBUG 0

#if MPUSH_DEBUG == 1
#define MPUSH_GET_SERVER_URL 		"http://192.168.20.91:9034/open-api/testFetchMessageServers"
#define MPUSH_SEND_MSG_URL 	 		"http://192.168.20.91:9034/open-api/testSendMessage"
#define MPUSH_GET_OFFLINE_MSG_URL 	"http://192.168.20.91:9034/open-api/testFetchOfflineMessage"
#define MPUSH_GET_OPEN_SERVICE_URL 	"http://api.deepbrain.ai:8383/open-api/service"		
#elif MPUSH_DEBUG == 2
#define MPUSH_GET_SERVER_URL 		"http://192.168.16.4:9034/open-api/testFetchMessageServers"
#define MPUSH_SEND_MSG_URL 	 		"http://192.168.16.4:9034/open-api/testSendMessage"
#define MPUSH_GET_OFFLINE_MSG_URL 	"http://192.168.16.4:9034/open-api/testFetchOfflineMessage"
#define MPUSH_GET_OPEN_SERVICE_URL 	"http://192.168.16.4:9134/open-api/service"	
#else
#define MPUSH_GET_SERVER_URL 		"http://message.deepbrain.ai:9134/open-api/testFetchMessageServers"
#define MPUSH_SEND_MSG_URL 	 		"http://message.deepbrain.ai:9134/open-api/testSendMessage"
#define MPUSH_GET_OFFLINE_MSG_URL 	"http://message.deepbrain.ai:9134/open-api/testFetchOfflineMessage"	
#define MPUSH_GET_OPEN_SERVICE_URL 	"http://api.deepbrain.ai:8383/open-api/service"	
#endif
#define MPUSH_SERVER_NUM 10

typedef enum
{
	MPUSH_ERROR_GET_SERVER_OK = 0,
	MPUSH_ERROR_GET_SERVER_FAIL,
	MPUSH_ERROR_GET_SN_OK,
	MPUSH_ERROR_GET_SN_FAIL,
	MPUSH_ERROR_GET_ACOUNT_OK,
	MPUSH_ERROR_GET_ACOUNT_FAIL,
	MPUSH_ERROR_GET_OFFLINE_MSG_OK,
	MPUSH_ERROR_GET_OFFLINE_MSG_FAIL,
	MPUSH_ERROR_SEND_MSG_OK,
	MPUSH_ERROR_SEND_MSG_FAIL,
	MPUSH_ERROR_START_OK,
	MPUSH_ERROR_START_FAIL,
	MPUSH_ERROR_INVALID_PARAMS,
	MPUSH_ERROR_CONNECT_OK,
	MPUSH_ERROR_CONNECT_FAIL,
	MPUSH_ERROR_HANDSHAKE_OK,
	MPUSH_ERROR_HANDSHAKE_FAIL,
	MPUSH_ERROR_BIND_USER_OK,
	MPUSH_ERROR_BIND_USER_FAIL,
	MPUSH_ERROR_HEART_BEAT_OK,
	MPUSH_ERROR_HEART_BEAT_FAIL,
}MPUSH_ERROR_CODE_T;

typedef enum
{
	MPUSH_STAUTS_INIT = 0,
	MPUSH_STAUTS_GET_ACOUNT_INFO,
	MPUSH_STAUTS_GET_SERVER_LIST,
	MPUSH_STAUTS_GET_OFFLINE_MSG,
	MPUSH_STAUTS_CONNECTING,
	MPUSH_STAUTS_HANDSHAKING,
	MPUSH_STAUTS_HANDSHAK_WAIT_REPLY,
	MPUSH_STAUTS_HANDSHAK_OK,
	MPUSH_STAUTS_HANDSHAK_FAIL,
	MPUSH_STAUTS_BINDING,
	MPUSH_STAUTS_BINDING_WAIT_REPLY,
	MPUSH_STAUTS_BINDING_OK,
	MPUSH_STAUTS_BINDING_FAIL,
	MPUSH_STAUTS_CONNECTED,
	MPUSH_STAUTS_DISCONNECT,
}MPUSH_STATUS_T;

typedef struct
{
	int  isValid;										//0-invalid, 1-valid
	char str_server_addr[BUFFER_SIZE_64];
	char str_server_port[BUFFER_SIZE_8];
}MPUSH_SERVER_INFO_T;

typedef struct
{
	SemaphoreHandle_t lock_handle;
	MPUSH_STATUS_T status;
	sock_t	server_sock;								// server socket
	char str_server_list_url[BUFFER_SIZE_128];			// server address
	int server_list_num;								// server list num
	int server_current_index;							// current server index
	MPUSH_SERVER_INFO_T server_list[MPUSH_SERVER_NUM];	//server list array
	char str_public_key[BUFFER_SIZE_1024];				// public key
	char str_comm_buf[BUFFER_SIZE_1024*30];				// commucation buffer
	char str_comm_buf1[BUFFER_SIZE_1024*30];			// 
	char str_recv_buf[BUFFER_SIZE_1024*10];				// recv buffer
	uint8_t str_push_msg[BUFFER_SIZE_1024*10];			// recv push msg
	MPUSH_MSG_CONFIG_T msg_cfg;
	mbedtls_rsa_context *rsa;
	PLATFORM_AI_ACOUNTS_T ai_acounts;
	//mbedtls_ctr_drbg_context ctr_drbg;
}MPUSH_CLIENT_HANDLER_t;

void *mpush_client_create(sint8_t *_str_version, sint8_t *_str_device_id);
void mpush_client_process(void *_handler);
void mpush_client_receive(void *_handler);
int mpush_client_destroy(void *handler);
int mpush_client_send_text(
	MPUSH_CLIENT_HANDLER_t * _pHandler,
	const char * _text, 
	const char * _nlp_text,
	const char *_to_user);

int mpush_client_send_file(
	MPUSH_CLIENT_HANDLER_t * _pHandler,
	const char * _data, 
	const size_t data_len, 
	const char * _nlp_text,
	const char *_file_type,
	const char *_to_user);

#ifdef __cplusplus
} /* extern "C" */	
#endif /* C++ */

#endif /* __MPUSH_CLIENT_API_H__ */

