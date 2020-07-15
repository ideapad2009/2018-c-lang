
#include <string.h>
#include <time.h>
#include "crypto_api.h"
#include "cJSON.h"
#include "xfl2w_api.h"
#include "device_api.h"
#include "socket.h"
#include "http_api.h"
#include "debug_log.h"
#include "crypto_api.h"
#include "speex/speex.h"

#define SMS16K "sms16k"
#define SMS8K  "sms8k"


//讯飞语音识别
#define XFL2W_DEFAULT_ASR_URL		"http://api.xfyun.cn/v1/service/v1/iat"
#define XFL2W_DEFAULT_APP_ID		"5ad6b053"
#define XFL2W_DEFAULT_API_KEY		"71278aebbe262a054e24b85395256916"


#define HTTP_PREF       "http://"
#define HTTPS_PREF      "https://"
#define XFL2W_ASR_HOST  "api.xfyun.cn"
#define XFL2W_ASR_PORT  "80"
#define XFL2W_ASR_PATH  "/v1/service/v1/iat"
#define XFL2W_ASR_URL   HTTP_PREF""XFL2W_ASR_HOST""XFL2W_ASR_PATH

#define GET_ASR_REQUEST_HEADER  "POST "XFL2W_ASR_PATH" HTTP/1.1\r\n"      \
    "User-Agent: esp-idf/1.0 esp32\r\n"                                   \
    "Accept-Encoding: identity\r\n"                                       \
    "Connection: close\r\n"                                               \
    "Host: "XFL2W_ASR_HOST"\r\n"                                          \
	"X-Appid: %s\r\n"                                                     \
	"X-CurTime: %ld\r\n"                                                  \
	"X-Param: %s\r\n"                                                     \
	"X-CheckSum: %s\r\n"                                                  \
    "Content-Type: application/x-www-form-urlencoded; charset=utf-8\r\n"  \
    "Content-Length: %zu\r\n"                                             \
    "\r\n"


#define  X_RAW_PARAM                                                     \
	"{"                                                                  \
    "\"engine_type\":\"%s\","                                            \
    "\"aue\":\"%s\","                                                    \
    "\"vad_eos\":\"%d\","                                               \
    "\"scene\":\"%s\""                                                   \
	"}"

#define  X_SPEEX_PARAM                                                   \
	"{"                                                                  \
    "\"engine_type\":\"%s\","                                            \
    "\"aue\":\"%s\","                                                    \
    "\"speex_size\":\"%d\","                                             \
    "\"vad_eos\":\"%d\","                                               \
    "\"scene\":\"%s\""                                                   \
	"}" 

#define SM16K_AUE "speex-wb"
#define SM8K_AUE  "speex"




struct ASR_CONTEXT{
	const char * asr_url;
	const char * app_id;
	const char * api_key;

/* *****************************************************
#define ENC_BAND_WIDTH_NB  2 // for 8k  sample rate
#define ENC_BAND_WIDTH_WB  3 // for 16k sample rate
#define ENC_BAND_WIDTH_UWB 4 // for 32k sample rate
#* *****************************************************/
	int  enc_band_width;
	int  enc_frame_size;
	int  pcm_frame_samples;
	
	void *enc_hnd;		 // encode handle
	SpeexBits bits;
};


static const char *TAG_ASR = "[XUNFEI ASR]";

ASR_CONTEXT_T* xfl2w_asr_ctx_create(void) 
{
	ASR_CONTEXT_T* asr_ctx = esp32_malloc(sizeof(ASR_CONTEXT_T));
	if (asr_ctx) {
		memset(asr_ctx, 0, sizeof(ASR_CONTEXT_T));
		asr_ctx->asr_url = XFL2W_ASR_URL;
		asr_ctx->app_id  = XFL2W_DEFAULT_APP_ID;
		asr_ctx->api_key = XFL2W_DEFAULT_API_KEY;
		asr_ctx->enc_band_width = ENC_BAND_WIDTH_NB;
		asr_ctx->enc_frame_size = 62;
		asr_ctx->pcm_frame_samples = 160;
	}
//	ESP_LOGE(TAG_ASR, "\033[1;33m xfl2w_asr_ctx_create exit(%p)\033[0;0m", asr_ctx);
	return asr_ctx;
}

