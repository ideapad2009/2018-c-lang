#include <string.h>
#include "adpcm-lib.h"
#include "socket.h"
#include "debug_log.h"
#include "device_api.h"
#include "deepbrain_asr_api.h"
#include "crypto_api.h"
#include "cJSON_Utils.h"
#include "userconfig.h"
#include "flash_config_manage.h"

#define DP_ASR_RECORD_FILE 0
#define DP_ASR_ADPCM_FRAME_SIZE 168
#define DP_ASR_ADPCM_FRAME_SAMPLES 329

#define ASR_DBG_FILE  "DeepBrainAsr-%d.adpcm"

#define X_SESSION_KEY "asr.cloud.freetalk"
#define X_APP_KEY     "asr.cloud.freetalk"

#define CONNECTION_KEEP_ALIVE "Keep-Alive"
#define CONNECTION_CLOSE      "Close"
#define X_NLP_SWITCH_YES      "yes"
#define X_NLP_SWITCH_NO       "no"

//static const char *DeepBrain_ASR_URL = 
//	"http://122.144.200.2:9036/open-api/asr/recognise";

static const char *DeepBrain_ASR_URL = 
	"http://asr.deepbrain.ai/open-api/asr/recognise";


static char g_deepbrain_asr_url[128] = {0};

static const char *TAG_ASR = "[Deepbrain ASR]";

static const char *HTTP_POST_HEADER = 
		"POST %s HTTP/1.0\r\n"
		"HOST: %s:%s\r\n"
		"Content-Type: application/octet-stream;charset=iso-8859-1\r\n"
		"Connection: %s\r\n"
		"Content-Length: %zu\r\n"
		"CreatedTime: %s\r\n"
		"PrivateKey: %s\r\n"
		"Key: %s\r\n"
		"Nonce: %s\r\n"
		"x-udid: %s\r\n"
		"x-task-config: audioformat=adpcm16k16bit,framesize=%zu,lan=%d,vadSeg=500,index=%d\r\n"
		"x-nlp-switch: %s\r\n"
		"x-user-id: %s\r\n"
		"x-device-id: %s\r\n"
		"x-app-id: %s\r\n"
		"x-robot-id: %s\r\n"
		"x-sdk-version: 0.1\r\n"
		"x-result-format: json\r\n\r\n";

//计算 存储 milliseconds毫秒音频(samples_per_sec 采样率，bytes_per_sample字节 采样精度)数据， 所需要的字节数
#define RAW_PCM_LEN_MS(milliseconds, samples_per_sec, bytes_per_sample)  (2L*(milliseconds)*(samples_per_sec)*(bytes_per_sample) / 1000) 

//计算 存储 seconds秒音频(samples_per_sec 采样率，bytes_per_sample字节 采样精度)数据， 所需要的字节数
#define RAW_PCM_LEN_S(seconds, samples_per_sec, bytes_per_sample)  (2L*(seconds)*(samples_per_sec)*(bytes_per_sample))

//ASR参数设置
typedef struct DP_ASR_PARAMS_t
{
	DP_ASR_MODE_t mode;
	DP_ASR_LANG_TYPE_t lang;
}DP_ASR_PARAMS_t;

//语音识别上下文
struct ASR_CONTEXT {
	const char *asr_url;
	void  		*adpcm_ctx; // encode handle
	sock_t      sock;
	int         errnum;
	int         frame_samples;
	int         adpcm_frame_size;
	uint8_t 	udid[64];// uuid

	//pcm、adpcm数据缓存
	int         index;			//数据发送索引, 1~n, -n表示录音发送结束
	char     	inpcm[16*1000];	//保存剩余PCM数据
	char     	inpcm_swap[16*1000];	//PCM数据交换缓存
	uint32_t    inpcm_len;		//保存pcm数据长度
	char        adpcm[RAW_PCM_LEN_S(1, 16000, 2)/4]; //保存ADPCM数据
	uint32_t    adpcm_len;							//保存ADPCM数据长度

	//发送缓存
	char        http_post_head[1024*32];
	int         http_post_head_len;
	
