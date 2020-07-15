/**
 * @file    sinovoice_api.h
 * @brief  sinovoice api
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

#ifndef __SINOVOICE_API_H__
#define __SINOVOICE_API_H__

#include "freertos/semphr.h"
#include "userconfig.h"
#include "flash_config_manage.h"

#ifdef __cplusplus
extern "C" {
#endif /* C++ */

typedef struct
{
	int  iValid;		//是否有效,0-无效,1-有效
	int  iSegmentIndex;	//索引
	int  iScore;		//得分
	int  iStartTime;	//起始时间
	int  iEndTime;		//结束时间
	char achText[512];	//识别结果
}SINOVOICE_ASR_RESULT_T;

//语音识别错误码
typedef enum
{
	//识别成功
	SINOVOICE_ASR_OK = 0,
	//识别失败
	SINOVOICE_ASR_FAIL,
	//无效参数
	SINOVOICE_ASR_INVALID_PARAMS,
	//http内容无效
	SINOVOICE_ASR_INVALID_HTTP_CONTENT,
	//json内容无效
	SINOVOICE_ASR_INVALID_JSON_CONTENT,
	//创建socket失败
	SINOVOICE_ASR_SOCKET_FAIL,
	//创建socket成功
	SINOVOICE_ASR_SOCKET_SUCCESS,
	//TTS成功
	SINOVOICE_TTS_SUCCESS,
	//TTS失败
	SINOVOICE_TTS_FAIL,
	//超出内存范围
	SINOVOICE_OUT_OF_MEM,
	//没有达到16k发送buffer
	SINOVOICE_NOT_REACH_16K
}SINOVOICE_ASR_ERRNO_T;

void *sinovoice_asr_create(SINOVOICE_ACOUNT_T *acount);
SINOVOICE_ASR_ERRNO_T sinovoice_asr_write(void *_handler, const char *data, const uint32_t data_len);
SINOVOICE_ASR_ERRNO_T sinovoice_asr_result(void *asr_handle, char* str_reply, const size_t str_reply_len);
void sinovoice_asr_delete(void *_handler);

#ifdef __cplusplus
} /* extern "C" */	
#endif /* C++ */

#endif /* __SINOVOICE_API_H__ */