void xfl2w_asr_ctx_delete( ASR_CONTEXT_T**asr_ctxp) 
{
	if (asr_ctxp && asr_ctxp[0]) {
		if (asr_ctxp[0]->enc_hnd) {
			speex_bits_destroy(&asr_ctxp[0]->bits);
			speex_encoder_destroy(asr_ctxp[0]->enc_hnd);
			asr_ctxp[0]->enc_hnd = NULL;
		}
		esp32_free(asr_ctxp[0]);
		asr_ctxp[0] = NULL;
	}
}

int xfl2w_asr_encode_handle_create(ASR_CONTEXT_T *ctx, int enc_band_width, int enc_quality) 
{
	spx_int16_t quality = enc_quality;
	if (!ctx || ctx->enc_hnd) {
		return -1;
	}
	
//	ESP_LOGE(TAG_ASR, "\033[1;33m xfl2w_asr_encode_handle_create->ctx(%p) come in \033[0;0m", ctx);
	ASR_CONTEXT_T *enc_hnd;
	ctx->enc_frame_size = 0;
	switch (enc_band_width) {
		case ENC_BAND_WIDTH_NB:  ctx->enc_band_width = enc_band_width; enc_band_width = SPEEX_MODEID_NB; break;
		case ENC_BAND_WIDTH_WB:  ctx->enc_band_width = enc_band_width; enc_band_width = SPEEX_MODEID_WB; break;
		case ENC_BAND_WIDTH_UWB: ctx->enc_band_width = enc_band_width; enc_band_width = SPEEX_MODEID_UWB; break;
		default:
		    ctx->enc_band_width = enc_band_width < ENC_BAND_WIDTH_NB ? ENC_BAND_WIDTH_NB : ENC_BAND_WIDTH_WB;
			enc_band_width = enc_band_width < ENC_BAND_WIDTH_NB ? SPEEX_MODEID_NB : SPEEX_MODEID_WB;
	}
	enc_hnd = speex_encoder_init(speex_lib_get_mode(enc_band_width));
	if (!enc_hnd) {
		ESP_LOGE(TAG_ASR, "\033[1;33mxfl2w_asr_encode_handle_create exit(-1)\033[0;0m");
		return -1;
	}
	speex_encoder_ctl(enc_hnd, SPEEX_SET_QUALITY, &quality);
	speex_encoder_ctl(enc_hnd, SPEEX_GET_FRAME_SIZE, &ctx->pcm_frame_samples);
	speex_bits_init(&ctx->bits);
	ctx->enc_hnd = enc_hnd;
//	ESP_LOGE(TAG_ASR, "\033[1;33mxfl2w_asr_encode_handle_create(%p), ctx->enc_hnd(%p), exit(0)\033[0;0m", enc_hnd, ctx->enc_hnd);
	return 0;
}

void xfl2w_asr_encode_handle_destory(ASR_CONTEXT_T *ctx) 
{
	if (ctx && ctx->enc_hnd) {
//		ESP_LOGE(TAG_ASR, "\033[1;33mxfl2w_asr_encode_handle_destory(%p)\033[0;0m", ctx->enc_hnd);
		speex_bits_destroy(&ctx->bits);
		speex_encoder_destroy(ctx->enc_hnd);
		ctx->enc_hnd = NULL;
		ctx->bits = (SpeexBits){0};
	}
}

int xfl2w_asr_encode_handle_valid(ASR_CONTEXT_T *ctx) 
{
	return (ctx ? (ctx->enc_hnd ? 1 : 0) : 0);
}

