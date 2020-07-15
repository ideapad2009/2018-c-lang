#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "esp_system.h"
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"
#include "soc/sens_reg.h"

#include "debug_log.h"
#include "driver/touch_pad.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "keyboard_manage.h"

#include "cJSON.h"
#include "deepbrain_service.h"
#include "touchpad.h"
#include "DeviceCommon.h"
#include "MediaControl.h"
#include "MediaHal.h"
#include "toneuri.h"
#include "baidu_api.h"
#include "xfl2w_api.h"
#include "http_api.h"
#include "deepbrain_api.h"
#include "device_api.h"
#include "socket.h"
#include "lwip/netdb.h"
#include "EspAudio.h"
#include "userconfig.h"
#include "Recorder.h"
#include "asr_service.h"
#include "memo_service.h"
#include "interf_enc.h"
#include "unisound_api.h"
#include "sinovoice_api.h"

#define LOG_TAG "asr service" 

static ASR_SERVICE_HANDLE_T *g_asr_service_handle = NULL;

static void record_encode_create(
	ASR_SERVICE_HANDLE_T *asr_service_handle, ASR_RECORD_TYPE_T record_type, ASR_LANGUAGE_TYPE_T language_type)
{
	asr_service_handle->language_type = language_type;
	
	switch (record_type)
	{
		case ASR_RECORD_TYPE_AMRNB:
		{
			if (asr_service_handle->amrnb_encode == NULL)
			{
				asr_service_handle->amrnb_encode = Encoder_Interface_init(0);
				if (asr_service_handle->amrnb_encode == NULL)
				{
					DEBUG_LOGE(LOG_TAG, "Encoder_Interface_init failed"); 
				}
			}
			memset(&asr_service_handle->asr_result, 0, sizeof(asr_service_handle->asr_result));
			snprintf(asr_service_handle->asr_result.record_data, sizeof(asr_service_handle->asr_result.record_data), 
				"%s", AMRNB_HEADER);
			asr_service_handle->asr_result.record_len = strlen(AMRNB_HEADER);
			break;
		}
		case ASR_RECORD_TYPE_AMRWB:
		{
			if (asr_service_handle->amrwb_encode == NULL)
			{
				asr_service_handle->amrwb_encode = amrwb_encode_create(VOAMRWB_MD2385);
				if (asr_service_handle->amrwb_encode == NULL)
				{
					DEBUG_LOGE(LOG_TAG, "amrwb_encode_create failed"); 
				}
			}
			memset(&asr_service_handle->asr_result, 0, sizeof(asr_service_handle->asr_result));
			snprintf(asr_service_handle->asr_result.record_data, sizeof(asr_service_handle->asr_result.record_data), 
				"%s", VOAMRWB_RFC3267_HEADER_INFO);
			asr_service_handle->asr_result.record_len = strlen(VOAMRWB_RFC3267_HEADER_INFO);
			break;
		}
		case ASR_RECORD_TYPE_SPXNB:
		{
			if (!asr_service_handle->asr_ctx)
				asr_service_handle->asr_ctx = xfl2w_asr_ctx_create();
			if (!xfl2w_asr_encode_handle_valid(asr_service_handle->asr_ctx)) {
				xfl2w_asr_encode_handle_create(asr_service_handle->asr_ctx, ENC_BAND_WIDTH_NB, 10);
				DEBUG_LOGE(LOG_TAG, "\033[1;33m xfl2w_asr_encode_handle_create ENC_BAND_WIDTH_NB \033[0;0m"); 
			}
		}
		case ASR_RECORD_TYPE_SPXWB:
		{
			if (!asr_service_handle->asr_ctx)
				asr_service_handle->asr_ctx = xfl2w_asr_ctx_create();
			if (!xfl2w_asr_encode_handle_valid(asr_service_handle->asr_ctx)) {
				xfl2w_asr_encode_handle_create(asr_service_handle->asr_ctx, ENC_BAND_WIDTH_WB, 10);
				DEBUG_LOGE(LOG_TAG, "\033[1;33m xfl2w_asr_encode_handle_create ENC_BAND_WIDTH_WB \033[0;0m"); 
			}
			memset(&asr_service_handle->asr_result, 0, sizeof(asr_service_handle->asr_result));
			asr_service_handle->asr_result.record_len = 0;
			asr_service_handle->asr_result.record_ms= 0;
			break;
		}
		case ASR_RECORD_TYPE_PCM_UNISOUND_16K:
		{
			memset(&asr_service_handle->unisound_acount, 0, sizeof(asr_service_handle->unisound_acount));
			snprintf(asr_service_handle->unisound_acount.user_id, sizeof(asr_service_handle->unisound_acount.user_id), 
				"%s", UNISOUND_DEFAULT_USER_ID);
			snprintf(asr_service_handle->unisound_acount.appkey, sizeof(asr_service_handle->unisound_acount.appkey), 
				"%s", UNISOUND_DEFAULT_APP_KEY);
			get_flash_cfg(FLASH_CFG_DEVICE_ID, asr_service_handle->unisound_acount.device_id);
			
			if (asr_service_handle->unisound_asr_handle != NULL)
			{
				unisound_asr_delete(asr_service_handle->unisound_asr_handle);
				asr_service_handle->unisound_asr_handle = NULL;
			}
			
			asr_service_handle->unisound_asr_handle = unisound_asr_create(&asr_service_handle->unisound_acount);
			if (asr_service_handle->unisound_asr_handle == NULL)
			{	
				DEBUG_LOGE(LOG_TAG, "unisound_asr_create failed"); 
			}
			break;
		}
		case ASR_RECORD_TYPE_PCM_SINOVOICE_16K:
		{
			if (asr_service_handle->sinovoice_asr_handle != NULL)
			{
				sinovoice_asr_delete(asr_service_handle->sinovoice_asr_handle);
				asr_service_handle->sinovoice_asr_handle = NULL;
			}

			get_ai_acount(AI_ACOUNT_SINOVOICE, &asr_service_handle->sinovoice_acount);
			asr_service_handle->sinovoice_asr_handle = sinovoice_asr_create(&asr_service_handle->sinovoice_acount);
			if (asr_service_handle->sinovoice_asr_handle == NULL)
			{	
				DEBUG_LOGE(LOG_TAG, "sinovoice_asr_create failed"); 
			}
			break;
		}
		default:
			break;
	}
}