	char      	time_stamp[32];
	char       	nonce[64];
	char       	private_key[64];
	char      	user_id[64];
	char      	device_id[64];
	char 		app_id[64];
	char		robot_id[64];

	char		domain[64];
	char		port[8];
	char		params[64];
	
	DP_ASR_PARAMS_t asr_params;

	//录音存储
	int         file_num;
	FILE        *filep;
};

DP_ASR_ERRNO_t deep_brain_asr_create(ASR_CONTEXT_T **ctx_out)
{
	//创建asr handle
	ASR_CONTEXT_T * ctx = (ASR_CONTEXT_T*)esp32_malloc(sizeof(ASR_CONTEXT_T));
	if (ctx == NULL) 
	{
		DEBUG_LOGE(TAG_ASR, "esp32_malloc(sizeof(ASR_CONTEXT_T)) failed");
		return DP_ASR_ERRNO_NOT_ENOUGH_MEM;
	}
	memset(ctx, 0, sizeof(ASR_CONTEXT_T));
	ctx->sock = INVALID_SOCK;
	*ctx_out = ctx;
	
	return DP_ASR_ERRNO_OK;
}

DP_ASR_ERRNO_t deep_brain_asr_connect(ASR_CONTEXT_T *ctx)
{
	if (ctx == NULL)
	{
		return DP_ASR_ERRNO_INVALID_PARAMS;
	}
	
	if (ctx->sock >= 0)
	{
		sock_close(ctx->sock);
		ctx->sock = INVALID_SOCK;
	}
	ctx->errnum = 0;

	//语音流识别地址可通过串口命令切换
	if (strlen(g_deepbrain_asr_url) > 0)
	{
		if (sock_get_server_info(g_deepbrain_asr_url, &ctx->domain, &ctx->port, &ctx->params) != 0)
		{
			DEBUG_LOGE(TAG_ASR, "sock_get_server_info failed");
			return DP_ASR_ERRNO_NO_NETWORK;
		}
	}
	else
	{
		if (sock_get_server_info(DeepBrain_ASR_URL, &ctx->domain, &ctx->port, &ctx->params) != 0)
		{
			DEBUG_LOGE(TAG_ASR, "sock_get_server_info failed");
			return DP_ASR_ERRNO_NO_NETWORK;
		}
	}

	//建立socket
	if((ctx->sock = sock_connect(ctx->domain, ctx->port)) < 0 ) 
	{
		DEBUG_LOGE(TAG_ASR, "sock_connect(%s,%s) failed", ctx->domain, ctx->port);
		return DP_ASR_ERRNO_NO_NETWORK;
	}
	sock_set_nonblocking(ctx->sock);
	DEBUG_LOGI(TAG_ASR, "sock_connect(%s,%s) success", ctx->domain, ctx->port);

#if DP_ASR_RECORD_FILE
	if (ctx->filep) 
	{
		fclose(ctx->filep);
		ctx->filep = NULL;
		ctx->file_num++;
	}
	
	char filename[100] = {0};
	sprintf(filename, ASR_DBG_FILE, ctx->file_num);
	ctx->filep = fopen(filename, "wb");
#endif

	//初始化参数
	ctx->inpcm_len = 0;
	ctx->adpcm_len = 0;
	ctx->adpcm_frame_size = DP_ASR_ADPCM_FRAME_SIZE;
	ctx->frame_samples 	  = DP_ASR_ADPCM_FRAME_SAMPLES;
	get_flash_cfg(FLASH_CFG_USER_ID, ctx->user_id);
	get_flash_cfg(FLASH_CFG_DEVICE_ID, ctx->device_id);
	get_flash_cfg(FLASH_CFG_APP_ID, ctx->app_id);
	get_flash_cfg(FLASH_CFG_ROBOT_ID, ctx->robot_id);
	memset(ctx->udid, 0, sizeof(ctx->udid));
	if (!ctx->user_id[0]) 
	{
		memcpy(ctx->user_id, ctx->device_id, sizeof(ctx->device_id));
	}
	
	return DP_ASR_ERRNO_OK;
}