int xfl2w_asr_encode_handle_frame_samples(ASR_CONTEXT_T *ctx) 
{
	if (!ctx || !ctx->enc_hnd) {
		return -1;
	}
	return ctx->pcm_frame_samples;
}

int xfl2w_asr_encode_handle_spx_frame_size(ASR_CONTEXT_T *ctx) 
{
	if (!ctx || !ctx->enc_hnd || !ctx->enc_band_width) {
		return -1;
	}
	return ctx->enc_frame_size > 0 ?ctx->enc_frame_size:((ctx->enc_band_width == ENC_BAND_WIDTH_NB)?62:((ctx->enc_band_width == ENC_BAND_WIDTH_WB)?106:212));
}


int xfl2w_asr_encode(ASR_CONTEXT_T *ctx, const char *src, int src_max_len, char* out, int out_max) 
{
	if ((ctx ? (ctx->enc_hnd ? 0 : 1) : 1)) {
		ESP_LOGE(TAG_ASR, "\033[1;33m ctx is error\033[0;0m");
		return -1;
	}
	if (src_max_len < ctx->pcm_frame_samples * 2) {
		ESP_LOGE(TAG_ASR, "\033[1;33m src pcm data is not enough\033[0;0m");
		return 0;
	}
	speex_bits_reset(&ctx->bits);
	speex_encode_int(ctx->enc_hnd, (spx_int16_t*)src, &ctx->bits);
	int write_bytes = speex_bits_write(&ctx->bits, out, out_max);
	if (write_bytes > 0 && write_bytes > ctx->enc_frame_size) {
		ctx->enc_frame_size = write_bytes;
	}
	return write_bytes;
}

#define CRYPTO_ERR_BASE64URL_BUFFER_TOO_SMALL               -0x002A  /**< Output buffer too small. */
#define CRYPTO_ERR_BASE64URL_INVALID_CHARACTER              -0x002C  /**< Invalid character in input. */
#define CRYPTO_BASE64_SIZE_T_MAX   ( (size_t) -1 ) /* SIZE_T_MAX is not standard */

/*
 * Encode a buffer into base64 format and urlencode the Encoded Buffer
 */
