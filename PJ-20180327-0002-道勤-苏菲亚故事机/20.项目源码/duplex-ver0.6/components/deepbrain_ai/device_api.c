
#include <string.h>
#include <sys/time.h>
#include <stdlib.h>
#include <unistd.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "crypto_api.h"
#include "mbedtls/sha1.h"
#include "mbedtls/md5.h"
#include "mbedtls/base64.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event_loop.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_eth.h"
#include "lwip/netdb.h"
#include "EspAudioAlloc.h"

void device_get_mac(unsigned char eth_mac[6])
{
	esp_wifi_get_mac(ESP_IF_WIFI_STA, eth_mac);
	
	return;
}

void device_get_mac_str(char *_out, size_t out_len)
{
	unsigned char eth_mac[6];
	
	esp_wifi_get_mac(ESP_IF_WIFI_STA, eth_mac);
	snprintf(_out, out_len, "%02X%02X%02X%02X%02X%02X", 
		eth_mac[0],eth_mac[1],eth_mac[2],eth_mac[3],eth_mac[4],eth_mac[5]);
	
	return;
}

uint64_t get_time_of_day(void)
{
	struct timeval tv;
	
	gettimeofday(&tv, NULL);
	
	return (tv.tv_sec*1000 + tv.tv_usec/1000);
}

void *esp32_malloc(size_t _size)
{
	return EspAudioAlloc(1, _size);
}

void esp32_free(void *_mem)
{
	if (_mem != NULL)
	{
		free(_mem);
	}
}

void device_sleep(int sec, int usec)
{
	struct timeval timeout;
	timeout.tv_sec = sec;
	timeout.tv_usec = usec;

	select(0, NULL, NULL, NULL, &timeout);
}