static void record_encode_free(ASR_SERVICE_HANDLE_T *asr_service_handle)
{
	if (asr_service_handle->amrnb_encode != NULL)
	{
		Encoder_Interface_exit(asr_service_handle->amrnb_encode);
		asr_service_handle->amrnb_encode = NULL;
	}

	if (asr_service_handle->amrwb_encode != NULL)
	{
		amrwb_encode_delete(asr_service_handle->amrwb_encode);
		asr_service_handle->amrwb_encode = NULL;
	}
//	if (xfl2w_asr_encode_handle_valid(asr_service_handle->asr_ctx)) {
//		xfl2w_asr_encode_handle_destory(asr_service_handle->asr_ctx);
//	}
}

static void record_encode_process(ASR_SERVICE_HANDLE_T *asr_service_handle, ASR_RECORD_DATA_T *p_record)
{
	int ret = 0;
	int i = 0;
	ASR_RESULT_T *p_asr_result = &asr_service_handle->asr_result;
	
	switch (p_record->record_type)
	{
		case ASR_RECORD_TYPE_AMRNB:
		{
			if (asr_service_handle->amrnb_encode == NULL)
			{
				break;
			}
			const int enc_frame_size = AMRNB_ENCODE_OUT_BUFF_SIZE;
			const int pcm_frame_size = AMRNB_ENCODE_IN_BUFF_SIZE;
			//降采样,16k 转 8k
			{
				short * src = (short *)p_record->record_data;
				short * dst = (short *)asr_service_handle->amrnb_buff.record_data_8k;
				int     samples = p_record->record_len/2;
				int idx;
				for (idx = 0; idx < samples; idx += 2) {
					dst[idx/2] = src[idx];
				}
				asr_service_handle->amrnb_buff.record_data_8k_len = p_record->record_len/2;
			}
			//pcm转amrnb
			for (i=0; (i * pcm_frame_size) < asr_service_handle->amrnb_buff.record_data_8k_len; i++)
			{
				if ((p_asr_result->record_len + enc_frame_size) > sizeof(p_asr_result->record_data))
				{
					break;
				}
				
				ret = Encoder_Interface_Encode(asr_service_handle->amrnb_encode, MR122, 
						(short*)(asr_service_handle->amrnb_buff.record_data_8k + i * pcm_frame_size), (unsigned char*)p_asr_result->record_data + p_asr_result->record_len, 0);
				if (ret > 0)
				{
					p_asr_result->record_len += ret;
					p_asr_result->record_ms += AMR_ENCODE_FRAME_MS;
				}
				else
				{
					ESP_LOGE(LOG_TAG, "Encoder_Interface_Encode failed[%d]", ret);
				}
			}
			break;
		}
		case ASR_RECORD_TYPE_AMRWB:
		{
			if (asr_service_handle->amrwb_encode == NULL)
			{
				break;
			}
			const int enc_frame_size = AMRWB_ENC_OUT_BUFFER_SZ;
			const int pcm_frame_size = AMRWB_ENC_IN_BUFFER_SZ;
			//pcm转amrwb
			for (i=0; (i*pcm_frame_size) < p_record->record_len; i++)
			{
				if ((p_asr_result->record_len + enc_frame_size) > sizeof(p_asr_result->record_data))
				{
					break;
				}

				ret = amrwb_encode_process(asr_service_handle->amrwb_encode, (unsigned char *)(p_record->record_data + i*pcm_frame_size), 
					pcm_frame_size, (unsigned char *)p_asr_result->record_data + p_asr_result->record_len);
				if (ret > 0)
				{
					p_asr_result->record_len += ret;
					p_asr_result->record_ms += AMR_ENCODE_FRAME_MS;
				}
				else
				{
					ESP_LOGE(LOG_TAG, "amrwb_encode_process failed[%d]", ret);
				}
			}
			break;
		}
		case ASR_RECORD_TYPE_SPXNB:
		case ASR_RECORD_TYPE_SPXWB:
		{
			char * record_data;
			int    record_len;
			int    enc_max_len;
			int    pcm_frame_samples;
			int    spx_frame_size;
			int    samples_per_ms;

			if (!xfl2w_asr_encode_handle_valid(asr_service_handle->asr_ctx)) {
				ESP_LOGE(LOG_TAG, "\033[1;33mxfl2w_asr_encode_handle_valid failed\033[0;0m");
				break;
			}
			record_data = p_record->record_data;
			record_len  = p_record->record_len;
			enc_max_len = sizeof(p_asr_result->record_data);
			pcm_frame_samples = xfl2w_asr_encode_handle_frame_samples(asr_service_handle->asr_ctx);
			spx_frame_size    = xfl2w_asr_encode_handle_spx_frame_size(asr_service_handle->asr_ctx);
			samples_per_ms = 16;
			if (p_record->record_type == ASR_RECORD_TYPE_SPXNB) {
				record_data = asr_service_handle->amrnb_buff.record_data_8k;
				samples_per_ms    = 8;
				//窄带 降采样,16k 转 8k
				{
					short * src = (short *)p_record->record_data;
					short * dst = (short *)asr_service_handle->amrnb_buff.record_data_8k;
					int     samples = p_record->record_len/2;
					int idx;
					for (idx = 0; idx < samples; idx += 2) {
						dst[idx/2] = src[idx];
					}
					record_len = p_record->record_len/2;
				}

			}
			
			for (i=0; i * pcm_frame_samples * 2 < record_len; i++) {
				if ((p_asr_result->record_len + spx_frame_size) > enc_max_len) {
					ESP_LOGE(LOG_TAG, "\033[1;33m record enc data space may be to less\033[0;0m");
					break;
				}
				ret = xfl2w_asr_encode(asr_service_handle->asr_ctx, 
										record_data + i*pcm_frame_samples*2, record_len - i * pcm_frame_samples * 2, 
										p_asr_result->record_data + p_asr_result->record_len, enc_max_len - p_asr_result->record_len);
				
				if (ret > 0) {
					spx_frame_size = ret;
					p_asr_result->record_len += spx_frame_size;
					p_asr_result->record_ms  += pcm_frame_samples / samples_per_ms;
//					ESP_LOGE(LOG_TAG, "spx_frame_size(%d)", spx_frame_size);
				} else {
					ESP_LOGE(LOG_TAG, "\033[1;33mxfl2w_asr_encode skip one frame[return %d]\033[0;0m", ret);
				}
			}
//			ESP_LOGE(LOG_TAG, "record_len(%d), pcm_frame_samples(%d), ", record_len, pcm_frame_samples);
			break;
		}
		case ASR_RECORD_TYPE_PCM_UNISOUND_16K:
		{
			if (unisound_asr_wirte(asr_service_handle->unisound_asr_handle, 
				p_record->record_data, p_record->record_len) != UNISOUND_ASR_ERRNO_OK)
			{
				DEBUG_LOGE(LOG_TAG, "unisound_asr_wirte failed");
				unisound_asr_delete(asr_service_handle->unisound_asr_handle);
				asr_service_handle->unisound_asr_handle = NULL;
			}
			break;
		}
		case ASR_RECORD_TYPE_PCM_SINOVOICE_16K:
		{
			if (sinovoice_asr_write(asr_service_handle->sinovoice_asr_handle, 
				p_record->record_data, p_record->record_len) != SINOVOICE_ASR_OK)
			{
				DEBUG_LOGE(LOG_TAG, "sinovoice_asr_write failed");
				sinovoice_asr_delete(asr_service_handle->unisound_asr_handle);
				asr_service_handle->unisound_asr_handle = NULL;
			}
			break;
		}
		default:
			break;
	}
}