static int crypto_base64url_encode( unsigned char *dst, size_t dlen, size_t *olen,
                   const unsigned char *src, size_t slen, int check)
{
		static const unsigned char *base64_enc_map =  (const unsigned char *)"ABCDEFGHIJ" "KLMNOPQRST" "UVWXYZabcd" 
												                             "efghijklmn" "opqrstuvwx" "yz01234567" 
												                             "89+/";
	size_t i, n;
    int C1, C2, C3, Cidx;
    unsigned char *p;

    if( slen == 0 )
    {
        *olen = 0;
        return( 0 );
    }

    n = slen / 3 + ( slen % 3 != 0 );

    if( n > ( CRYPTO_BASE64_SIZE_T_MAX - 1 ) / 4 )
    {
        *olen = CRYPTO_BASE64_SIZE_T_MAX;
        return( CRYPTO_ERR_BASE64URL_BUFFER_TOO_SMALL );
    }

    if (!dst || (dlen <= 4 * n) || (dlen < 12*n + 1 && check)){ // check dlen enough or not
        int len = ( slen / 3 ) * 3;
        const unsigned char *src_back = src;
        n=0;
        for ( i = 0, p = dst; i < len; i += 3 ) {
            C1 = *src++;
            C2 = *src++;
            C3 = *src++;
            if (((C1 >> 2) & 0x3F) >= 62) {
                n+=2;
            }
            if (((((C1 &  3) << 4) + (C2 >> 4)) & 0x3F) >= 62) {
                n+=2;
            }
            if (((((C2 & 15) << 2) + (C3 >> 6)) & 0x3F) >= 62) {
                n+=2;
            }
            if ((C3 & 0x3F) >= 62) {
                n+=2;
            }
            n+=4;
        }
        if( i < slen ) {
            C1 = *src++;
            C2 = ( ( i + 1 ) < slen ) ? *src++ : 0;
            if (((C1 >> 2) & 0x3F) >= 62)
                n+=2;
            if (((((C1 & 3) << 4) + (C2 >> 4)) & 0x3F) >= 62)
                n+=2;

            n += 2;

            if( ( i + 1 ) < slen ) {
                if ((((C2 & 15) << 2) & 0x3F) >= 62)
                   n+=2;
                n++;
            } else {
                n += 3;
            }
            n+=3;
        }
        if (n + 1 > dlen || check) {
            *olen = n + 1;
            return(CRYPTO_ERR_BASE64URL_BUFFER_TOO_SMALL);
        }
        src = src_back;
    }


    n = ( slen / 3 ) * 3;

    for( i = 0, p = dst; i < n; i += 3 )
    {
        C1 = *src++;
        C2 = *src++;
        C3 = *src++;
		Cidx = (C1 >> 2) & 0x3F;
		(Cidx >= 62) ? ({*p++ = '%';*p++ = '2';*p++ = Cidx==62? 'B':'F';}) : (*p++ = base64_enc_map[Cidx]);
		Cidx = (((C1 &  3) << 4) + (C2 >> 4)) & 0x3F;
		(Cidx >= 62) ? ({*p++ = '%';*p++ = '2';*p++ = Cidx==62? 'B':'F';}) : (*p++ = base64_enc_map[Cidx]);
		Cidx = (((C2 & 15) << 2) + (C3 >> 6)) & 0x3F;
		(Cidx >= 62) ? ({*p++ = '%';*p++ = '2';*p++ = Cidx==62? 'B':'F';}) : (*p++ = base64_enc_map[Cidx]);
		Cidx = C3 & 0x3F;
		(Cidx >= 62) ? ({*p++ = '%';*p++ = '2';*p++ = Cidx==62? 'B':'F';}) : (*p++ = base64_enc_map[Cidx]);
    }

    if( i < slen )
    {
        C1 = *src++;
        C2 = ( ( i + 1 ) < slen ) ? *src++ : 0;
		Cidx = (C1 >> 2) & 0x3F;
		(Cidx >= 62) ? ({*p++ = '%';*p++ = '2';*p++ = Cidx==62? 'B':'F';}) : (*p++ = base64_enc_map[Cidx]);
		Cidx = (((C1 & 3) << 4) + (C2 >> 4)) & 0x3F;
		(Cidx >= 62) ? ({*p++ = '%';*p++ = '2';*p++ = Cidx==62? 'B':'F';}) : (*p++ = base64_enc_map[Cidx]);

        if( ( i + 1 ) < slen ) {
             Cidx = ((C2 & 15) << 2) & 0x3F;
			 (Cidx >= 62) ? ({*p++ = '%';*p++ = '2';*p++ = Cidx==62? 'B':'F';}) : (*p++ = base64_enc_map[Cidx]);
       } else {
			*p++ = '%';
			*p++ = '3';
			*p++ = 'D';
        }

		*p++ = '%';
		*p++ = '3';
		*p++ = 'D';
    }

    *olen = p - dst;
    *p = 0;

    return( 0 );
}


