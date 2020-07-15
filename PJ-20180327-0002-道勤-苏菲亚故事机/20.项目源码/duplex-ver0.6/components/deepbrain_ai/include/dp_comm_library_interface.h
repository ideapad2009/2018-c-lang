#ifndef DP_COMM_LIBRARY_INTERFACE_H
#define DP_COMM_LIBRARY_INTERFACE_H

#include <stdio.h>
#include "ctypes.h"

#ifdef __cplusplus
extern "C" {
#endif /* C++ */

//计算 存储 milliseconds毫秒音频(samples_per_sec 采样率，bytes_per_sample字节 采样精度)数据， 所需要的字节数
#define RAW_PCM_LEN_MS(milliseconds, samples_per_sec, bytes_per_sample)  (2L*(milliseconds)*(samples_per_sec)*(bytes_per_sample) / 1000) 

//计算 存储 seconds秒音频(samples_per_sec 采样率，bytes_per_sample字节 采样精度)数据， 所需要的字节数
#define RAW_PCM_LEN_S(seconds, samples_per_sec, bytes_per_sample)  (2L*(seconds)*(samples_per_sec)*(bytes_per_sample))

/******************************************************************
//语义处理接口
*******************************************************************/
#define NLP_SERVICE_LINK_MAX_NUM 10
	
/*语义结果类型*/
typedef enum NLP_RESULT_TYPE_T
{
	NLP_RESULT_TYPE_NONE,
	NLP_RESULT_TYPE_NO_ASR,
	NLP_RESULT_TYPE_SHORT_AUDIO,
	NLP_RESULT_TYPE_CHAT,
	NLP_RESULT_TYPE_LINK,
	NLP_RESULT_TYPE_CMD,
	NLP_RESULT_TYPE_TRANSLATE,
	NLP_RESULT_TYPE_ERROR,
}NLP_RESULT_TYPE_T;

/*语义结果-聊天结果*/
typedef struct NLP_RESULT_CHAT_T
{
	char text[1024];
	char link[1024];
}NLP_RESULT_CHAT_T;

/*语义结果-音乐资源*/
typedef struct NLP_RESULT_LINK_T
{
	char link_name[96];
	char link_url[128];
}NLP_RESULT_LINK_T;

/*语义结果-音乐资源集合*/
typedef struct NLP_RESULT_LINKS_T
{
	int link_size;
	NLP_RESULT_LINK_T link[NLP_SERVICE_LINK_MAX_NUM];
}NLP_RESULT_LINKS_T;

/*语义结果集合*/
typedef struct NLP_RESULT_T
{
	int request_sn;		//请求序列号
	char input_text[256];
	NLP_RESULT_TYPE_T type;
	NLP_RESULT_CHAT_T chat_result;
	NLP_RESULT_LINKS_T link_result;
}NLP_RESULT_T;

/*语义结果回调*/
typedef void (*nlp_result_cb)(NLP_RESULT_T *result);


/******************************************************************
//语音识别接口
*******************************************************************/
/*语音识别结果类型*/
typedef enum ASR_RESULT_TYPE_T
{
	ASR_RESULT_TYPE_SHORT_AUDIO,	//语音输入太短
	ASR_RESULT_TYPE_FAIL,			//语音识别失败
	ASR_RESULT_TYPE_SUCCESS,		//语音识别成功
	ASR_RESULT_TYPE_NO_RESULT,		//没有识别结果
}ASR_RESULT_TYPE_T;

/*语音识别结果*/
typedef struct ASR_RESULT_T
{
	int 				record_sn;									//语音识别序列号
	ASR_RESULT_TYPE_T 	type;										//语音识别结果类型
	char 				asr_text[256];								//语音识别结果
	char 				record_data[RAW_PCM_LEN_S(10, 16000, 2)/4];	//录音数据缓存
	uint32_t 			record_ms;									//录音时间
	uint32_t 			record_len;									//录音长度
}ASR_RESULT_T;

/*语音输入类型*/
typedef enum ASR_MSG_TYPE_T
{
	ASR_SERVICE_RECORD_START,
	ASR_SERVICE_RECORD_READ,
	ASR_SERVICE_RECORD_STOP,
	ASR_SERVICE_QUIT,
}ASR_MSG_TYPE_T;

/*语音识别引擎选择*/
typedef enum ASR_ENGINE_TYPE_t
{
	//amr
	ASR_ENGINE_TYPE_AMRNB = 0,
	ASR_ENGINE_TYPE_AMRWB,
	//deepbrain private asr engine
	ASR_ENGINE_TYPE_DP_ENGINE,
}ASR_ENGINE_TYPE_t;

/*语音识别语言类型*/
typedef enum ASR_LANGUAGE_TYPE_T
{
	ASR_LANGUAGE_TYPE_CHINESE,//中文
	ASR_LANGUAGE_TYPE_ENGLISH,//英文
}ASR_LANGUAGE_TYPE_T;

/*语音识别输入PCM数据*/
typedef struct ASR_RECORD_DATA_T
{
	int 				record_sn;		//录音序列号
	int 				record_id;		//录音索引
	int 				record_len;		//录音数据长度
	int 				record_ms;		//录音时长
	int 				is_max_ms;		//录音是否是最大的录音时间
	uint64_t 			time_stamp;		//录音时间戳
	ASR_LANGUAGE_TYPE_T language_type;	//识别语言类型
	ASR_ENGINE_TYPE_t 	asr_engine;		//语音识别引擎类型
	char record_data[RAW_PCM_LEN_MS(200, 16000, 2)];//200毫秒pcm数据
}ASR_RECORD_DATA_T;

//语音识别结果回调
typedef void (*asr_result_cb)(ASR_RESULT_T *result);

//语音识别输入数据消息
typedef struct ASR_RECORD_MSG_T
{
	ASR_MSG_TYPE_T 		msg_type;		//消息类型
	ASR_RECORD_DATA_T 	record_data;	//pcm数据
	asr_result_cb 		asr_call_back;	//可以设置为NULL
	nlp_result_cb 		nlp_call_back;	//可以设置为NULL
}ASR_RECORD_MSG_T;

#ifdef __cplusplus
} /* extern "C" */	
#endif /* C++ */

#endif

