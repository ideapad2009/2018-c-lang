/* OTA_TAG audio example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event_loop.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "EspAudio.h"
#include "toneuri.h"
#include "device_api.h"
#include "socket.h"
#include "http_api.h"
#include "ota_update.h"
#include "cJSON.h"
#include "ota_service.h"
#include "userconfig.h"
#include "flash_config_manage.h"

static char *OTA_TAG = "OTA_UPDATE";
static char ota_write_data[1024] = {0};
static const char *GET_OTA_REQUEST_HEADER = "GET %s HTTP/1.0\r\n"
    "Host: %s\r\n"
    "User-Agent: esp-idf/1.0 esp32\r\n"
    "Content-Type: application/json; charset=utf-8\r\n\r\n";

static esp_err_t ota_update_audio_begin(esp_partition_t *_wifi_partition, esp_ota_handle_t *out_handle)
{
	esp_err_t ret = esp_ota_begin(_wifi_partition, 0x0, out_handle);
    if (ret != ESP_OK) 
	{
        ESP_LOGE(OTA_TAG, "Error: ota begin wifi failed,ret=%x", ret);
        return ESP_FAIL;
    }

    return ESP_OK;
}

static esp_err_t ota_update_audio_write(esp_ota_handle_t _handle, const void *_data, size_t _size)
{
	esp_err_t ret = esp_ota_write(_handle, _data, _size);
    if (ret != ESP_OK) 
	{
        ESP_LOGE(OTA_TAG, "Error: write wifi handle failed,ret=%x, size=%d", ret, _size);
        return ESP_FAIL;
    }

    return ESP_OK;
}

static esp_err_t ota_update_audio_end(esp_ota_handle_t _handle)
{
    if (esp_ota_end(_handle) != ESP_OK) 
	{
        ESP_LOGE(OTA_TAG, "Error: end wifi handle failed");
        return ESP_FAIL;
    }

    return ESP_OK;
}

static esp_err_t ota_update_audio_set_boot_partition(esp_partition_t *_wifi_partition)
{
    if (esp_ota_set_boot_partition(_wifi_partition) != ESP_OK) 
	{
        ESP_LOGE(OTA_TAG, "Error: set boot wifi  failed");
        return -1;
    }

    return ESP_OK;
}

static OTA_ERROR_NUM_T ota_update_init(esp_ota_handle_t *_out_handle, esp_partition_t *_out_wifi_partition)
{
    const esp_partition_t *esp_current_partition = esp_ota_get_boot_partition();
    if ((esp_current_partition == NULL) || (esp_current_partition->type != ESP_PARTITION_TYPE_APP)) 
	{
        ESP_LOGE(OTA_TAG, "Error esp_current_partition->type[%x] != ESP_PARTITION_TYPE_APP[%x]",
			esp_current_partition->type,
			ESP_PARTITION_TYPE_APP);
        return OTA_INIT_FAIL;
    }
    ESP_LOGI("esp_current_partition:", "current type[%x]", esp_current_partition->type);
    ESP_LOGI("esp_current_partition:", "current subtype[%x]", esp_current_partition->subtype);
    ESP_LOGI("esp_current_partition:", "current address:0x%x", esp_current_partition->address);
    ESP_LOGI("esp_current_partition:", "current size:0x%x", esp_current_partition->size);
    ESP_LOGI("esp_current_partition:", "current labe:%s", esp_current_partition->label);

    esp_partition_t find_wifi_partition;
    memset(_out_wifi_partition, 0, sizeof(esp_partition_t));
    memset(&find_wifi_partition, 0, sizeof(esp_partition_t));
    /*choose which OTA_TAG audio image should we write to*/
    switch (esp_current_partition->subtype) {
    case ESP_PARTITION_SUBTYPE_APP_OTA_0:
        find_wifi_partition.subtype = ESP_PARTITION_SUBTYPE_APP_OTA_1;
        break;
    case  ESP_PARTITION_SUBTYPE_APP_OTA_1:
        find_wifi_partition.subtype = ESP_PARTITION_SUBTYPE_APP_OTA_0;
        break;
    default:
		find_wifi_partition.subtype = ESP_PARTITION_SUBTYPE_APP_OTA_0;
        break;
    }
    find_wifi_partition.type = ESP_PARTITION_TYPE_APP;
	ESP_LOGI("find_wifi_partition", "%d: subtype[%x]", __LINE__, find_wifi_partition.subtype);
    const esp_partition_t *wifi_partition = NULL;
    wifi_partition = esp_partition_find_first(find_wifi_partition.type, find_wifi_partition.subtype, NULL);
    assert(wifi_partition != NULL);
    memcpy(_out_wifi_partition, wifi_partition, sizeof(esp_partition_t));
    /*actual : we would not assign size for it would assign by default*/
    ESP_LOGI("wifi_partition", "%d: type[%x]", __LINE__, wifi_partition->type);
    ESP_LOGI("wifi_partition", "%d: subtype[%x]", __LINE__, wifi_partition->subtype);
    ESP_LOGI("wifi_partition", "%d: address:0x%x", __LINE__, wifi_partition->address);
    ESP_LOGI("wifi_partition", "%d: size:0x%x", __LINE__, wifi_partition->size);
    ESP_LOGI("wifi_partition", "%d: labe:%s", __LINE__,  wifi_partition->label);
	
    if (ota_update_audio_begin(_out_wifi_partition, _out_handle) != ESP_OK) 
	{
        ESP_LOGE(OTA_TAG, "Error: esp_ota_begin failed!");
        return OTA_INIT_FAIL;
    } 
	else 
    {
        ESP_LOGI(OTA_TAG, "Info: esp_ota_begin init OK!");
        return OTA_SUCCESS;
    }

    return OTA_INIT_FAIL;
}