// need call esp32_free
static char *  xfl2w_make_b64_x_params(int enc_band_width, int speex_frame_size, int vad_eos_ms, char*scene) 
{
	const char *const smsnk  = enc_band_width == ENC_BAND_WIDTH_NB? SMS8K : SMS16K;
	const char *const aue    = (speex_frame_size > 0) ? (enc_band_width == ENC_BAND_WIDTH_NB? SM8K_AUE : SM16K_AUE) : "raw";
	size_t xparam_size       = snprintf(NULL, 0, X_SPEEX_PARAM, smsnk, aue, speex_frame_size, vad_eos_ms, scene) + 1;
	size_t b64_xparam_size = (xparam_size + 2) / 3 * 4 + 1;
	char * const xparams     = esp32_malloc(xparam_size);
	if (!xparams) 
		return NULL;
	char * const b64_xparams = esp32_malloc(b64_xparam_size);
	if (!b64_xparams) {
		esp32_free(xparams);
		return NULL;
	}
	if (speex_frame_size > 0) {
		xparam_size = sprintf(xparams, X_SPEEX_PARAM, smsnk, aue, speex_frame_size, vad_eos_ms, scene);
	} else {
		xparam_size = sprintf(xparams, X_RAW_PARAM, smsnk, aue, vad_eos_ms, scene);
	}
	DEBUG_LOGE(TAG_ASR, "%s", xparams);
	if (crypto_base64_encode((unsigned char *)b64_xparams, b64_xparam_size, &b64_xparam_size, (const unsigned char *) xparams, xparam_size) != 0) {
		esp32_free(xparams);
		esp32_free(b64_xparams);
		return NULL;
	}
	esp32_free(xparams);
	return b64_xparams;
	
}


static unsigned char * xfl2w_make_md5_check_sum_str(const char*api_key, const char*x_params_b64, time_t raw_sec) 
{
	int md5_check_data_size = snprintf(NULL, 0, "%s%ld%s", api_key, raw_sec, x_params_b64)+2;
	unsigned char * const md5_check_data   = esp32_malloc(md5_check_data_size);
	if (!md5_check_data)
		return NULL;
	md5_check_data_size = sprintf((char *)md5_check_data, "%s%ld%s", api_key, raw_sec, x_params_b64);
	unsigned char * const md5_check_sum	  = (unsigned char *)esp32_malloc(20);
	if (!md5_check_sum) {
		esp32_free(md5_check_data);
		return NULL;
	}
	crypto_md5(md5_check_data, md5_check_data_size, md5_check_sum);
	esp32_free(md5_check_data);
	unsigned char * const md5_check_sum_str	  = (unsigned char *)esp32_malloc(33);
	if (!md5_check_sum_str) {
		esp32_free(md5_check_sum);
		return NULL;
	}
	int idx;
	for(idx=0;idx < 16;idx++) {
		sprintf((char*)md5_check_sum_str + idx*2, "%02hhx", md5_check_sum[idx]);
	}
	return md5_check_sum_str;
}


