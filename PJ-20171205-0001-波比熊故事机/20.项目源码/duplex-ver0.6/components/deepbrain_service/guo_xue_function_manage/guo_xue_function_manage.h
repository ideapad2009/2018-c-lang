#ifndef __GUO_XUE_FUNCTION_MANAGE_H__
#define __GUO_XUE_FUNCTION_MANAGE_H__

#include "nlp_service.h"

typedef struct guo_xue_handle
{
	int cur_play_index;
	NLP_RESULT_T nlp_result;
}GUO_XUE_HANDLE;

void guo_xue_play_start();
void guo_xue_init();

#endif