int deep_brain_asr_valid(ASR_CONTEXT_T* ctx)
{
//	DEBUG_LOGE(TAG_ASR, "ctx:%p, ctx->sock:%d, ctx->errnum:%d", ctx, ctx->sock, ctx->errnum);
	return (ctx && ctx->sock >= 0 && !ctx->errnum);
}

/* 
 * samples_per_pcm_block 
 * 返回值： 每个音频数据块中的样本数
 */
DP_ASR_ERRNO_t deep_brain_asr_set_frame(
	ASR_CONTEXT_T* ctx, 
	int samples_frame_ms)
{
	int samples_per_frame = samples_frame_ms*16;
	
	if (ctx == NULL || samples_frame_ms <= 0) 
	{
		return DP_ASR_ERRNO_INVALID_PARAMS;
	}
	
	ctx->adpcm_frame_size = samples_per_frame / 8 * 4; // adpcm_block_size 必须是4的倍数
	ctx->frame_samples    = 2 * ctx->adpcm_frame_size  - 7;
	DEBUG_LOGI(TAG_ASR, "adpcm_frame_size[%d], frame_samples[%d]", 
		ctx->adpcm_frame_size, ctx->frame_samples);

	return DP_ASR_ERRNO_OK;
}

int deep_brain_asr_get_frame_samples(ASR_CONTEXT_T* ctx)
{
	if (!ctx) 
	{
		return -1;
	}
	
	return ctx->frame_samples;
}

int deep_brain_asr_get_frame_size(ASR_CONTEXT_T* ctx)
{
	if (!ctx) 
	{
		return -1;
	}
	
	return ctx->adpcm_frame_size;
}

void deep_brain_asr_set_mode(ASR_CONTEXT_T* ctx, int mode)
{
	if (!ctx) 
	{
		return;
	}

	ctx->asr_params.mode = mode;
}

void deep_brain_asr_set_lang(ASR_CONTEXT_T* ctx, int lang)
{
	if (!ctx) 
	{
		return;
	}

	ctx->asr_params.lang = lang;
}

void deep_brain_asr_set_url(const char *url)
{
	snprintf(g_deepbrain_asr_url, sizeof(g_deepbrain_asr_url), "%s", url);
}