static int get_asr_result(
	ASR_CONTEXT_T *_acount,
	char* const _str_ret,
	const size_t _str_ret_len,
	const unsigned char* _data,
	const size_t _data_len)
{
	int 	             err_no      = XFL2W_ASR_SUCCESS;
	sock_t	             sock        = INVALID_SOCK;
	char	            *str_buf     = NULL;
	ESP_LOGE(TAG_ASR, "\033[1;33mget_asr_result...\033[0;0m");
	sock = sock_connect(XFL2W_ASR_HOST, XFL2W_ASR_PORT);
	if (sock == INVALID_SOCK) {
		DEBUG_LOGE(TAG_ASR, "sock_connect fail");
		err_no = XFL2W_ASR_FAIL;
		goto GET_ASR_RESULT_ERROR;
	}
	sock_set_nonblocking(sock);
	{   // send data to xunfei yun iat.
		size_t	b64url_body_len = 0;

		// compute content-length.
		crypto_base64url_encode(NULL, 0, &b64url_body_len, _data, _data_len, 1);
		b64url_body_len += 6;
		//post http header		
		{ // send header
			char	         *x_params_b64      = NULL;
			unsigned char    *md5_check_sum_str = NULL;
			char             *header            = NULL;
			time_t            time_raw          = time(NULL);
			DEBUG_LOGE(TAG_ASR, "speex audio_data_len:[%zu]", _data_len);
			x_params_b64 = xfl2w_make_b64_x_params(_acount->enc_band_width, _acount->enc_frame_size, 4000, "main");
			if (!x_params_b64) {
				err_no = XFL2W_ASR_FAIL;
				goto GET_ASR_RESULT_ERROR;
			}
			md5_check_sum_str = xfl2w_make_md5_check_sum_str(_acount->api_key, x_params_b64, time_raw);
			if (!md5_check_sum_str) {
				esp32_free(x_params_b64);
				err_no = XFL2W_ASR_FAIL;
				goto GET_ASR_RESULT_ERROR;
			}

			header  = esp32_malloc(snprintf(NULL, 0, GET_ASR_REQUEST_HEADER, _acount->app_id, time_raw, x_params_b64, md5_check_sum_str, b64url_body_len-1) + 3);
			sprintf(header, GET_ASR_REQUEST_HEADER, _acount->app_id, time_raw, x_params_b64, md5_check_sum_str, b64url_body_len-1);
			DEBUG_LOGE(TAG_ASR, "[%lld ms] b64url_encode_audio_len(%zu), content-length(%zu)", get_time_of_day(), b64url_body_len - 6 -1, b64url_body_len-1);
			DEBUG_LOGE(TAG_ASR, "header: \r\n%s", header);
			if (sock_writen_with_timeout(sock, header, strlen(header), 1000) != strlen(header)) {
				esp32_free(header);
				esp32_free(x_params_b64);
				esp32_free(md5_check_sum_str);
				DEBUG_LOGE(TAG_ASR, "sock_write http header fail");
				err_no = XFL2W_ASR_FAIL;
				goto GET_ASR_RESULT_ERROR;
			}
			esp32_free(header);
			esp32_free(x_params_b64);
			esp32_free(md5_check_sum_str);
		}{  // base64 encode audio and urlencode audio data. output url_body, url_body_len
			size_t	olen		= 0;
			unsigned char *b64url_body = esp32_malloc(b64url_body_len);//content-length: b64url_body_len
			if (!b64url_body) {
				DEBUG_LOGE(TAG_ASR, "malloc for crypto_base64url_encode fail");
				err_no = XFL2W_ASR_POST_AUDIO_FAIL;
				goto GET_ASR_RESULT_ERROR;
			}
			sprintf((char*)b64url_body, "audio=");//audio=
			if (crypto_base64url_encode(b64url_body+6, b64url_body_len - 6, &olen, _data, _data_len, 0) != 0) {
				DEBUG_LOGE(TAG_ASR, "crypto_base64_encode fail");
				err_no = XFL2W_ASR_POST_AUDIO_FAIL;
				esp32_free(b64url_body);
				goto GET_ASR_RESULT_ERROR;
			}
			b64url_body_len = olen + 6;
			if (sock_writen_with_timeout(sock, b64url_body, b64url_body_len, 2000) != b64url_body_len) {
				DEBUG_LOGE(TAG_ASR, "sock_write http body fail");
				err_no = XFL2W_ASR_FAIL;
				goto GET_ASR_RESULT_ERROR;
			}
			DEBUG_LOGE(TAG_ASR, "[%lld ms] all asr data send pass content-length:%zu", get_time_of_day(), b64url_body_len);
			esp32_free(b64url_body);
		}
	}
	/* Read HTTP response */
	const int recv_buf_size = 1024;
	str_buf = esp32_malloc(1024);
	if (!str_buf) {
		DEBUG_LOGE(TAG_ASR, "esp32_malloc for recv server data failed");
		err_no = XFL2W_ASR_FAIL;
	}
	memset(str_buf, 0, 1024);
	DEBUG_LOGE(TAG_ASR, "start recv server data");
	sock_readn_with_timeout(sock, str_buf, recv_buf_size - 1, 2000);
	DEBUG_LOGE(TAG_ASR, "str_reply=%s\n", str_buf);
	
	if (http_get_error_code(str_buf) != 200)
	{
		DEBUG_LOGE(TAG_ASR, "reply:[%s]", str_buf);
		err_no = XFL2W_ASR_FAIL;
		goto GET_ASR_RESULT_ERROR;
	}
	
	char *pBody = http_get_body(str_buf);
	if (NULL == pBody)
	{
		err_no = XFL2W_ASR_FAIL;
		goto GET_ASR_RESULT_ERROR;
	}

	cJSON *pJson = cJSON_Parse(pBody);
    if (NULL == pJson) {
    	err_no = XFL2W_ASR_FAIL;
		DEBUG_LOGE(TAG_ASR, "invalid json:[%s]", pBody);
		goto GET_ASR_RESULT_ERROR;
	}

    cJSON *pJsonSub_error = cJSON_GetObjectItem(pJson, "code");
	if (NULL == pJsonSub_error) {
		cJSON_Delete(pJson);
		err_no = XFL2W_ASR_FAIL;
		goto GET_ASR_RESULT_ERROR;
	}

	err_no = pJsonSub_error->valueint;
	if (err_no == 0) {
		cJSON *pJsonSub_text = cJSON_GetObjectItem(pJson, "data");
		if (NULL != pJsonSub_text) {
			if (pJsonSub_text->type == cJSON_String) {
				snprintf(_str_ret, _str_ret_len, "%s", pJsonSub_text->valuestring);
			} else if (pJsonSub_text->type == cJSON_Number) {
				snprintf(_str_ret, _str_ret_len, "%d", pJsonSub_text->valueint);
			}
		}
	} else {
		switch(err_no) {
			case XFL2W_ASR_ILLEGAL_SCCESS:
				ESP_LOGE(TAG_ASR, "XFL2W_ASR_ILLEGAL_SCCESS");
			break;
			case XFL2W_ASR_INVALID_PARAMETER:
				ESP_LOGE(TAG_ASR, "XFL2W_ASR_INVALID_PARAMETER");
			break;
			case XFL2W_ASR_ILLEGAL_PARAMETER:
				ESP_LOGE(TAG_ASR, "XFL2W_ASR_ILLEGAL_PARAMETER");
			break;
			case XFL2W_ASR_ILLEGAL_TEXT_AUDIO_LENGTH:
				ESP_LOGE(TAG_ASR, "XFL2W_ASR_ILLEGAL_TEXT_AUDIO_LENGTH");
			break;
			case XFL2W_ASR_NO_LICENSE:
				ESP_LOGE(TAG_ASR, "XFL2W_ASR_NO_LICENSE");
			break;
			case XFL2W_ASR_TIME_OUT:
				ESP_LOGE(TAG_ASR, "XFL2W_ASR_TIME_OUT");
			break;
			case XFL2W_ASR_ENGINE_ERROR:
				ESP_LOGE(TAG_ASR, "XFL2W_ASR_ENGINE_ERROR");
			break;
			case XFL2W_ASR_NO_VCN_AUTH:
				ESP_LOGE(TAG_ASR, "XFL2W_ASR_NO_VCN_AUTH");
			break;
			default:
				ESP_LOGE(TAG_ASR, "unknow error:%d", err_no);
				break;
		}
	}

	cJSON_Delete(pJson);

GET_ASR_RESULT_ERROR:
	sock_close(sock);
	if (str_buf) {
		esp32_free(str_buf);
		str_buf = NULL;
	}
	ESP_LOGE(TAG_ASR, "\033[1;33mget_asr_result exit(%d)\033[0;0m", err_no);
	return err_no;
}



int xfl2w_asr(
	ASR_CONTEXT_T *_contex,
	char* str_reply, 
	size_t str_reply_len, 
	unsigned char *_data, 
	size_t _data_len)
{
	//post audio data and get result 
	return get_asr_result(_contex, str_reply, str_reply_len, _data, _data_len);
}






