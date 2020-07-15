#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "rom/crc.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event_loop.h"
#include "esp_log.h"

#include "nvs_flash.h"
#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include "lwip/netdb.h"
#include "lwip/dns.h"
#include "ToneOtaService.h"
#include "UriParser.h"
#include "esp_ota_ops.h"
#include "esp_partition.h"

#define TONE_OTA_TAG  "TONE_OTA"

/*an ota data write buffer ready to write to the flash*/
static char ota_write_data[BUFFSIZE + 1] = { 0 };
/*an packet receive buffer*/
static char text[BUFFSIZE + 1] = { 0 };
/* an image total length*/
static int binary_file_length = 0;
/*socket id*/
QueueHandle_t toneotaqueue = NULL;
static int socket_id = -1;
static char ota_host[64];
static char ota_path[64];
static uint16_t ota_port = 80;;

static void task_fatal_error(esp_err_t* ret)
{
    ESP_LOGE(TONE_OTA_TAG, "Exiting task due to fatal error...");

    if (socket_id != -1) {
        close(socket_id);
    }

    xQueueSend(toneotaqueue, ret, 0);
    vTaskDelete(NULL);
}

/*read buffer by byte still delim ,return read bytes counts*/
static int read_until(char* buffer, char delim, int len)
{
//  /*TODO: delim check,buffer check,further: do an buffer length limited*/
    int i = 0;

    while (buffer[i] != delim && i < len) {
        ++i;
    }

    return i + 1;
}

static int _resolevDns(const char* host, struct sockaddr_in* ip)
{
    struct hostent* he;
    struct in_addr** addr_list;
    he = gethostbyname(host);

    if (he == NULL) {
        return 0;
    }

    addr_list = (struct in_addr**)he->h_addr_list;

    if (addr_list[0] == NULL) {
        return 0;
    }

    ip->sin_family = AF_INET;
    memcpy(&ip->sin_addr, addr_list[0], sizeof(ip->sin_addr));
    return 1;
}

/* resolve a packet from http socket
 * return true if packet including \r\n\r\n that means http packet header finished,start to receive packet body
 * otherwise return false
 * */
static bool read_past_http_header(char text[], int total_len, const esp_partition_t* partition)
{
    /* i means current position */
    int i = 0, i_read_len = 0;

    while (text[i] != 0 && i < total_len) {
        i_read_len = read_until(&text[i], '\n', total_len);

        // if we resolve \r\n line,we think packet header is finished
        if (i_read_len == 2) {
            int i_write_len = total_len - (i + 2);
            memset(ota_write_data, 0, BUFFSIZE);
            /*copy first http packet body to write buffer*/
            memcpy(ota_write_data, &(text[i + 2]), i_write_len);

            esp_err_t err = esp_partition_write(partition, binary_file_length, (const void*)ota_write_data, i_write_len);

            if (err != ESP_OK) {
                ESP_LOGE(TONE_OTA_TAG, "Error: esp_ota_write failed! err=0x%x", err);
                return false;
            } else {
                ESP_LOGI(TONE_OTA_TAG, "esp_ota_write header OK");
                binary_file_length += i_write_len;
            }

            return true;
        }

        i += i_read_len;
    }

    return false;
}

static esp_err_t connect_to_http_server(const char* url)
{
    ESP_LOGI(TONE_OTA_TAG, "Server url %s", TONE_OTA_URL);
    esp_err_t ret;
    struct sockaddr_in sock_info;
    ParsedUri* puri;

    socket_id = socket(AF_INET, SOCK_STREAM, 0);

    if (socket_id == -1) {
        ESP_LOGE(TONE_OTA_TAG, "Error: create socket failed!");
        return ESP_FAIL;
    } else {
        ESP_LOGI(TONE_OTA_TAG, "Info: create socket success!");
    }

    puri = ParseUri(url);

    if (puri != NULL) {
        if (puri->port) {
            ota_port = atoi(puri->port);
        }

        if (!(puri->host && puri->path)) {
            ESP_LOGE(TONE_OTA_TAG, "http host name or path is error");
            return ESP_FAIL;
        }

        memcpy(ota_host, puri->host, strlen(puri->host));
        memcpy(ota_path, puri->path, strlen(puri->path));
    }

    if (0 == _resolevDns(puri->host, &sock_info)) {
        ESP_LOGE(TONE_OTA_TAG, "_resolev host %s dns failed!", ota_host);
        return ESP_FAIL;
    }

    FreeParsedUri(puri);
    sock_info.sin_port = htons(ota_port);
    ESP_LOGI(TONE_OTA_TAG, "\r\nconnecting to server IP:%s,Port:%d...\r\n",
             ipaddr_ntoa((const ip_addr_t*)&sock_info.sin_addr.s_addr), ota_port);
    // connect to http server
    ret = connect(socket_id, (struct sockaddr*)&sock_info, sizeof(sock_info));

    if (ret == -1) {
        ESP_LOGE(TONE_OTA_TAG, "errno:%d, connect to server failed!", errno);
        return ESP_FAIL;
    }

    ESP_LOGI(TONE_OTA_TAG, "Info: connect to server success!");
    return ESP_OK;
}