static DP_ASR_ERRNO_t deep_brain_asr_encode(
	ASR_CONTEXT_T *ctx, 
	const char *pcm, 
	const size_t pcm_len)
{
	const int16_t *pcm_data = (int16_t*)pcm;
	int in_samples = pcm_len / 2;
	int pcm_copy_len = 0;
	int pcm_remain_len = 0;
	int min_pcm_frame_size = ctx->frame_samples * 2;//ADPCM编码一帧需要的最小字节数
	
	if (ctx == NULL) 
	{
		return DP_ASR_ERRNO_INVALID_PARAMS;
	}

	//创建adpcm编码句柄
	if (ctx->adpcm_ctx == NULL) 
	{
		int32_t initial_deltas [2] = {0};
		initial_deltas[0] = pcm_data[0];
		
		while (in_samples--) 
		{
			initial_deltas[0] = (initial_deltas[0] + pcm_data[in_samples]) / 2;
		}
		
		initial_deltas[1] = initial_deltas[0];
		ctx->adpcm_ctx = adpcm_create_context(1, 0, NOISE_SHAPING_OFF, initial_deltas);
		if (ctx->adpcm_ctx == NULL)
		{
			DEBUG_LOGE(TAG_ASR, "adpcm_create_context failed");
			return DP_ASR_ERRNO_NOT_ENOUGH_MEM;
		}
		else
		{
			DEBUG_LOGI(TAG_ASR, "adpcm_create_context success");
		}
	}

	//如果pcm为NULL，表示编码最后一帧数据
	if (pcm == NULL && pcm_len == 0)
	{
		if (ctx->inpcm_len > 0)
		{//存在剩余数据
			adpcm_encode_block(ctx->adpcm_ctx, (uint8_t *)ctx->adpcm, &ctx->adpcm_len, ctx->inpcm, ctx->inpcm_len/2);	
		}
		else
		{//没有剩余数据
			ctx->adpcm_len = 0;
		}
	}
	else if (pcm != NULL && pcm_len > 0)
	{
		//复制pcm数据至inpcm缓存
		if (ctx->inpcm_len + pcm_len > sizeof(ctx->inpcm))
		{
			pcm_copy_len = sizeof(ctx->inpcm) - ctx->inpcm_len;
			pcm_remain_len = pcm_len - pcm_copy_len;
		}
		else
		{
			pcm_copy_len = pcm_len;
			pcm_remain_len = 0;
		}
		memcpy(ctx->inpcm + ctx->inpcm_len, pcm, pcm_copy_len);
		ctx->inpcm_len += pcm_copy_len;
		//DEBUG_LOGE(TAG_ASR, "ctx->inpcm_len[%d],pcm_remain_len[%d],pcm_copy_len[%d][%d]", 
		//		ctx->inpcm_len, pcm_remain_len, pcm_copy_len, sizeof(ctx->inpcm));
		
		if (ctx->inpcm_len >= min_pcm_frame_size)
		{//字节足够，编码一帧
			adpcm_encode_block(ctx->adpcm_ctx, (uint8_t *)ctx->adpcm, &ctx->adpcm_len, ctx->inpcm, ctx->frame_samples);
			ctx->inpcm_len -= min_pcm_frame_size;
			if (ctx->inpcm_len > 0)
			{
				memcpy(ctx->inpcm_swap, ctx->inpcm + min_pcm_frame_size, ctx->inpcm_len);
			}
			
			//DEBUG_LOGE(TAG_ASR, "ctx->inpcm_len[%d],pcm_remain_len[%d]", 
			//	ctx->inpcm_len, pcm_remain_len);
			
			if ((ctx->inpcm_len + pcm_remain_len) > 0 
				&& ctx->inpcm_len + pcm_remain_len <= sizeof(ctx->inpcm))
			{
				memcpy(ctx->inpcm_swap + ctx->inpcm_len, pcm + pcm_copy_len, pcm_remain_len);
				ctx->inpcm_len += pcm_remain_len;
				memcpy(ctx->inpcm, ctx->inpcm_swap, ctx->inpcm_len);
			}
			else if ((ctx->inpcm_len + pcm_remain_len) > 0 
				&& ctx->inpcm_len + pcm_remain_len > sizeof(ctx->inpcm))
			{
				memcpy(ctx->inpcm_swap + ctx->inpcm_len, pcm + pcm_copy_len, sizeof(ctx->inpcm) - ctx->inpcm_len);
				ctx->inpcm_len = sizeof(ctx->inpcm);
				memcpy(ctx->inpcm, ctx->inpcm_swap, ctx->inpcm_len);
				DEBUG_LOGE(TAG_ASR, "pcm data overflow, throw [%d]bytes pcm data", ctx->inpcm_len + pcm_remain_len - sizeof(ctx->inpcm));
			}
			else
			{

			}
		}
		else
		{//字节不够，等到下一帧编码
			ctx->adpcm_len = 0;
		}
	}
	else
	{
		return DP_ASR_ERRNO_INVALID_PARAMS;
	}

	return DP_ASR_ERRNO_OK;
}