static void record_asr_process(ASR_SERVICE_HANDLE_T *asr_service_handle, ASR_RECORD_MSG_T *p_msg)
{
	uint64_t start_time = get_time_of_day();
	ASR_RESULT_T *p_asr_result = &asr_service_handle->asr_result;
	memset(p_asr_result->asr_text, 0, sizeof(p_asr_result->asr_text));
	
	switch (p_msg->record_data.record_type)
	{
		case ASR_RECORD_TYPE_AMRNB:
		{
			BAIDU_ASR_MODE_T asr_mode = BAIDU_ASR_MODE_1536;
			get_ai_acount(AI_ACOUNT_BAIDU, &asr_service_handle->baidu_acount);
			memset(p_asr_result->asr_text, 0, sizeof(p_asr_result->asr_text));

			if (asr_service_handle->language_type == ASR_LANGUAGE_TYPE_CHINESE)
			{
				asr_mode = BAIDU_ASR_MODE_1536;
			}
			else if (asr_service_handle->language_type == ASR_LANGUAGE_TYPE_ENGLISH)
			{
				asr_mode = BAIDU_ASR_MODE_1737;
			}
			else
			{
				asr_mode = BAIDU_ASR_MODE_1536;
			}
			
			if (baidu_asr_amrnb(&asr_service_handle->baidu_acount, p_asr_result->asr_text, sizeof(p_asr_result->asr_text), 
				(unsigned char*)p_asr_result->record_data, p_asr_result->record_len, asr_mode) == BAIDU_ASR_FAIL)
			{
				ESP_LOGE(LOG_TAG, "baidu_asr_amrnb failed");
				asr_service_handle->asr_result.type = ASR_RESULT_TYPE_FAIL;
			}
			else
			{
				if (strlen(p_asr_result->asr_text) > 0)
				{
					asr_service_handle->asr_result.type = ASR_RESULT_TYPE_SUCCESS;
				}
				else
				{
					asr_service_handle->asr_result.type = ASR_RESULT_TYPE_NO_RESULT;
				}
			}
			break;
		}
		case ASR_RECORD_TYPE_AMRWB:
		{
			BAIDU_ASR_MODE_T asr_mode = BAIDU_ASR_MODE_1536;
			
			get_ai_acount(AI_ACOUNT_BAIDU, &asr_service_handle->baidu_acount);
			memset(p_asr_result->asr_text, 0, sizeof(p_asr_result->asr_text));

			if (asr_service_handle->language_type == ASR_LANGUAGE_TYPE_CHINESE)
			{
				asr_mode = BAIDU_ASR_MODE_1536;
			}
			else if (asr_service_handle->language_type == ASR_LANGUAGE_TYPE_ENGLISH)
			{
				asr_mode = BAIDU_ASR_MODE_1737;
			}
			else
			{
				asr_mode = BAIDU_ASR_MODE_1536;
			}
			
			if (baidu_asr_amrwb(&asr_service_handle->baidu_acount, p_asr_result->asr_text, sizeof(p_asr_result->asr_text), 
				(unsigned char*)p_asr_result->record_data, p_asr_result->record_len, asr_mode) == BAIDU_ASR_FAIL)
			{
				ESP_LOGE(LOG_TAG, "baidu_asr_amrwb failed");
				asr_service_handle->asr_result.type = ASR_RESULT_TYPE_FAIL;
			}
			else
			{
				if (strlen(p_asr_result->asr_text) > 0)
				{
					asr_service_handle->asr_result.type = ASR_RESULT_TYPE_SUCCESS;
				}
				else
				{
					asr_service_handle->asr_result.type = ASR_RESULT_TYPE_NO_RESULT;
				}
			}
			break;
		}
		case ASR_RECORD_TYPE_SPXNB:
		case ASR_RECORD_TYPE_SPXWB:
		{
			memset(p_asr_result->asr_text, 0, sizeof(p_asr_result->asr_text));
			if (xfl2w_asr(asr_service_handle->asr_ctx, 
				p_asr_result->asr_text, sizeof(p_asr_result->asr_text), 
				(unsigned char*)p_asr_result->record_data, p_asr_result->record_len) != XFL2W_ASR_SUCCESS) {
				ESP_LOGE(LOG_TAG, "xfl2w_asr_%s failed", p_msg->record_data.record_type == ASR_RECORD_TYPE_SPXNB ?"spxnb":"spxwb");
				asr_service_handle->asr_result.type = ASR_RESULT_TYPE_FAIL;
				break;
			}
			if (strlen(p_asr_result->asr_text) > 0) {
				asr_service_handle->asr_result.type = ASR_RESULT_TYPE_SUCCESS;
			} else {
				asr_service_handle->asr_result.type = ASR_RESULT_TYPE_NO_RESULT;
			}
			break;
		}
		case ASR_RECORD_TYPE_PCM_UNISOUND_16K:
		{
			if (unisound_asr_result(asr_service_handle->unisound_asr_handle, p_asr_result->asr_text, sizeof(p_asr_result->asr_text)) != UNISOUND_ASR_ERRNO_OK)
			{
				DEBUG_LOGE(LOG_TAG, "unisound_asr_result failed");
			}
			
			unisound_asr_delete(asr_service_handle->unisound_asr_handle);
			asr_service_handle->unisound_asr_handle = NULL;
			if (strlen(p_asr_result->asr_text) > 0) 
			{
				asr_service_handle->asr_result.type = ASR_RESULT_TYPE_SUCCESS;
			} 
			else 
			{
				asr_service_handle->asr_result.type = ASR_RESULT_TYPE_NO_RESULT;
			}
			break;
		}
		case ASR_RECORD_TYPE_PCM_SINOVOICE_16K:
		{
			if (sinovoice_asr_result(asr_service_handle->sinovoice_asr_handle, p_asr_result->asr_text, sizeof(p_asr_result->asr_text)) != SINOVOICE_ASR_OK)
			{
				DEBUG_LOGE(LOG_TAG, "sinovoice_asr_result failed");
			}
			
			sinovoice_asr_delete(asr_service_handle->sinovoice_asr_handle);
			asr_service_handle->sinovoice_asr_handle = NULL;
			if (strlen(p_asr_result->asr_text) > 0) 
			{
				asr_service_handle->asr_result.type = ASR_RESULT_TYPE_SUCCESS;
			} 
			else 
			{
				asr_service_handle->asr_result.type = ASR_RESULT_TYPE_NO_RESULT;
			}
			break;
		}
		default:
			break;
	}

	uint64_t asr_cost = get_time_of_day() - p_msg->record_data.time_stamp;
	DEBUG_LOGE(LOG_TAG, "asr result:mode[%d]:record_len[%d],cost[%lldms],result[%s]", 
		p_msg->record_data.record_type, p_asr_result->record_len, asr_cost, p_asr_result->asr_text);
	if (p_msg->call_back != NULL)
	{
		p_msg->call_back(&asr_service_handle->asr_result);
	}
}