static uint32_t tone_status_crc(const ToneStatus* s)
{
    return crc32_le(UINT32_MAX, (uint8_t*)&s->index, sizeof(ToneStatus) - sizeof(int));
}

static bool tone_status_valid(const ToneStatus* s)
{
    return s->index != UINT32_MAX && s->crc == tone_status_crc(s);
}

esp_err_t process_flash_tone_part(ToneStatus* status, bool w_r_flag)
{
    esp_err_t ota_res = ESP_OK;
    ToneStatus A_status, B_status;
    const esp_partition_t* flash_tone_status = esp_partition_find_first(ESP_PARTITION_TYPE_DATA, 0x06, "tonestatus");

    if (flash_tone_status == NULL) {
        ota_res = TONE_OTA_PARTITION_NOT_FOUND;
    }

    esp_partition_read(flash_tone_status, 0, &A_status, sizeof(ToneStatus));
    esp_partition_read(flash_tone_status, SPI_FLASH_SEC_SIZE, &B_status, sizeof(ToneStatus));

    if (tone_status_valid(&A_status) && tone_status_valid(&B_status)) {
        if (A_status.index >= B_status.index) {
            if (w_r_flag == true) {
                B_status.latest_flag = status->latest_flag;
                B_status.tone_ota_status = status->tone_ota_status;
                B_status.index = A_status.index + 1;
                B_status.crc = tone_status_crc(&B_status);
                esp_partition_erase_range(flash_tone_status, SPI_FLASH_SEC_SIZE, SPI_FLASH_SEC_SIZE);
                esp_partition_write(flash_tone_status, SPI_FLASH_SEC_SIZE, &B_status, sizeof(ToneStatus));
            } else {
                memcpy(status, &A_status, sizeof(ToneStatus));
            }
        } else {
            if (w_r_flag == true) {
                A_status.latest_flag = status->latest_flag;
                A_status.tone_ota_status = status->tone_ota_status;
                A_status.index = B_status.index + 1;
                A_status.crc = tone_status_crc(&A_status);
                esp_partition_erase_range(flash_tone_status, 0, SPI_FLASH_SEC_SIZE);
                esp_partition_write(flash_tone_status, 0, &A_status, sizeof(ToneStatus));
            } else {
                memcpy(status, &B_status, sizeof(ToneStatus));
            }
        }
    } else if (tone_status_valid(&A_status)) {
        if (w_r_flag == true) {
            B_status.latest_flag = status->latest_flag;
            B_status.tone_ota_status = status->tone_ota_status;
            B_status.index = A_status.index + 1;
            B_status.crc = tone_status_crc(&B_status);
            esp_partition_erase_range(flash_tone_status, SPI_FLASH_SEC_SIZE, SPI_FLASH_SEC_SIZE);
            esp_partition_write(flash_tone_status, SPI_FLASH_SEC_SIZE, &B_status, sizeof(ToneStatus));
        } else {
            memcpy(status, &A_status, sizeof(ToneStatus));
        }
    } else if (tone_status_valid(&B_status)) {
        if (w_r_flag == true) {
            A_status.latest_flag = status->latest_flag;
            A_status.tone_ota_status = status->tone_ota_status;
            A_status.index = B_status.index + 1;
            A_status.crc = tone_status_crc(&A_status);
            esp_partition_erase_range(flash_tone_status, 0, SPI_FLASH_SEC_SIZE);
            esp_partition_write(flash_tone_status, 0, &A_status, sizeof(ToneStatus));
        } else {
            memcpy(status, &B_status, sizeof(ToneStatus));
        }
    } else if (A_status.tone_ota_status == OTA_STATUS_INIT &&
               B_status.tone_ota_status == OTA_STATUS_INIT) {
        if (w_r_flag == true) {
            A_status.latest_flag = status->latest_flag;
            A_status.tone_ota_status = status->tone_ota_status;
            A_status.index = 1;
            A_status.crc = tone_status_crc(&A_status);
            esp_partition_erase_range(flash_tone_status, 0, SPI_FLASH_SEC_SIZE);
            esp_partition_write(flash_tone_status, 0, &A_status, sizeof(ToneStatus));
        } else {
            A_status.latest_flag = 0x01;
            A_status.tone_ota_status = OTA_STATUS_INIT;
            A_status.index = 1;
            A_status.crc = tone_status_crc(&A_status);
            memcpy(status, &A_status, sizeof(ToneStatus));
        }
    } else {
        ESP_LOGE(TONE_OTA_TAG, "flash tone OTA status partition error，A: 0x%08x B: 0x%08x", A_status.tone_ota_status, B_status.tone_ota_status);
        ota_res = TONE_OTA_PARTITION_INVALID;
        esp_partition_erase_range(flash_tone_status, 0, 2 * SPI_FLASH_SEC_SIZE);
    }

    return ota_res;
}
esp_err_t update_latest_flag(void)
{
    ToneStatus status;
    esp_err_t ota_res = ESP_OK;
    ota_res = process_flash_tone_part(&status, false);

    if (ota_res == ESP_OK) {
        status.latest_flag = 0x00;
        return process_flash_tone_part(&status, true);
    }

    return ota_res;

}
esp_err_t esp_tone_ota_audio_set_status(uint32_t status)
{
    ToneStatus tone_status;
    esp_err_t ota_res = ESP_OK;
    ota_res = process_flash_tone_part(&tone_status, false);

    if (ota_res == ESP_OK) {
        tone_status.tone_ota_status = status;

        if (status == (OTA_STATUS_END | OTA_RES_SUCCESS)) {
            tone_status.latest_flag = 0x01;
        }

        return process_flash_tone_part(&tone_status, true);
    }

    return ota_res;
}