// len <= 14k
DP_ASR_ERRNO_t deep_brain_asr_write(
	ASR_CONTEXT_T* ctx, 
	const void * const pcm_data, 
	const size_t pcm_len, 
	const size_t timeout_msec)
{
	int ret;
	int drop = 0;
	int loop_num = 0;
	
	if (ctx == NULL || timeout_msec == 0) 
	{
		return DP_ASR_ERRNO_INVALID_PARAMS;
	}

	if (ctx->sock == INVALID_SOCK)
	{
		return DP_ASR_ERRNO_NO_NETWORK;
	}

	//ADPCM编码
	if (deep_brain_asr_encode(ctx, pcm_data, pcm_len) != DP_ASR_ERRNO_OK)
	{
		DEBUG_LOGE(TAG_ASR, "deep_brain_asr_encode failed");
		return DP_ASR_ERRNO_ENCODE_FAIL;
	}

	//没有数据，则不需要发送
	if (ctx->adpcm_len == 0 && ctx->index > 0)
	{
		return DP_ASR_ERRNO_OK;
	}
	
	//组包
	if (ctx->udid[0] == 0) 
	{
		crypto_generate_nonce_hex(ctx->udid, sizeof(ctx->udid));
		ctx->index = 0;
	}
	
	if (ctx->index >= 0) 
	{
		ctx->index ++;
	} 
	else if (ctx->index < 0) 
	{
		ctx->index = ctx->index - 1;
	}

	crypto_generate_nonce((unsigned char *)ctx->nonce, sizeof(ctx->nonce) - 1);
	crypto_time_stamp((unsigned char*)ctx->time_stamp, sizeof(ctx->time_stamp));
	crypto_generate_private_key((uint8_t *)ctx->private_key, sizeof(ctx->private_key), ctx->nonce, ctx->time_stamp, ctx->robot_id);
	if (ctx->asr_params.mode == DP_ASR_MODE_ASR)
	{
		if (ctx->index < 0)
		{
			
			ctx->http_post_head_len = snprintf(ctx->http_post_head, sizeof(ctx->http_post_head), HTTP_POST_HEADER, 
				ctx->params, ctx->domain, ctx->port,
				CONNECTION_CLOSE, ctx->adpcm_len, ctx->time_stamp, 
				ctx->private_key, ctx->robot_id, ctx->nonce, 
				ctx->udid, ctx->adpcm_len, ctx->asr_params.lang, ctx->index, X_NLP_SWITCH_NO,
				ctx->user_id, ctx->device_id, ctx->app_id, ctx->robot_id);
		}
		else
		{

		
			ctx->http_post_head_len = snprintf(ctx->http_post_head, sizeof(ctx->http_post_head), HTTP_POST_HEADER, 
				ctx->params, ctx->domain, ctx->port,
				CONNECTION_KEEP_ALIVE, ctx->adpcm_len, ctx->time_stamp, 
				ctx->private_key, ctx->robot_id, ctx->nonce, 
				ctx->udid, ctx->adpcm_len, ctx->asr_params.lang, ctx->index, X_NLP_SWITCH_NO,
				ctx->user_id, ctx->device_id, ctx->app_id, ctx->robot_id);
		}
	}
	else
	{
		if (ctx->index < 0)
		{
			ctx->http_post_head_len = snprintf(ctx->http_post_head, sizeof(ctx->http_post_head), HTTP_POST_HEADER, 
				ctx->params, ctx->domain, ctx->port,
				CONNECTION_CLOSE, ctx->adpcm_len, ctx->time_stamp, 
				ctx->private_key, ctx->robot_id, ctx->nonce, 
				ctx->udid, ctx->adpcm_len, ctx->asr_params.lang, ctx->index, X_NLP_SWITCH_YES,
				ctx->user_id,ctx->device_id, ctx->app_id, ctx->robot_id);
		}
		else
		{
			ctx->http_post_head_len = snprintf(ctx->http_post_head, sizeof(ctx->http_post_head), HTTP_POST_HEADER, 
				ctx->params, ctx->domain, ctx->port,
				CONNECTION_KEEP_ALIVE, ctx->adpcm_len, ctx->time_stamp, 
				ctx->private_key, ctx->robot_id, ctx->nonce, 
				ctx->udid, ctx->adpcm_len, ctx->asr_params.lang, ctx->index, X_NLP_SWITCH_YES,
				ctx->user_id,ctx->device_id, ctx->app_id, ctx->robot_id);
		}
	}
	
	//DEBUG_LOGW(TAG_ASR, "@line[%d] headerlen[%d], httpheader: %s", __LINE__, ctx->http_post_head_len, ctx->http_post_head);
	memcpy(ctx->http_post_head + ctx->http_post_head_len, ctx->adpcm, ctx->adpcm_len);
	ctx->http_post_head_len += ctx->adpcm_len;

	//发送数据
	uint64_t costtime1 = get_time_of_day();
	ret = sock_writen_with_timeout(ctx->sock, ctx->http_post_head, ctx->http_post_head_len, timeout_msec);
	uint64_t costtime2 = get_time_of_day();
	if (ret != ctx->http_post_head_len)
	{
		DEBUG_LOGE(TAG_ASR, "deep_brain_asr_write index[%d], len[%d], send_len[%d], cost[%llums]", 
			ctx->index, ctx->http_post_head_len, ret, costtime2 - costtime1);
		return DP_ASR_ERRNO_POOR_NETWORK;
	}
	else
	{
		DEBUG_LOGI(TAG_ASR, "deep_brain_asr_write index[%d], len[%d], send_len[%d], cost[%llums]", 
			ctx->index, ctx->http_post_head_len, ret, costtime2 - costtime1);
	}

	if(ctx->filep) 
	{
		fwrite(ctx->adpcm, 1, ctx->adpcm_len, ctx->filep);
		fflush(ctx->filep);
	}

	//接收数据
	if (ctx->index > 0) 
	{
		while(recv(ctx->sock, &drop, sizeof(drop), MSG_DONTWAIT) > 0);
	}
	
	return DP_ASR_ERRNO_OK;
}