static OTA_ERROR_NUM_T ota_update_data_from_sd(
	const char *_firmware_path,
	esp_ota_handle_t _sd_ota_handle,
	size_t *_out_firmware_lenght)
{
	OTA_ERROR_NUM_T ret = OTA_SD_FAIL;
	FILE *fp = NULL;
    fp = fopen(_firmware_path, "r");
    if (NULL == fp)
    {
    	ESP_LOGE(OTA_TAG, "open %s failed!", _firmware_path);
        return OTA_NO_SD_CARD;
    }
	ESP_LOGI(OTA_TAG, "open %s success", _firmware_path);
	
	int b_first_read = 1;
    while(1) 
	{
        memset(ota_write_data, 0, sizeof(ota_write_data));
		int buff_len = fread(ota_write_data, sizeof(char), sizeof(ota_write_data), fp);
		if (buff_len > 0) 
		{
			if (b_first_read == 1)
			{
				if (ota_write_data[0] != 0xE9 || buff_len < 2)
				{
					goto ota_update_data_from_sd_error;
				}
				b_first_read = 0;
			}
			
            if (ota_update_audio_write(_sd_ota_handle, (const void *)ota_write_data, buff_len) != ESP_OK) 
			{
                ESP_LOGE(OTA_TAG, "Error: ota_update_audio_write firmware failed!");
                goto ota_update_data_from_sd_error;
            }
            *_out_firmware_lenght += buff_len;
            ESP_LOGI(OTA_TAG, "Info: had written image length %d,%x", *_out_firmware_lenght, *_out_firmware_lenght);
        } 
		else if (buff_len == 0) 
       	{  /*read over*/
       		ret = OTA_SUCCESS;
            ESP_LOGI(OTA_TAG, "Info: receive all packet over!");
			break;
        } 
		else 
		{
            ESP_LOGI(OTA_TAG, "Warning: uncontolled event!");
            goto ota_update_data_from_sd_error;
        }
    }
	
ota_update_data_from_sd_error:
	if (fp != NULL)
	{
		fclose(fp);
		fp = NULL;
	}
	
    return ret;
}

static void str_to_lower(char *_src, char *_out_dest, size_t _len)
{
	int i = 0;

	if (_src == NULL || _out_dest == NULL || _len == 0)
	{
		return;
	}

	memset(_out_dest, 0, _len);
	for (i=0; i<_len - 1 && *(_src+i) != '\0'; i++)
	{
		*(_out_dest+i) = tolower((int)*(_src+i));
	}

	return;
}