uint32_t esp_tone_ota_audio_get_status(void)
{
    ToneStatus tone_status;
    esp_err_t ota_res = ESP_OK;
    ota_res = process_flash_tone_part(&tone_status, false);

    if (ota_res == ESP_OK) {
        return tone_status.tone_ota_status;
    }

    return ota_res;
}

void tone_ota_audio_begin_task(void* param)
{
    char* url = (char*)param;
    esp_err_t ota_res = ESP_OK;
    esp_ota_handle_t update_handle = 0 ;
    const esp_partition_t* flash_tone_partition = NULL;

    ESP_LOGI(TONE_OTA_TAG, "Starting flash tone OTA ");

    ota_res = update_latest_flag();

    if (ota_res != ESP_OK) {
        esp_tone_ota_audio_set_status(OTA_STATUS_START | OTA_RES_FAILED);
        task_fatal_error(&ota_res);
    }

    esp_tone_ota_audio_set_status(OTA_STATUS_START | OTA_RES_SUCCESS);

    const esp_partition_t* flash_tone_part = esp_partition_find_first(ESP_PARTITION_TYPE_DATA, 0x04, "flashTone");

    if (flash_tone_part == NULL) {
        ota_res = TONE_OTA_PARTITION_NOT_FOUND;
        task_fatal_error(&ota_res);
    }

    /*connect to http server*/
    if (connect_to_http_server(url) == ESP_OK) {
        /*erase all flash tone partition*/
        esp_tone_ota_audio_set_status(OTA_STATUS_ERASE | OTA_RES_PROGRESS);
        ota_res = esp_partition_erase_range(flash_tone_part, 0, flash_tone_part->size);

        if (ota_res != ESP_OK) {
            esp_tone_ota_audio_set_status(OTA_STATUS_ERASE | OTA_RES_FAILED);
            task_fatal_error(&ota_res);
        }

        esp_tone_ota_audio_set_status(OTA_STATUS_ERASE | OTA_RES_SUCCESS);
    } else {
        ota_res = TONE_OTA_CONNECT_SERVER_FAIL;
        task_fatal_error(&ota_res);
    }

    const char* GET_FORMAT =
        "GET %s HTTP/1.0\r\n"
        "Host: %s:%d\r\n"
        "User-Agent: esp-idf/1.0\r\n\r\n";
    char* http_request = NULL;
    int get_len = asprintf(&http_request, GET_FORMAT, ota_path, ota_host, ota_port);
    int res = send(socket_id, http_request, get_len, 0);
    printf("%s", http_request);
    free(http_request);

    if (res < 0) {
        ESP_LOGE(TONE_OTA_TAG, "Send GET request to server failed");
        ota_res = TONE_OTA_SEND_HTTP_REQ_FAIL;
        task_fatal_error(&ota_res);
    } else {
        ESP_LOGI(TONE_OTA_TAG, "Send GET request to server succeeded");
    }

    bool resp_body_start = false, flag = true;
    /*deal with all receive packet*/
    esp_tone_ota_audio_set_status(OTA_STATUS_WRITE | OTA_RES_PROGRESS);

    while (flag) {
        memset(text, 0, TEXT_BUFFSIZE);
        memset(ota_write_data, 0, BUFFSIZE);
        int buff_len = recv(socket_id, text, TEXT_BUFFSIZE, 0);

        if (buff_len < 0) { /*receive error*/
            ESP_LOGE(TONE_OTA_TAG, "Error: receive data error! errno=%d", errno);
            ota_res = TONE_OTA_RECV_PACKET_ERROR;
            task_fatal_error(&ota_res);
        } else if (buff_len > 0 && !resp_body_start) { /*deal with response header*/
            memcpy(ota_write_data, text, buff_len);
            resp_body_start = read_past_http_header(text, buff_len, flash_tone_part);
        } else if (buff_len > 0 && resp_body_start) { /*deal with response body*/
            memcpy(ota_write_data, text, buff_len);
            ota_res = esp_partition_write(flash_tone_part, binary_file_length, ota_write_data, buff_len);

            if (ota_res != ESP_OK) {
                ESP_LOGE(TONE_OTA_TAG, "Error: esp_ota_write failed! err=0x%x", ota_res);
                esp_tone_ota_audio_set_status(OTA_STATUS_WRITE | OTA_RES_FAILED);
                task_fatal_error(&ota_res);
            }

            binary_file_length += buff_len;
            ESP_LOGI(TONE_OTA_TAG, "Have written image length %d", binary_file_length);
        } else if (buff_len == 0) {  /*packet over*/
            flag = false;
            ESP_LOGI(TONE_OTA_TAG, "Connection closed, all packets received");
            esp_tone_ota_audio_set_status(OTA_STATUS_WRITE | OTA_RES_SUCCESS);
            break;
        } else {
            ESP_LOGE(TONE_OTA_TAG, "Unexpected recv result");
            ota_res = TONE_OTA_UNEXCEPTED_RECV_ERROR;
            task_fatal_error(&ota_res);
        }
    }

    ota_res = ESP_OK;
    xQueueSend(toneotaqueue, &ota_res, 0);
    vTaskDelete(NULL);
}