DP_ASR_ERRNO_t deep_brain_asr_result(
	ASR_CONTEXT_T *ctx, 
	char* str_reply,	
	const size_t str_reply_len)
{	
	cJSON *pJson = NULL, *subJson = NULL;
	int64_t timeraw1;
	int64_t timeraw2;
	int loop_num = 0;
	int drop;
	char * find = NULL;
	DP_ASR_ERRNO_t err = DP_ASR_ERRNO_OK;

	if (!ctx || !str_reply) 
	{
		return DP_ASR_ERRNO_INVALID_PARAMS;
	}

	if (ctx->udid[0] == 0) 
	{
		crypto_generate_nonce_hex(ctx->udid, sizeof(ctx->udid));
		ctx->index = 0;
	}

	if(ctx->filep) 
	{
		fwrite(ctx->adpcm, 1, ctx->adpcm_len, ctx->filep);
		fflush(ctx->filep);
	}
	
	ctx->index *= -1;
	timeraw1 = get_time_of_day();
	err = deep_brain_asr_write(ctx, NULL, 0, 1000);
	timeraw2 = get_time_of_day();
	DEBUG_LOGI(TAG_ASR, "@line[%d] send last package audio cost time: %lld ms", __LINE__, timeraw2 - timeraw1);
	if (err != DP_ASR_ERRNO_OK)
	{
		DEBUG_LOGE(TAG_ASR, "send last package audio failed");
		return err;
	}
	
	drop = 0;
	loop_num = 20;
	timeraw1 = timeraw2;
	memset(ctx->http_post_head, 0, sizeof(ctx->http_post_head));
	while (loop_num--) 
	{
		int ret = sock_readn_with_timeout(ctx->sock, ctx->http_post_head, sizeof(ctx->http_post_head), 100);
		if (ret < 0) {
			break;
		}
		ctx->http_post_head[ret] = 0;
		if ((find = strstr(ctx->http_post_head + drop, "{"))) 
		{
			pJson = cJSON_Parse(find);
			if (pJson == NULL) 
			{
				int start_pos = ret;
				ret = sock_readn_with_timeout(ctx->sock, ctx->http_post_head + start_pos, sizeof(ctx->http_post_head) - start_pos, 2000);
				ctx->http_post_head[ret + start_pos] = 0;
				//DEBUG_LOGE(TAG_ASR, "@line[%d] %s", __LINE__, ctx->http_post_head);
				pJson = cJSON_Parse(find);
			}
			break;
		}
	}
	timeraw2 = get_time_of_day();
	DEBUG_LOGW(TAG_ASR, "@line[%d] recv asr result cost time:[%lld ms], recv len[%d]", 
		__LINE__, timeraw2- timeraw1, strlen(ctx->http_post_head));

	if (pJson != NULL)
	{
		snprintf(str_reply, str_reply_len, "%s", find);
	}
	else
	{
		err = DP_ASR_ERRNO_INVALID_JSON;
	}
		
	close(ctx->sock);
	ctx->sock = -1;
	ctx->udid[0] = 0;
	memset(ctx->adpcm, 0, sizeof(ctx->adpcm));
	ctx->adpcm_len = 0;
	if (pJson) 
	{
		cJSON_Delete(pJson);
	}
	
	return err;
}


