
#include <string.h>
#include "driver/gpio.h"
#include "driver/ledc.h"

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"
#include "freertos/timers.h"
#include "esp_types.h"
#include "esp_log.h"
#include "led.h"
#include "userconfig.h"

#define ESP32_LYRA32T
// user gpio definition

#define LED_TAG "LED"
#define LED_LIGHT			1	//led状态为亮
#define LED_CLOSE			0	//led状态为灭

typedef struct {
    uint32_t period;  // unit Hz
    uint32_t duty;    // unit 0~100,off time
} LedFreq;

typedef struct {
    LedWorkState    state;
    LedFreq         freq;
} LedWorkFreq;

typedef struct {
    PwmChannel    ch;
    ledc_timer_t  timer;
    uint32_t      pin;
    LedWorkState  curtState;
} LedPwmPara;

// net is green led.
// sys is red led.
static LedWorkFreq arryLedFreq[] = {
    {LedWorkState_NetSetting,       {2, 40     }   },
    {LedWorkState_NetConnectOk,     {1, 100    }   },
    {LedWorkState_NetDisconnect,    {1, 80     }   },

    {LedWorkState_SysBufLow,        {3, 70     }   },
    {LedWorkState_SysPause,         {1, 100     }   },
    {LedWorkState_Off,              {1, 0      }   },
    {0,                             {0,   0    }   },
};

LedPwmPara arryLedPara[] = {
    {PwmChannel0,   LEDC_TIMER_0,   LED_INDICATOR_NET,   0xFF},
    {PwmChannel1,   LEDC_TIMER_1,   LED_INDICATOR_SYS,   0xFF},
    {0xFF,          0xFF,        0xFF,                0xFF},
};

void LedIndicatorSet(uint32_t num, LedWorkState state)
{
    if ((num > 1) || (state > LedWorkState_Off)) {
        ESP_LOGE(LED_TAG, "Para err.num=%d,state=%d", num, state);
        return;
    }
    if (arryLedPara[num].curtState != state) {
        uint32_t precision = 1 << LEDC_TIMER_12_BIT;
        uint32_t fre = arryLedFreq[state].freq.period;
        uint32_t duty = precision * arryLedFreq[state].freq.duty / 100;
        arryLedPara[num].curtState = state;

        ledc_timer_config_t timer_conf = {
#if IDF_3_0 == 1
            .duty_resolution = LEDC_TIMER_12_BIT,                //set timer counter bit number
#else
            .bit_num = LEDC_TIMER_12_BIT,                        //set timer counter bit number
#endif
            .freq_hz = fre,                                      //set frequency of pwm, here, 1000Hz
            .speed_mode = LEDC_HIGH_SPEED_MODE,                  //timer mode,
            .timer_num = arryLedPara[num].timer & 3,             //timer number
        };
        ledc_timer_config(&timer_conf);                          //setup timer.
        ledc_channel_config_t ledc_conf = {
            .channel = arryLedPara[num].ch,                      //set LEDC channel 0
            .duty = duty,                                        //set the duty for initialization.(duty range is 0 ~ ((2**bit_num)-1)
            .gpio_num = arryLedPara[num].pin,                    //GPIO number
            .intr_type = LEDC_INTR_DISABLE,                      //GPIO INTR TYPE, as an example, we enable fade_end interrupt here.
            .speed_mode = LEDC_HIGH_SPEED_MODE,                  //set LEDC mode, from ledc_mode_t
            .timer_sel = arryLedPara[num].timer & 3,             //set LEDC timer source, if different channel use one timer, the frequency and bit_num of these channels should be the same
        };
        ledc_channel_config(&ledc_conf);                         //setup the configuration
    }
}

void switch_color_led(void)
{
	int led_level = gpio_get_level(LED_QICAI_GPIO);
	gpio_set_level(LED_QICAI_GPIO, !led_level);
}

void UserGpioInit()
{
    gpio_config_t  io_conf;
	
    memset(&io_conf, 0, sizeof(io_conf));
    io_conf.pin_bit_mask     = GPIO_SEL_LED_GREEN | GPIO_SEL_LED_RED | LED_QICAI_SEL;
	io_conf.mode			 = GPIO_MODE_INPUT_OUTPUT;
    io_conf.pull_up_en       = GPIO_PULLUP_ENABLE;
    io_conf.intr_type        = GPIO_PIN_INTR_DISABLE ;
    gpio_config(&io_conf);

	LedIndicatorSet(LedIndexNet, LedWorkState_NetDisconnect);

	
}