#if DEBUG_RECORD_PCM

static FILE *pcm_record_open(void)
{
	FILE *p_file = NULL;
	static int count = 1;
	char str_record_path[32] = {0};
	snprintf(str_record_path, sizeof(str_record_path), "/sdcard/asr_%d.pcm", count++);
	p_file = fopen(str_record_path, "w+");
	if (p_file == NULL)
	{
		ESP_LOGE(LOG_TAG, "fopen %s failed", str_record_path);
	}
	
	return p_file;
}

static void pcm_record_write(FILE *p_file, char *pcm_data, size_t pcm_len)
{
	if (p_file != NULL)
	{
		fwrite(pcm_data, pcm_len, 1, p_file);
	}
}

static void pcm_record_close(FILE *p_file)
{
	if (p_file != NULL)
	{
		fclose(p_file);
	}
}

static void arm_record_save(char *amr_data, size_t arm_len)
{	
	static int amr_count = 1;
	FILE *p_record_file = NULL;
	char str_record_path[32] = {0};
	snprintf(str_record_path, sizeof(str_record_path), "/sdcard/asr_%d.amr", amr_count++);
	p_record_file = fopen(str_record_path, "w+");
	if (p_record_file != NULL)
	{
		fwrite(amr_data, arm_len, 1, p_record_file);
		fclose(p_record_file);
	}
}

