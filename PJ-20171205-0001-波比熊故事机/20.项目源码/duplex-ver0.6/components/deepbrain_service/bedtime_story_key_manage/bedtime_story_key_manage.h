#ifndef __BEDTIME_STORY_KEY_MANAGE_H__
#define __BEDTIME_STORY_KEY_MANAGE_H__
	
#include "nlp_service.h"
	
typedef struct bedtime_story_handle
{
	int cur_play_index;
	NLP_RESULT_T nlp_result;
}BEDTIME_STORY_HANDLE;

void bedtime_story_play_start();
void bedtime_story_function_init();
	
#endif

