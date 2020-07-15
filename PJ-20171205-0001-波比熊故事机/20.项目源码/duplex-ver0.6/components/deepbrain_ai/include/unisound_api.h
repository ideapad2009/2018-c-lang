#ifndef __UNISOUND_API_H__
#define __UNISOUND_API_H__

#ifdef __cplusplus
extern "C" {
#endif /* C++ */

//语音识别错误码
typedef enum
{
	UNISOUND_ASR_ERRNO_OK = 0,			//识别成功
	UNISOUND_ASR_ERRNO_FAIL,			//识别失败
	UNISOUND_ASR_ERRNO_INVALID_PARAMS,	//无效参数
	UNISOUND_ASR_ERRNO_DATA_TOO_LONG,	//数据太长
	UNISOUND_ASR_ERRNO_WRITE_FAIL,		//上传语音失败
}UNISOUND_ASR_ERRNO_T;

//云知声识别账号
typedef struct
{
	char user_id[32];
	char device_id[32];
	char appkey[64];
}UNISOUND_ASR_ACCOUNT_T;

void *unisound_asr_create(const UNISOUND_ASR_ACCOUNT_T *asr_acount);
UNISOUND_ASR_ERRNO_T unisound_asr_wirte(void *asr_handle, const char *data, const uint32_t data_len);
UNISOUND_ASR_ERRNO_T unisound_asr_result(void *asr_handle, char* str_reply, const size_t str_reply_len);
void unisound_asr_delete(void *asr_handle);
void unisound_asr_end(void *asr_handle);


#ifdef __cplusplus
} /* extern "C" */	
#endif /* C++ */

#endif /* __BAIDU_API_H__ */

