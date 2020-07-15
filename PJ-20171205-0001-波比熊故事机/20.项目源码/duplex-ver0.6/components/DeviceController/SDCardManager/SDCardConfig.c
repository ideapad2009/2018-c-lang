#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_err.h"
#include "esp_vfs_fat.h"
#include "driver/sdmmc_host.h"
#include "driver/sdmmc_defs.h"
#include "sdmmc_cmd.h"

#include "SDCardDef.h"
#include "SDCardConfig.h"
#include "userconfig.h"


#define SD_CARD_TAG                 "SD_CARD_UTIL"

esp_err_t sd_card_mount(const char* basePath)
{
    sdmmc_host_t host = SDMMC_HOST_DEFAULT();

    // To use 1-line SD mode, uncomment the following line:
    host.flags = SDMMC_HOST_FLAG_1BIT;

    sdmmc_slot_config_t slot_config = SDMMC_SLOT_CONFIG_DEFAULT();
    slot_config.gpio_cd = SD_CARD_INTR_GPIO;
    esp_vfs_fat_sdmmc_mount_config_t mount_config = {
        .format_if_mount_failed = false,
        .max_files = SD_CARD_OPEN_FILE_NUM_MAX
    };

    sdmmc_card_t* card;
    esp_err_t ret = esp_vfs_fat_sdmmc_mount(basePath, &host, &slot_config, &mount_config, &card);
    switch (ret) {
    case ESP_OK:
        // Card has been initialized, print its properties
        sdmmc_card_print_info(stdout, card);
        ESP_LOGI(SD_CARD_TAG, "CID name %s!\n", card->cid.name);
        break;
    case ESP_ERR_INVALID_STATE:
        ESP_LOGE(SD_CARD_TAG, "File system already mounted");
        break;
    case ESP_FAIL:
        ESP_LOGE(SD_CARD_TAG, "Failed to mount filesystem. If you want the card to be formatted, set format_if_mount_failed = true.");
        break;
    default:
        ESP_LOGE(SD_CARD_TAG, "Failed to initialize the card (%d). Make sure SD card lines have pull-up resistors in place.", ret);
        break;
    }

    return ret;

}

esp_err_t sd_card_unmount(void)
{
    esp_err_t ret = esp_vfs_fat_sdmmc_unmount();
    if (ret == ESP_ERR_INVALID_STATE) {
        ESP_LOGE(SD_CARD_TAG, "File system not mounted");
    }
    return ret;
}

static void IRAM_ATTR sd_card_gpio_intr_handler(void* arg)
{
    xQueueHandle queue = (xQueueHandle) arg;
    gpio_num_t gpioNum = (gpio_num_t) SD_CARD_INTR_GPIO;
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    if (queue)
        xQueueSendFromISR(queue, &gpioNum, &xHigherPriorityTaskWoken);
    if (xHigherPriorityTaskWoken) {
        portYIELD_FROM_ISR();
    }
}

int sd_card_status_detect()
{
    vTaskDelay(1000 / portTICK_RATE_MS);
    return gpio_get_level(SD_CARD_INTR_GPIO);
}

int sd_card_insert_status(void)
{
	return !gpio_get_level(SD_CARD_INTR_GPIO);
}

void sd_card_intr_init(xQueueHandle queue)
{
    gpio_config_t  io_conf;
    memset(&io_conf, 0, sizeof(io_conf));
#if IDF_3_0
    io_conf.intr_type = GPIO_INTR_ANYEDGE;
#else
    io_conf.intr_type = GPIO_PIN_INTR_ANYEGDE;
#endif
    io_conf.pin_bit_mask = SD_CARD_INTR_SEL;
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pull_up_en = 1;
    gpio_config(&io_conf);
    gpio_isr_handler_add(SD_CARD_INTR_GPIO, sd_card_gpio_intr_handler, (void*) queue);
}
