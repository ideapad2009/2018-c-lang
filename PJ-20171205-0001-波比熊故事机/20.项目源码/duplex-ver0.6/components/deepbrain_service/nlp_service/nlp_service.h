
#ifndef __NLP_SERVICE_H__
#define __NLP_SERVICE_H__
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif /* C++ */

#define NLP_SERVICE_LINK_MAX_NUM 10

//nlp result type
typedef enum
{
	NLP_RESULT_TYPE_NONE,
	NLP_RESULT_TYPE_CHAT,
	NLP_RESULT_TYPE_LINK,
	NLP_RESULT_TYPE_CMD,
}NLP_RESULT_TYPE_T;

//nlp result chat
typedef struct
{
	char text[1024];
}NLP_RESULT_CHAT_T;

//nlp result link
typedef struct
{
	char link_name[96];
	char link_url[128];
}NLP_RESULT_LINK_T;

//nlp result links
typedef struct
{
	int	link_size;
	NLP_RESULT_LINK_T link[NLP_SERVICE_LINK_MAX_NUM];
}NLP_RESULT_LINKS_T;

//nlp result
typedef struct 
{
	char input_text[64];
	NLP_RESULT_TYPE_T type;
	NLP_RESULT_CHAT_T chat_result;
	NLP_RESULT_LINKS_T link_result;
}NLP_RESULT_T;

//nlp result callback
typedef void (*nlp_service_result_cb)(NLP_RESULT_T *result);

//interface
void nlp_service_create(void);
void nlp_service_delete(void);
void nlp_service_send_request(const char *text, nlp_service_result_cb cb);

#ifdef __cplusplus
} /* extern "C" */	
#endif /* C++ */

#endif