DP_ASR_ERRNO_t deep_brain_asr_close(ASR_CONTEXT_T* ctx)
{
	int loop_num, drop, ret;
	if (!ctx || ctx->sock < 0)
	{
		return 0;
	}
	
	if (ctx->index > 0) 
	{
		ctx->index++;
		crypto_generate_nonce((unsigned char *)ctx->nonce, sizeof(ctx->nonce) - 1);
		crypto_time_stamp((unsigned char*)ctx->time_stamp, sizeof(ctx->time_stamp) - 1);
		crypto_generate_private_key((uint8_t *)ctx->private_key, sizeof(ctx->private_key), ctx->nonce, ctx->time_stamp, DEEP_BRAIN_ROBOT_ID);
		ctx->http_post_head_len = snprintf(ctx->http_post_head, sizeof(ctx->http_post_head), HTTP_POST_HEADER, 
									ctx->params, ctx->domain, ctx->port,
									CONNECTION_CLOSE, 0, ctx->time_stamp, 
									ctx->private_key, ctx->robot_id, ctx->nonce, 
									ctx->udid, 0, ctx->asr_params.lang, -ctx->index, X_NLP_SWITCH_YES,
									ctx->user_id, ctx->device_id, ctx->app_id, ctx->robot_id);
		
		//DEBUG_LOGE(TAG_ASR, "@line[%d] headerlen[%d], httpheader: %s", __LINE__, ctx->http_post_head_len, ctx->http_post_head);
		for(drop = loop_num = 0; loop_num < 4; loop_num++) 
		{
			ret = sock_writen_with_timeout(ctx->sock, ctx->http_post_head + drop, ctx->http_post_head_len - drop, 1000);
			if (ret > 0) 
			{
				drop += ret;
			}
			
			if (ctx->http_post_head_len <= drop) 
			{
				break;
			}
		}
	}
	
	ret = sock_close(ctx->sock);
	if (!ret) 
	{
		ctx->sock = -1;
		ctx->udid[0] = 0;
		ctx->index = 0;
		ctx->http_post_head_len = 0;
//		ctx->http_send_buf_len = 0;
		ctx->inpcm_len = 0;
		memset(ctx->http_post_head, 0, sizeof(ctx->http_post_head));
//		memset(ctx->http_send_buf, 0, sizeof(ctx->http_send_buf));
	}
	
	return ret;
}

DP_ASR_ERRNO_t deep_brain_asr_decode(
	const char *json_result,
	const char *asr_text,
	const size_t asr_text_len)
{	
	DP_ASR_ERRNO_t err = DP_ASR_ERRNO_OK;
	
	cJSON *pJson_body = cJSON_Parse(json_result);
	if (NULL != pJson_body) 
	{
		cJSON *pJson_head = cJSON_GetObjectItem(pJson_body, "responseHead");
		if (pJson_head == NULL)
		{
			err = DP_ASR_ERRNO_INVALID_JSON;
			goto deep_brain_asr_decode_error;
		}

		cJSON *pJson_asr_data = cJSON_GetObjectItem(pJson_body, "asrData");
		if (pJson_asr_data == NULL)
		{
			err = DP_ASR_ERRNO_INVALID_JSON;
			goto deep_brain_asr_decode_error;
		}
		
		cJSON *pJson_asr_ret = cJSON_GetObjectItem(pJson_asr_data, "Result");
		if (pJson_asr_ret != NULL && pJson_asr_ret->valuestring != NULL)
		{
			snprintf(asr_text, asr_text_len, "%s", pJson_asr_ret->valuestring);
		}		
	}
	else
	{
		DEBUG_LOGE(TAG_ASR, "deep_brain_asr_decode cJSON_Parse error,[%d][%s]", 
			strlen(json_result), json_result);
		goto deep_brain_asr_decode_error;
	}

	if (NULL != pJson_body)
	{
		cJSON_Delete(pJson_body);
	}
	
	return err;
	
deep_brain_asr_decode_error:
	
	if (NULL != pJson_body)
	{
		cJSON_Delete(pJson_body);
	}

	return err;
}