static OTA_ERROR_NUM_T ota_update_data_from_server(
	const char *_server_url, 
	const esp_ota_handle_t _handle,
	size_t *_out_firmware_len)
{
	char	domain[64]	= {0};
	char	port[6] 	= {0};
	char	params[128] = {0};
	char	header[256] = {0};
	char	str_buf[1024]= {0};
	sock_t	sock		= INVALID_SOCK;
	int 	total_len 	= 0;
	
	if (sock_get_server_info(_server_url, (char*)&domain, (char*)&port, (char*)&params) != 0)
	{
		ESP_LOGE(OTA_TAG, "sock_get_server_info fail");
		goto ota_data_from_server_error;
	}

	sock = sock_connect(domain, port);
	if (sock == INVALID_SOCK)
	{
		ESP_LOGE(OTA_TAG, "sock_connect fail");
		goto ota_data_from_server_error;
	}

	sock_set_nonblocking(sock);

	snprintf(header, sizeof(header), GET_OTA_REQUEST_HEADER, params, domain);
	if (sock_writen_with_timeout(sock, header, strlen(header), 1000) != strlen(header)) 
	{
		ESP_LOGE(OTA_TAG, "sock_write http header fail");
		goto ota_data_from_server_error;
	}
	ESP_LOGE(OTA_TAG, "header=%s\n", header);
	
    bool pkg_body_start = false;
    /*deal with all receive packet*/
    while (1) 
	{
		vTaskDelay(5);//避免完全占用cpu，导致提示音无法播报
        memset(str_buf, 0, sizeof(str_buf));
        int buff_len = sock_readn_with_timeout(sock, str_buf, sizeof(str_buf) - 1, 5000);
        if (buff_len < 0 && (errno == EINTR || errno == EAGAIN))
		{
			ESP_LOGE(OTA_TAG, "recv total len: %d bytes", *_out_firmware_len);
			if (total_len == *_out_firmware_len)
			{
				break;
			}
			
			continue;
		}
		else if (buff_len < 0)
		{ /*receive error*/
            ESP_LOGE(OTA_TAG, "Error: receive data error!");
            goto ota_data_from_server_error;
        } 
		else if (buff_len > 0 && !pkg_body_start) 
		{ /*deal with packet header*/
			if (http_get_error_code(str_buf) != 200)
			{
				ESP_LOGE(OTA_TAG, "http reply=%s", str_buf);
				goto ota_data_from_server_error;
			}

			total_len = http_get_content_length(str_buf);
			if (total_len <= 0)
			{
				ESP_LOGE(OTA_TAG, "http reply=%s", str_buf);
				goto ota_data_from_server_error;
			}
			ESP_LOGE(OTA_TAG, "ota_update_data_from_server firmware total len = %d\r\n", total_len);

			char* pBody = http_get_body(str_buf);
			if (NULL == pBody)
			{
				ESP_LOGE(OTA_TAG, "http reply=%s", str_buf);
				goto ota_data_from_server_error;
			}
			buff_len -= pBody - str_buf;
			
			//check firmware head byte
			if (*pBody != 0xE9 || buff_len < 2)
			{
				goto ota_data_from_server_error;
			}

			if (ota_update_audio_write(_handle, (const void *)pBody, buff_len) != ESP_OK) 
			{
                ESP_LOGE(OTA_TAG, "Error: ota_update_audio_write firmware failed!");
                goto ota_data_from_server_error;
            }
			*_out_firmware_len = buff_len;
            pkg_body_start = true;
        } 
		else if (buff_len > 0 && pkg_body_start) 
		{ /*deal with packet body*/
            if (ota_update_audio_write(_handle, (const void *)str_buf, buff_len) != ESP_OK) 
			{
                ESP_LOGE(OTA_TAG, "Error: ota_update_audio_write firmware failed!");
                goto ota_data_from_server_error;
            }
            *_out_firmware_len += buff_len;
        } 
		else if (buff_len == 0) 
		{  /*packet over*/
			if (total_len == *_out_firmware_len)
			{
				break;
			}
			else
			{
				goto ota_data_from_server_error;
			}
        } 
		else 
		{
            goto ota_data_from_server_error;
        }
    }
	
	ESP_LOGI(OTA_TAG, "Info: total len=%d, had written image length %d", total_len, *_out_firmware_len);
	sock_close(sock);
	return OTA_SUCCESS;
	
ota_data_from_server_error:
	
	ESP_LOGE(OTA_TAG, "Info: total len=%d, had written image length %d", total_len, *_out_firmware_len);
	sock_close(sock);
	return OTA_WIFI_FAIL;
}


OTA_ERROR_NUM_T ota_update_from_sd(const char *_firmware_path)
{
	size_t firmware_len = 0;
	esp_ota_handle_t sd_ota_handle;
	esp_partition_t wifi_partition;
    memset(&wifi_partition, 0, sizeof(wifi_partition));

	if (_firmware_path == NULL || strlen(_firmware_path) <= 10)
	{
		return OTA_SD_FAIL;
	}

    /*begin ota*/
    if (ota_update_init(&sd_ota_handle, &wifi_partition) == OTA_SUCCESS) 
	{
        ESP_LOGI(OTA_TAG, "Info: OTA Init success!");
    } 
	else 
   	{
        ESP_LOGE(OTA_TAG, "Error: OTA Init failed");		
        return OTA_SD_FAIL;
    }
	
    /*if wifi ota exist,would ota it*/
	if (ota_update_data_from_sd(_firmware_path, sd_ota_handle, &firmware_len) != OTA_SUCCESS)
	{
		ESP_LOGI(OTA_TAG, "sd_ota_data_from_sd failed!");		
		return OTA_SD_FAIL;
	}
	ESP_LOGI(OTA_TAG, "Info: Total Write wifi binary data length : %d", firmware_len);

	/*ota end*/
    if (ota_update_audio_end(sd_ota_handle) != ESP_OK) 
	{
        ESP_LOGE(OTA_TAG, "Error : esp_ota_end failed!");		
        return OTA_SD_FAIL;
    }
    /*set boot*/
    if (ota_update_audio_set_boot_partition(&wifi_partition) != ESP_OK) 
	{
        ESP_LOGE(OTA_TAG, "Error : esp_ota_set_boot_partition failed!");
        return OTA_SD_FAIL;
    }
    ESP_LOGI(OTA_TAG, "Prepare to restart system!");

    return OTA_SUCCESS;
}

