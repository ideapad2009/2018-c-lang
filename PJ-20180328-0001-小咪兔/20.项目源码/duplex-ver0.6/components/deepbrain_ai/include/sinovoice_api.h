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

typedef void (*cb_asr_result)(char *_asr_result);

typedef struct
{
	int		audio_index; 	//音频段索引，从1开始
	int	    last_audio;	 	//1，最后一段音频
	char	*audio_data; 	//音频数据
	int		audio_len;	 	//音频数据大小
}SINOVOICE_ASR_RECORD, *PSINOVOICE_ASR_RECORD;

typedef struct
{
	int  iValid;		//是否有效,0-无效,1-有效
	int  iSegmentIndex;	//索引
	int  iScore;		//得分
	int  iStartTime;	//起始时间
	int  iEndTime;		//结束时间
	char achText[512];	//识别结果
}SINOVOICE_ASR_RESULT, *PSINOVOICE_ASR_RESULT;

//语音识别错误码
typedef enum
{
	//识别成功
	SINOVOICE_ASR_SUCCESS = 0,
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
}SINOVOICE_ASR_RESULT_ERRNO;

typedef struct
{
	int		sock;		 	//socket
	char	identify[32];	//唯一标识符
	char	ach_domain[64]; //服务器域名
	char	ach_params[64]; //服务器参数
	char	ach_port[8];	//服务器端口
	char	str_reply[1024*10];	//http reply
	char 	strTimeStamp[32];
	char 	header[512];
	char 	strSessionKey[64];
	SemaphoreHandle_t lock_handle;
	cb_asr_result cb_asr_result;
	SINOVOICE_ACOUNT_T acount;
}SINOVOICE_ASR_HANDLER, *PSINOVOICE_ASR_HANDLER;

void *sinovoice_asr_create(cb_asr_result _cb);
int sinovoice_asr_write(void *_handler, PSINOVOICE_ASR_RECORD _record, PSINOVOICE_ASR_RESULT _asr_result);
void sinovoice_asr_destroy(void *_handler);
void sinovoice_tts_generate(char *_text);

#ifdef __cplusplus
} /* extern "C" */	
#endif /* C++ */

#endif /* __SINOVOICE_API_H__ */

