/**
 * @file    baidu_api.h
 * @brief  baidu api
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

#ifndef __BAIDU_API_H__
#define __BAIDU_API_H__

#include "flash_config_manage.h"

#ifdef __cplusplus
extern "C" {
#endif /* C++ */

//TTS合成错误码
typedef enum
{
	//合成成功
	BAIDU_TTS_SUCCESS = 0,
	//获取认证失败
	BAIDU_TTS_GET_TOKEN_FAIL,
}BAIDU_TTS_RESULT;

//百度语音识别模型
/*
dev_pid 参数列表
| dev_pid | 语言 | 模型 | 是否有标点 | 备注 |
| ——- | —— | ——- | ——- | —— |
| 1536 | 普通话(支持简单的英文识别) | 搜索模型 | 无标点 | 支持自定义词库 |
| 1537 | 普通话(纯中文识别) | 输入法模型 | 有标点 | 不支持自定义词库 |
| 1737 | 英语 | | 有标点 | 不支持自定义词库 |
| 1637 | 粤语 | | 有标点 | 不支持自定义词库 |
| 1837 | 四川话 | | 有标点 |不支持自定义词库 |
| 1936| 普通话远场 | 远场模型 | 有标点 | 不支持 |
*/
typedef enum
{
	BAIDU_ASR_MODE_1536 = 1536,//普通话(支持简单的英文识别) 
	BAIDU_ASR_MODE_1537 = 1537,//普通话(纯中文识别)
	BAIDU_ASR_MODE_1737 = 1737,//英语
	BAIDU_ASR_MODE_1637 = 1637,//粤语
	BAIDU_ASR_MODE_1837 = 1837,//四川话
	BAIDU_ASR_MODE_1936 = 1936,//普通话远场
}BAIDU_ASR_MODE_T;

//百度语音识别音频类型
typedef enum
{
	BAIDU_ASR_AUDIO_TYPE_AMRNB,
	BAIDU_ASR_AUDIO_TYPE_AMRWB,
}BAIDU_ASR_AUDIO_TYPE_T;

//语音识别错误码
typedef enum
{
	//识别成功
	BAIDU_ASR_SUCCESS = 0,
	//识别失败
	BAIDU_ASR_FAIL,
	//获取认证失败
	BAIDU_ASR_GET_TOKEN_FAIL,
	//上传音频文件失败
	BAIDU_ASR_POST_AUDIO_FAIL,
	//识别结果错误
	BAIDU_ASR_RESULT_ERROR,
	//输入参数不正确
	BAIDU_ASR_INVALID_PARAMS = 3300,
	//识别错误
	BAIDU_ASR_REC_FAIL = 3301,
	//验证失败
	BAIDU_ASR_TOKEN_FAIL = 3302,
	//语音服务器后端问题
	BAIDU_ASR_SERVER_ERROR = 3303,
	//请求 GPS 过大，超过限额
	BAIDU_ASR_LIMIT_GPS = 3304,
	//产品线当前日请求数超过限额
	BAIDU_ASR_REQUEST_LIMIT = 3305,
}BAIDU_ASR_RESULT;

int baidu_asr_amrnb(BAIDU_ACOUNT_T *_acount, char* str_reply, size_t str_reply_len, unsigned char *_data, size_t _data_len, BAIDU_ASR_MODE_T mode);
int baidu_asr_amrwb(BAIDU_ACOUNT_T *_acount, char* str_reply, size_t str_reply_len, unsigned char *_data, size_t _data_len, BAIDU_ASR_MODE_T mode);
int baidu_tts(BAIDU_ACOUNT_T *_acount, char* str_url, size_t str_url_len, char* str_text);

#ifdef __cplusplus
} /* extern "C" */	
#endif /* C++ */

#endif /* __BAIDU_API_H__ */