#endif
void task_asr(void *pv)
{
//	int ret = 0;
//	int i = 0;
	ASR_SERVICE_HANDLE_T *asr_service_handle = (ASR_SERVICE_HANDLE_T *)pv;
	ASR_RECORD_MSG_T *p_msg = NULL;
//	void *amr_encode_handle = NULL;
#if DEBUG_RECORD_PCM
	FILE *p_pcm_file = NULL;
#endif
	
	while (1) 
	{		
		if (xQueueReceive(asr_service_handle->msg_queue, &p_msg, portMAX_DELAY) == pdPASS) 
		{			
			switch (p_msg->msg_type)
			{
				case ASR_SERVICE_RECORD_START:
				{
					//创建音频编码句柄
					record_encode_create(
						asr_service_handle, p_msg->record_data.record_type, p_msg->record_data.language_type);
#if DEBUG_RECORD_PCM
					p_pcm_file = pcm_record_open();			
#endif
				}
				case ASR_SERVICE_RECORD_READ:
				{
					if (p_msg->record_data.record_len <= 0)
					{
						break;
					}

#if DEBUG_RECORD_PCM
					pcm_record_write(p_pcm_file, p_msg->record_data.record_data, p_msg->record_data.record_len);
#endif
					//PCM 转其他音频格式
					record_encode_process(asr_service_handle, &p_msg->record_data);
					break;
				}
				case ASR_SERVICE_RECORD_STOP:
				{
#if DEBUG_RECORD_PCM
					pcm_record_close(p_pcm_file);
					p_pcm_file = NULL;
#endif
					record_encode_free(asr_service_handle);
					//语音小于1秒，不做语音识别
					if (p_msg->record_data.record_ms < ASR_MIN_AUDIO_MS)
					{
						if (p_msg->call_back != NULL)
						{
							asr_service_handle->asr_result.type = ASR_RESULT_TYPE_SHORT_AUDIO;
							p_msg->call_back(&asr_service_handle->asr_result);
						}
						break;
					}
					
#if	DEBUG_RECORD_PCM
					arm_record_save(asr_service_handle->asr_result.record_data, asr_service_handle->asr_result.record_len);					
#endif	
					//语音识别处理
					record_asr_process(asr_service_handle, p_msg);
					
					break;
				}
				default:
					break;
			}

			if (p_msg->msg_type == ASR_SERVICE_QUIT)
			{
				esp32_free(p_msg);
				p_msg = NULL;
				break;
			}
			else
			{
				esp32_free(p_msg);
				p_msg = NULL;
			}
		}
	}
	
	vTaskDelete(NULL);
}