esp_err_t esp_tone_ota_audio_end(void)
{
    const esp_partition_t* flash_tone_part = esp_partition_find_first(ESP_PARTITION_TYPE_DATA, 0x04, "flashTone");
    esp_tone_ota_audio_set_status(OTA_STATUS_END | OTA_RES_PROGRESS);
    int crc = 0;

    if (flash_tone_part == NULL) {
        return TONE_OTA_PARTITION_NOT_FOUND;
    }

    uint8_t check_buff[1024];
    static int offset = 0;

    while (binary_file_length > 1024) {
        esp_partition_read(flash_tone_part, offset, check_buff, 1024);
        crc = crc32_le(crc, check_buff, 1024);
        offset += 1024;
        binary_file_length -= 1024;
        memset(check_buff, 0, 1024);
    }

    if (binary_file_length <= 1024) {
        esp_partition_read(flash_tone_part, offset, check_buff, binary_file_length);
        crc = crc32_le(crc, check_buff, binary_file_length - 4);
        int cmp_value;
        memcpy(&cmp_value, &check_buff[binary_file_length - 4], sizeof(int));
        ESP_LOGI(TONE_OTA_TAG, "crc :0x%08x,cmp: 0x%08x", crc , cmp_value);
        binary_file_length = 0;
        offset = 0;

        if (crc == cmp_value) {
            esp_tone_ota_audio_set_status(OTA_STATUS_END | OTA_RES_SUCCESS);
        } else {
            esp_tone_ota_audio_set_status(OTA_STATUS_END | OTA_RES_FAILED);
            return TONE_OTA_ENC_CHECKSUM_ERROR;
        }
    }

    return ESP_OK;
}

esp_err_t esp_tone_ota_audio_begin(const char* url)
{
    toneotaqueue = xQueueCreate(2, sizeof(uint32_t));
    int ota_res;
    xTaskCreate(&tone_ota_audio_begin_task, "tone_ota_audio_begin_task", 8192, url, 5, NULL);

    while (1) {
        if (pdTRUE == xQueueReceive(toneotaqueue, &ota_res, portMAX_DELAY)) {
            if (ota_res != ESP_OK) {
                ESP_LOGE(TONE_OTA_TAG, "tone ota failed ,reason 0x%08x", ota_res);
            }

            break;
        } else {
            return ESP_FAIL;
        }
    }

    return ota_res;
}
