#ifndef _TONE_OTA_SERVICE_H_
#define _TONE_OTA_SERVICE_H_
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "MediaService.h"

#define TONE_OTA_URL "http://192.168.3.3:8080/audio-esp.bin"
#define BUFFSIZE 1024
#define TEXT_BUFFSIZE 1024

#define ESP_ERR_AUDIO_TONEOTA_BASE        0x1900
#define TONE_OTA_PARTITION_NOT_FOUND      (ESP_ERR_AUDIO_TONEOTA_BASE + 0x01)
#define TONE_OTA_CONNECT_SERVER_FAIL      (ESP_ERR_AUDIO_TONEOTA_BASE + 0x02)
#define TONE_OTA_PARTITION_INVALID        (ESP_ERR_AUDIO_TONEOTA_BASE + 0x03)
#define TONE_OTA_SEND_HTTP_REQ_FAIL       (ESP_ERR_AUDIO_TONEOTA_BASE + 0x04)
#define TONE_OTA_RECV_PACKET_ERROR        (ESP_ERR_AUDIO_TONEOTA_BASE + 0x05)
#define TONE_OTA_UNEXCEPTED_RECV_ERROR    (ESP_ERR_AUDIO_TONEOTA_BASE + 0x06)
#define TONE_OTA_ENC_CHECKSUM_ERROR       (ESP_ERR_AUDIO_TONEOTA_BASE + 0x07)

#define OTA_RES_SUCCESS    (1<<0)
#define OTA_RES_PROGRESS   (1<<1)
#define OTA_RES_FAILED     (1<<2)

#define OTA_STATUS_INIT    0xFFFFFFFF
#define OTA_STATUS_IDLE    (1<<16)
#define OTA_STATUS_START   (1<<17)
#define OTA_STATUS_ERASE   (1<<18)
#define OTA_STATUS_WRITE   (1<<19)
#define OTA_STATUS_END     (1<<20)

typedef struct ToneOtaService { //extern from TreeUtility
    /*relation*/
    MediaService Based;
    /*private*/
    QueueHandle_t _ToneOtaQueue;
} ToneOtaService;

typedef struct ToneStatus_t {
    uint32_t index;
    uint32_t latest_flag;
    uint32_t tone_ota_status;
    uint32_t crc;
} ToneStatus;

ToneOtaService* ToneOtaServiceCreate(void);

esp_err_t esp_tone_ota_audio_begin(const char* url);
esp_err_t esp_tone_ota_audio_set_status(uint32_t status);
uint32_t esp_tone_ota_audio_get_status(void);
esp_err_t esp_tone_ota_audio_end(void);

#endif