static int asr_service_send_msg(ASR_RECORD_MSG_T *p_msg)
{
	if (g_asr_service_handle == NULL || g_asr_service_handle->msg_queue == NULL)
	{
		return ESP_FAIL;
	}

	if (xQueueSend(g_asr_service_handle->msg_queue, &p_msg, 0) != pdPASS)
	{
		ESP_LOGE(LOG_TAG, "nlp_service_send_msg xQueueSend failed");
		return ESP_FAIL;
	}
	else
	{
		return ESP_OK;		
	}
}

void asr_service_create(void)
{
	g_asr_service_handle = (ASR_SERVICE_HANDLE_T *)esp32_malloc(sizeof(ASR_SERVICE_HANDLE_T));
	if (g_asr_service_handle == NULL)
	{
		ESP_LOGE(LOG_TAG, "asr_service_create failed, out of memory");
		return;
	}
	memset((char*)g_asr_service_handle, 0, sizeof(ASR_SERVICE_HANDLE_T));
	g_asr_service_handle->msg_queue = xQueueCreate(20, sizeof(char *));

	
    if (xTaskCreate(task_asr,
                    "task_asr",
                    1024*11,
                    g_asr_service_handle,
                    4,
                    NULL) != pdPASS) {

        ESP_LOGE(LOG_TAG, "ERROR creating task_deepbrain_nlp task! Out of memory?");
    }
}

void asr_service_delete(void)
{
	//nlp_service_send_msg(NLP_SERVICE_MSG_TYPE_QUIT, NULL, NULL);
}

int asr_service_send_request(ASR_RECORD_MSG_T *msg)
{
	return asr_service_send_msg(msg);
}

