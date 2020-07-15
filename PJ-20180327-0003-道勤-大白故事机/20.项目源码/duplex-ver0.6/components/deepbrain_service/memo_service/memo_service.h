#ifndef MEMO_SERVICE_H
#define MEMO_SERVICE_H

#include "ctypes.h"
#include "MediaService.h"

#define MEMO_EVEVT_MAX 10

typedef enum  
{
	MEMO_IDLE,
	MEMO_INIT,
	MEMO_ADD,
	MEMO_LOOP,
}MEMO_STATUS_T; 

typedef struct 
{	
	uint time;
	char str[64];
}MEMO_EVENT_T;

typedef struct 
{	
	uint current;
	MEMO_EVENT_T event[MEMO_EVEVT_MAX];
}MEMO_ARRAY_T;

typedef struct 
{
	char domain[64];
	char port[6];
	char params[128];
	char str_nonce[64];
	char str_timestamp[64];
	char str_private_key[64];
	char str_device_id[32];
	char str_buf[512];
	char str_comm_buf[BUFFER_SIZE_1024*2]; //校时：http请求参数
}TEMP_PARAM_T;

typedef struct 
{	
	uint initial_time_sec;    //初始时间（秒）
	uint initial_time_usec;   //毫秒
	MEMO_EVENT_T  memo_event; 
	MEMO_ARRAY_T  memo_array;
	MEMO_STATUS_T memo_status;
	MediaService  *service;
}MEMO_SERVICE_HANDLE_T;

extern void memo_service_create(MediaService *self);
extern int get_memo_result(char *str_body);
extern void show_memo(void);

#endif