OTA_ERROR_NUM_T ota_update_from_wifi(const char *_firmware_url)
{
	size_t firmware_len = 0;
	esp_ota_handle_t sd_ota_handle;
	esp_partition_t wifi_partition;
    memset(&wifi_partition, 0, sizeof(wifi_partition));

	if (_firmware_url == NULL || strlen(_firmware_url) <= 10)
	{
		return OTA_WIFI_FAIL;
	}

    /*begin ota*/
    if (ota_update_init(&sd_ota_handle, &wifi_partition) == OTA_SUCCESS)
	{
        ESP_LOGI(OTA_TAG, "Info: OTA Init success!");
    } 
	else 
   	{
        ESP_LOGE(OTA_TAG, "Error: OTA Init failed");
        return OTA_WIFI_FAIL;
    }
	
    /*if wifi ota exist,would ota it*/
	if (ota_update_data_from_server(_firmware_url, sd_ota_handle, &firmware_len) != OTA_SUCCESS)
	{
		ESP_LOGI(OTA_TAG, "ota_data_from_server failed!");
		return OTA_WIFI_FAIL;
	}
	ESP_LOGI(OTA_TAG, "Info: Total Write wifi binary data length : %d", firmware_len);

	/*ota end*/
    if (ota_update_audio_end(sd_ota_handle) != ESP_OK) 
	{
        ESP_LOGE(OTA_TAG, "Error : esp_ota_end failed!");
        return OTA_WIFI_FAIL;
    }

	/*set boot*/
    if (ota_update_audio_set_boot_partition(&wifi_partition) != ESP_OK) 
	{
        ESP_LOGE(OTA_TAG, "Error : esp_ota_set_boot_partition failed!");
        return OTA_WIFI_FAIL;
    }

    return OTA_SUCCESS;
}

