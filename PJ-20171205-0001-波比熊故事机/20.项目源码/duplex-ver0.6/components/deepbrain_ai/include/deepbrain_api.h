/**
 * @file    deepbrain_api.h
 * @brief  deepbrain nlp api
 * 
 *  This file contains the quick application programming interface (API) declarations 
 *  of deep brain nlp interface. Developer can include this file in your project to build applications.
 *  For more information, please read the developer guide.
 * 
 * @author  Jacky Ding
 * @version 1.0.0
 * @date    2017/6/15
 * 
 * @see        
 * 
 * History:
 * index    version        date            author        notes
 * 0        1.0            2017/6/15     Jacky Ding   Create this file
 */

#ifndef __DEEP_BRAIN_API_H__
#define __DEEP_BRAIN_API_H__

#include "ctypes.h"

#ifdef __cplusplus
extern "C" {
#endif /* C++ */

//NLP error code
typedef enum
{
	DB_ERROR_CODE_SUCCESS = 0,
	DB_ERROR_CODE_HTTP_FAIL,
	DB_ERROR_CODE_INVALID_PARAMS,
}EM_DB_ERROR_CODE;

//NLP params index
typedef enum
{
	DB_NLP_PARAMS_APP_ID = 0,		//app id
	DB_NLP_PARAMS_ROBOT_ID,			//robot id
	DB_NLP_PARAMS_DEVICE_ID,		//device id
	DB_NLP_PARAMS_USER_ID,			//user id
	DB_NLP_PARAMS_CITY_NAME,		//城市
	DB_NLP_PARAMS_CITY_LONGITUDE,	//经度
	DB_NLP_PARAMS_CITY_LATITUDE,	//纬度
}EM_DB_NLP_PARAMS_INDEX;

void *deep_brain_nlp_create(void);
int deep_brain_nlp_process(void *_handler, char *_str_text);
char *deep_brain_nlp_result(void *_handler);
void deep_brain_nlp_destroy(void *_handler);
int deep_brain_set_params(void *_handler, int _index, void *_data);

#ifdef __cplusplus
} /* extern "C" */	
#endif /* C++ */

#endif /* __BAIDU_ASR_API_H__ */

