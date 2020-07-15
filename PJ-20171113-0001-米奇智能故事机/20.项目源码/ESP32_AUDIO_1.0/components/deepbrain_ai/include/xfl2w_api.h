/**
 * @file   xf_l2w_api.h 
 * @brief  xunfei yun listen to write api
 * 
 *  This file contains the quick application programming interface (API) declarations 
 *  of deep brain nlp interface. Developer can include this file in your project to build applications.
 *  For more information, please read the developer guide.
 * 
 * @author  Cal yan
 * @version 1.0.0
 * @date    2018/4/18
 * 
 * @see        
 * 
 * History:
 * index    version        date            author        notes
 * 0        1.0            2018/4/18       Cal yan       Create this file
 */

#ifndef __XF_L2W_API_H__
#define __XF_L2W_API_H__
#include "flash_config_manage.h"

#ifdef __cplusplus
extern "C" {
#endif /* C++ */

#define ENC_BAND_WIDTH_NB  2 // for 8k  sample rate
#define ENC_BAND_WIDTH_WB  3 // for 16k sample rate
#define ENC_BAND_WIDTH_UWB 4 // for 32k sample rate

typedef struct ASR_CONTEXT ASR_CONTEXT_T;


//语音识别错误码
typedef enum
{
	//识别成功
	XFL2W_ASR_SUCCESS = 0,
	//识别失败
	XFL2W_ASR_FAIL,
	//获取认证失败
	XFL2W_ASR_GET_TOKEN_FAIL,
	//上传音频文件失败
	XFL2W_ASR_POST_AUDIO_FAIL,
	//识别结果错误
	XFL2W_ASR_RESULT_ERROR,
	//没有权限
	XFL2W_ASR_ILLEGAL_SCCESS = 10105,
	//无效参数
	XFL2W_ASR_INVALID_PARAMETER = 10106,
	//非法参数值
	XFL2W_ASR_ILLEGAL_PARAMETER = 10107,
	//文本/音频长度非法
	XFL2W_ASR_ILLEGAL_TEXT_AUDIO_LENGTH = 10109,
	//无授权许可
	XFL2W_ASR_NO_LICENSE = 10110,
	//超时
	XFL2W_ASR_TIME_OUT = 10114,
	//引擎错误
	XFL2W_ASR_ENGINE_ERROR = 10700,
	//无发音人授权
	XFL2W_ASR_NO_VCN_AUTH = 11200,
	//产品线当前日请求数超过限额
	XFL2W_ASR_APPID_AUTH_NUM_LIMIT = 11201,
}XFL2W_ASR_RESULT;

int xfl2w_asr_encode_handle_create(ASR_CONTEXT_T * ctx, int enc_band_width, int enc_quality);
void xfl2w_asr_encode_handle_destory(ASR_CONTEXT_T * ctx);
int xfl2w_asr_encode_handle_valid(ASR_CONTEXT_T *ctx);
int xfl2w_asr_encode_handle_spx_frame_size(ASR_CONTEXT_T * ctx);
int xfl2w_asr_encode_handle_frame_samples(ASR_CONTEXT_T * ctx);
int xfl2w_asr_encode(ASR_CONTEXT_T *ctx, const char *src, int src_max_len, char* out, int out_max);
int xfl2w_asr(ASR_CONTEXT_T *_acount, char* str_reply, size_t str_reply_len, unsigned char *_data, size_t _data_len);
ASR_CONTEXT_T* xfl2w_asr_ctx_create(void);



#ifdef __cplusplus
}
#endif /* C++ */
#endif