OTA_ERROR_NUM_T ota_update_check_fw_version(
	const char *_server_url,
	char *_out_fw_url,
	size_t _out_len)
{
	char	domain[64]	= {0};
	char	port[6] 	= {0};
	char	params[128] = {0};
	char	str_buf[1024]= {0};
	char	str_buf_1[128] = {0};
	char	str_buf_2[128] = {0};
	cJSON   *pJson_body = NULL;
	sock_t	sock		= INVALID_SOCK;
	char device_sn[32]= {0};
	
	get_flash_cfg(FLASH_CFG_DEVICE_ID, device_sn);
	if (strlen(device_sn) <= 0)
	{
		ESP_LOGE(OTA_TAG, "device sn is empty, please activate device first");
		goto ota_update_check_fw_version_error;
	}
	if (sock_get_server_info(_server_url, (char*)&domain, (char*)&port, (char*)&params) != 0)
	{
		ESP_LOGE(OTA_TAG, "sock_get_server_info fail");
		goto ota_update_check_fw_version_error;
	}

	sock = sock_connect(domain, port);
	if (sock == INVALID_SOCK)
	{
		ESP_LOGE(OTA_TAG, "sock_connect fail");
		goto ota_update_check_fw_version_error;
	}
	sock_set_nonblocking(sock);

	snprintf(str_buf, sizeof(str_buf), GET_OTA_REQUEST_HEADER, params, domain);
	if (sock_writen_with_timeout(sock, str_buf, strlen(str_buf), 2000) != strlen(str_buf)) 
	{
		ESP_LOGE(OTA_TAG, "sock_write http header fail");
		goto ota_update_check_fw_version_error;
	}
	ESP_LOGE(OTA_TAG, "header=%s\n", str_buf);
	
	memset(str_buf, 0, sizeof(str_buf));
	sock_readn_with_timeout(sock, str_buf, sizeof(str_buf) - 1, 5000);
	sock_close(sock);
	ESP_LOGE(OTA_TAG, "reply=%s\n", str_buf);
	if (http_get_error_code(str_buf) != 200)
	{
		ESP_LOGE(OTA_TAG, "%s", str_buf);
		goto ota_update_check_fw_version_error;
	}
	else
	{
		char* str_body = http_get_body(str_buf);
		//ESP_LOGE(OTA_TAG, "str_body is %s", str_reply);
		if (str_body == NULL)
		{
			goto ota_update_check_fw_version_error;
		}
	
		pJson_body = cJSON_Parse(str_body);
	    if (NULL != pJson_body) 
		{
	        cJSON *pJson_version = cJSON_GetObjectItem(pJson_body, "version");
			if (NULL == pJson_version || pJson_version->valuestring == NULL)
			{
				goto ota_update_check_fw_version_error;
			}

			str_to_lower(pJson_version->valuestring, str_buf_1, sizeof(str_buf_1));
			str_to_lower(ESP32_FW_VERSION, str_buf_2, sizeof(str_buf_2));
			ESP_LOGE(OTA_TAG, "server version=%s", str_buf_1);
			ESP_LOGE(OTA_TAG, "client version=%s", str_buf_2);
			if (strcmp(str_buf_1, str_buf_2) == 0)
			{
				goto ota_update_check_fw_version_error;
			}

			cJSON *pJson_start_sn = cJSON_GetObjectItem(pJson_body, "start_sn");
			if (NULL == pJson_start_sn || pJson_start_sn->valuestring == NULL)
			{
				goto ota_update_check_fw_version_error;
			}
			
			str_to_lower(pJson_start_sn->valuestring, str_buf_1, sizeof(str_buf_1));
			str_to_lower(device_sn, str_buf_2, sizeof(str_buf_2));
			ESP_LOGE(OTA_TAG, "server sn=%s", str_buf_1);
			ESP_LOGE(OTA_TAG, "client sn=%s", str_buf_2);
			if (strlen(str_buf_1) != DEVICE_SN_MAX_LEN || strlen(str_buf_2) != DEVICE_SN_MAX_LEN)
			{
				ESP_LOGE(OTA_TAG, "sn len is not %d", DEVICE_SN_MAX_LEN);
				goto ota_update_check_fw_version_error;
			}
			
			if (strcmp(str_buf_1 + 10, str_buf_2 + 10) > 0)
			{
				ESP_LOGE(OTA_TAG, "%s > %s", str_buf_1, str_buf_2);
				goto ota_update_check_fw_version_error;
			}

			cJSON *pJson_end_sn = cJSON_GetObjectItem(pJson_body, "end_sn");
			if (NULL == pJson_end_sn || pJson_end_sn->valuestring == NULL)
			{
				goto ota_update_check_fw_version_error;
			}

			str_to_lower(pJson_end_sn->valuestring, str_buf_1, sizeof(str_buf_1));
			ESP_LOGE(OTA_TAG, "server sn=%s", str_buf_1);
			ESP_LOGE(OTA_TAG, "client sn=%s", str_buf_2);

			if (strlen(str_buf_1) != DEVICE_SN_MAX_LEN || strlen(str_buf_2) != DEVICE_SN_MAX_LEN)
			{
				ESP_LOGE(OTA_TAG, "sn len is not %d", DEVICE_SN_MAX_LEN);
				goto ota_update_check_fw_version_error;
			}
			
			if (strcmp(str_buf_1 + 10, str_buf_2 + 10) < 0)
			{
				ESP_LOGE(OTA_TAG, "%s < %s", str_buf_1, str_buf_2);
				goto ota_update_check_fw_version_error;
			}
			
			cJSON *pJson_download_url = cJSON_GetObjectItem(pJson_body, "download_url");
			if (NULL == pJson_download_url || pJson_download_url->valuestring == NULL)
			{
				goto ota_update_check_fw_version_error;
			}
			
			snprintf(_out_fw_url, _out_len, "%s", pJson_download_url->valuestring);
			
			if (NULL != pJson_body)
			{
				cJSON_Delete(pJson_body);
			}
	    }
		else
		{
			goto ota_update_check_fw_version_error;
		}
	}

	if (sock != INVALID_SOCK)
	{
		sock_close(sock);
	}

	return OTA_NEED_UPDATE_FW;
	
ota_update_check_fw_version_error:
	
	if (sock != INVALID_SOCK)
	{
		sock_close(sock);
	}

	if (NULL != pJson_body)
	{
		cJSON_Delete(pJson_body);
	}
	
	return OTA_NONEED_UPDATE_FW;
}


