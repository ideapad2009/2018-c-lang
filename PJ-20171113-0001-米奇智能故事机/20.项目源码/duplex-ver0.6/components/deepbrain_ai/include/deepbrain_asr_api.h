#ifndef DEEP_BRAIN_ASR_API_H__
#define DEEP_BRAIN_ASR_API_H__

#ifdef __cplusplus
extern "C" {
#endif /* C++ */

//语音识别错误码
typedef enum DP_ASR_ERRNO_t
{
	DP_ASR_ERRNO_OK,			//成功
	DP_ASR_ERRNO_FAIL,			//失败
	DP_ASR_ERRNO_NO_NETWORK,	//无网络
	DP_ASR_ERRNO_POOR_NETWORK,	//网络差
	DP_ASR_ERRNO_NOT_ENOUGH_MEM,//内存不足
	DP_ASR_ERRNO_INVALID_PARAMS,//无效参数
	DP_ASR_ERRNO_ENCODE_FAIL,	//编码失败
	DP_ASR_ERRNO_INVALID_JSON,	//无效的json数据
}DP_ASR_ERRNO_t;

//识别语言类型
typedef enum DP_ASR_LANG_TYPE_t
{
	DP_ASR_LANG_TYPE_CHINESE 		= 1536,	//普通话(支持简单的英文识别)
	DP_ASR_LANG_TYPE_PURE_CHINESE 	= 1537,	//普通话(纯中文识别)
	DP_ASR_LANG_TYPE_ENGLISH 		= 1737,	//英语
	DP_ASR_LANG_TYPE_CHINESE_YUEYU 	= 1637,	//粤语
	DP_ASR_LANG_TYPE_CHINESE_SICHUAN= 1837,	//四川话
	DP_ASR_LANG_TYPE_CHINESE_FAR 	= 1936,	//普通话远场
}DP_ASR_LANG_TYPE_t;

//语音识别模式
typedef enum DP_ASR_MODE_t
{
	DP_ASR_MODE_ASR = 0,	//语音识别模式
	DP_ASR_MODE_ASR_NLP,	//语音识别 + 语义处理模式
}DP_ASR_MODE_t;

typedef struct ASR_CONTEXT ASR_CONTEXT_T;

DP_ASR_ERRNO_t deep_brain_asr_create(ASR_CONTEXT_T ** ctx_out);
DP_ASR_ERRNO_t deep_brain_asr_connect(ASR_CONTEXT_T *ctx);
int deep_brain_asr_valid(ASR_CONTEXT_T* ctx);
DP_ASR_ERRNO_t deep_brain_asr_set_frame(ASR_CONTEXT_T* ctx, int samples_frame_ms);
int deep_brain_asr_get_frame_samples(ASR_CONTEXT_T* ctx);
int deep_brain_asr_get_frame_size(ASR_CONTEXT_T* ctx);
void deep_brain_asr_set_mode(ASR_CONTEXT_T* ctx, int mode);
void deep_brain_asr_set_lang(ASR_CONTEXT_T* ctx, int lang);
DP_ASR_ERRNO_t deep_brain_asr_result(ASR_CONTEXT_T *ctx, char* str_reply, const size_t str_reply_len);
DP_ASR_ERRNO_t deep_brain_asr_close(ASR_CONTEXT_T* ctx);
DP_ASR_ERRNO_t deep_brain_asr_write(ASR_CONTEXT_T* ctx, const void * const buff, const size_t len, const size_t timeout_msec);
void deep_brain_asr_set_url(const char *url);
DP_ASR_ERRNO_t deep_brain_asr_decode(const char *json_result, const char *asr_text, const size_t asr_text_len);

#ifdef __cplusplus
}
#endif /* C++ */
#endif



