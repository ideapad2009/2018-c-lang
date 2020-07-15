
#include <string.h>
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
#include "device_api.h"

void crypto_sha1(const unsigned char *input, const size_t ilen, unsigned char output[20])
{
	return mbedtls_sha1(input, ilen, output);	
}

void crypto_md5(const unsigned char *input, const size_t ilen, unsigned char output[16])
{
	return mbedtls_md5(input, ilen, output);
}

int crypto_base64_encode(unsigned char *dst, size_t dlen, size_t *olen, const unsigned char *src, size_t slen)
{
	return mbedtls_base64_encode(dst, dlen, olen, src, slen);
}

int crypto_base64_decode(unsigned char *dst, size_t dlen, size_t *olen, const unsigned char *src, size_t slen)
{
	return mbedtls_base64_decode(dst, dlen, olen, src, slen);
}

void crypto_rand_str(char *strRand, const size_t iLen)
{
	int i = 0;
	struct timeval tv;
	char metachar[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
	
	gettimeofday(&tv, NULL);
	srand(tv.tv_sec*1000 + tv.tv_usec/1000);
	memset(strRand, 0, iLen);
	for (i = 0; i < (iLen - 1); i++)
	{
		*(strRand+i) = metachar[rand() % 62];
	}

	return;
}

void crypto_sha1prng(unsigned char sha1prng[16], const unsigned char *strMac)
{
	unsigned char strSeedFull[64] = {0};
	unsigned char strSeed[16+1] = {0};
	unsigned char digest1[20];
	unsigned char digest2[20];

	//生成16位随机数种子
	crypto_rand_str((char*)strSeed, sizeof(strSeed));
	snprintf((char*)strSeedFull, sizeof(strSeedFull), "%s%s", strMac, strSeed);

	//第一次sha1加密
	crypto_sha1(strSeedFull, strlen((char*)strSeedFull), digest1); 

	//第二次sha1加密
	crypto_sha1(digest1, sizeof(digest1), digest2); 
	
	memcpy((char*)sha1prng, (char*)digest2, 16);

	return;
}

void crypto_time_stamp(unsigned char* strTimeStamp, const size_t iLen)
{
	time_t tNow =time(NULL);   
    struct tm tmNow = { 0 };  
	
    localtime_r(&tNow, &tmNow);//localtime_r为可重入函数，不能使用localtime  
    snprintf((char *)strTimeStamp, iLen, "%04d-%02d-%02dT%02d:%02d:%02d", 
		tmNow.tm_year + 1900, tmNow.tm_mon + 1, tmNow.tm_mday,
		tmNow.tm_hour, tmNow.tm_min, tmNow.tm_sec); 
	
	return strTimeStamp;
}

void crypto_random_byte(unsigned char _byte[16])
{
	unsigned char eth_mac[6] = {0};
	device_get_mac(eth_mac);

	crypto_sha1prng(_byte, eth_mac);

	return;
}

void crypto_generate_request_id(char *_out, size_t _out_len)
{
	unsigned char byte[16] = {0};
	unsigned char eth_mac[6] = {0};
	
	device_get_mac(eth_mac);
	crypto_sha1prng(byte, eth_mac);

	snprintf(_out, _out_len, "%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x",
		byte[0], byte[1], byte[2], byte[3], byte[4], byte[5], byte[6], byte[7], byte[8], byte[9], 
		byte[10], byte[11], byte[12], byte[13], byte[14], byte[15]);

	return;
}

void crypto_generate_nonce(unsigned char* strNonce, const size_t iStrLen)
{
	int 			i			= 0;
	size_t 			len 		= 0;
	uint8_t 		eth_mac[6]	= {0};
	unsigned char 	strSha1[16] = {0};
	unsigned char 	str_buf[3] 	= {0};
	unsigned char 	str_mac[13] = {0};

	memset(eth_mac, 0, sizeof(eth_mac));
    device_get_mac(eth_mac);
	for (i=0; i < sizeof(eth_mac); i++)
	{
		snprintf((char*)str_buf, sizeof(str_buf), "%02X", eth_mac[i]);
		strcat((char*)str_mac, (char*)str_buf);
	}
	crypto_sha1prng(strSha1, str_mac);
	memset(strNonce, 0, iStrLen);
	crypto_base64_encode(strNonce, iStrLen, &len, strSha1, sizeof(strSha1));

	return;
}

void crypto_generate_nonce_hex(unsigned char* strNonce, const size_t iStrLen)
{
	int 			i			= 0;
	size_t 			len 		= 0;
	uint8_t 		eth_mac[6]	= {0};
	unsigned char 	strSha1[16] = {0};
	unsigned char 	str_buf[3] 	= {0};
	unsigned char 	str_mac[13] = {0};

	memset(eth_mac, 0, sizeof(eth_mac));
    device_get_mac(eth_mac);
	for (i=0; i < sizeof(eth_mac); i++)
	{
		snprintf((char*)str_buf, sizeof(str_buf), "%02X", eth_mac[i]);
		strcat((char*)str_mac, (char*)str_buf);
	}
	crypto_sha1prng(strSha1, str_mac);

	memset(strNonce, 0, iStrLen);
	snprintf((char*)strNonce, iStrLen, "%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X", 
		strSha1[0],
		strSha1[1],strSha1[2],strSha1[3],strSha1[4],strSha1[5],
		strSha1[6],strSha1[7],strSha1[8],strSha1[9],strSha1[10],
		strSha1[11],strSha1[12],strSha1[13],strSha1[14],strSha1[15]);	
	
	return;
}

void crypto_generate_private_key(
	unsigned char* strPrivateKey, const size_t iStrLen, 
	const char* strNonce, const char* strTimeStamp, const char* strRobotID)
{
	size_t 			len 		= 0;
	char 			strBuf[256] = {0};
	unsigned char 	digest[20] 	= {0};

	snprintf(strBuf, sizeof(strBuf), "%s%s%s", strNonce, strTimeStamp, strRobotID); 
	crypto_sha1((unsigned char*)strBuf, strlen(strBuf), (unsigned char*)&digest); 

	crypto_base64_encode(strPrivateKey, iStrLen, &len, digest, sizeof(digest));

	return;
}




