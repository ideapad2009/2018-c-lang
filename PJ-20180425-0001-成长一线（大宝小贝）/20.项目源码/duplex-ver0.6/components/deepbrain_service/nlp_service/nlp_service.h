#ifndef __NLP_SERVICE_H__
#define __NLP_SERVICE_H__
#include <stdio.h>
#include "dp_comm_library_interface.h"

#ifdef __cplusplus
extern "C" {
#endif /* C++ */

//interface
void nlp_service_create(void);
void nlp_service_delete(void);
void nlp_service_send_request(const char *text, nlp_result_cb cb);
void nlp_service_send_translate_request(const char *text, nlp_result_cb cb);

#ifdef __cplusplus
} /* extern "C" */	
#endif /* C++ */

#endif

