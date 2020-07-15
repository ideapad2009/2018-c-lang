#ifndef DEEPBRAIN_NLP_DECODE_H
#define DEEPBRAIN_NLP_DECODE_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "dp_comm_library_interface.h"

#ifdef __cplusplus
extern "C" {
#endif /* C++ */

/*语义识别结果解析*/
typedef enum NLP_DECODE_ERRNO_t
{
	NLP_DECODE_ERRNO_OK,
	NLP_DECODE_ERRNO_FAIL,
	NLP_DECODE_ERRNO_INVALID_JSON,
}NLP_DECODE_ERRNO_t; 

NLP_DECODE_ERRNO_t dp_nlp_result_decode(const char* json_string, NLP_RESULT_T *nlp_result);

#ifdef __cplusplus
} /* extern "C" */	
#endif /* C++ */

#endif

