
#ifndef __GELING_EDU_H__
#define __GELING_EDU_H__
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif /* C++ */

#define TEST_M3U8_LINK "http://yunstudy.koo6.cn/Yinpinfile/ydevkxhqquexpnwj/cn/4659.m3u8"
#define GELING_EDU_LINK_MAX_NUM 100

typedef enum
{
	NLP_SERVICE_MSG_TYPE_REQUEST,
	NLP_SERVICE_MSG_TYPE_QUIT,	
}GL_NLP_SERVICE_MSG_TYPE_T;

//geling result link
typedef struct
{
	char link_url[256];
}GELING_EDU_LINK_T;

//geling msg
typedef struct
{
	GL_NLP_SERVICE_MSG_TYPE_T msg_type;
	char edu_url[256];
}GELING_EDU_MSG_T;

//geling result links
typedef struct
{
	int cur_play_index;
	int	link_size;
	GELING_EDU_LINK_T link[GELING_EDU_LINK_MAX_NUM];
}GELING_EDU_LINKS_T;

typedef struct
{
	char 	domain[64];
	char 	port[6];
	char 	params[128];
	char 	header[512];
}GELING_EDU_HTTP_BUFF_T;

typedef struct
{	
	SemaphoreHandle_t data_lock;
	char http_reply_buffer[20*1024];//20k
	xQueueHandle msg_queue;
	GELING_EDU_LINKS_T edu_links;//edu_result
	GELING_EDU_HTTP_BUFF_T http_buffer;
}GELING_EDU_SERVICE_STATUS_T;

//interface
void geling_edu_service_create(void);
void geling_edu_service_delete(void);
void geling_edu_service_send_request(const char *edu_link);

#ifdef __cplusplus
} /* extern "C" */	
#endif /* C++ */

#endif

