#include <string.h>
#include "driver/gpio.h"
#include "driver/ledc.h"

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"
#include "freertos/timers.h"
#include "esp_types.h"
#include "esp_log.h"
#include "led_ctrl.h"
#include "userconfig.h"
#include "mcu_serial_comm.h"

#define ESP32_LYRA32T
// user gpio definition

#define LED_TAG "LED"

#define WHILE_DELAY					10			//10毫秒
#define STATE_LED_FLICKER_TIME		500			//500毫秒
#define LED_STATE_FLICKER			0			//led闪烁
#define LED_STATE_LIGHT				1			//led常亮
#define COLOR_LED_FLICKER_TIME		300			//300毫秒

xTimerHandle g_state_led_flicker = NULL;
xTimerHandle g_color_led_flicker = NULL;
int g_color_led_flicker_stop = 0;

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
            .bit_num = LEDC_TIMER_12_BIT,                        //set timer counter bit number
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

#if MODULE_BOBI_CASE
void switch_color_led(void)
{
	color_led_flicker_stop();
	int led_level = gpio_get_level(LED_EAR_LED_GPIO);
	ESP_LOGI(LED_TAG, "led_level=%d", led_level);
	gpio_set_level(LED_EAR_LED_GPIO, !led_level);
}
#endif

//七彩指示灯开
void color_led_on()
{
	gpio_set_level(LED_EAR_LED_GPIO, 0x01);
}

//七彩指示灯关
void color_led_off()
{
	gpio_set_level(LED_EAR_LED_GPIO, 0x00);
}

void color_led_flicker()
{	
	if (g_color_led_flicker != NULL && g_color_led_flicker_stop == 0)
	{
		static int while_time = 0;	//回调循环的次数flag
	
		int delay_ms = while_time * STATE_LED_FLICKER_TIME;
		if (delay_ms < STATE_LED_FLICKER_TIME)
		{
			while_time = 1;
			color_led_on();
		}
		else
		{
			while_time = 0;
			color_led_off();
		}
		
		xTimerStart(g_color_led_flicker, 50/portTICK_PERIOD_MS);
	}
	else
	{
		xTimerStop(g_color_led_flicker, 50/portTICK_PERIOD_MS);
		xTimerDelete(g_color_led_flicker, 50/portTICK_PERIOD_MS);
		g_color_led_flicker = NULL;
	}
}

void color_led_flicker_stop(void)
{
	g_color_led_flicker_stop = 1;
}

void color_led_flicker_start()
{
	g_color_led_flicker_stop = 0;
	g_color_led_flicker = xTimerCreate((const char*)"color_led_flicker", COLOR_LED_FLICKER_TIME/portTICK_PERIOD_MS, pdFALSE, &g_color_led_flicker,
                color_led_flicker);
	xTimerStart(g_color_led_flicker, 50/portTICK_PERIOD_MS);
}

//状态指示灯开
void state_led_on()
{
	gpio_set_level(LED_INDICATOR_SYS, 0x01);
}

//状态指示灯关
void state_led_off()
{
	gpio_set_level(LED_INDICATOR_SYS, 0x00);
}

void state_led_flicker()
{	
	if (g_state_led_flicker != NULL)
	{
		static int while_time = 0;	//回调循环的次数flag
	
		int delay_ms = while_time * STATE_LED_FLICKER_TIME;
		if (delay_ms < STATE_LED_FLICKER_TIME)
		{
			while_time = 1;
			state_led_on();
		}
		else
		{
			while_time = 0;
			state_led_off();
		}
		
		xTimerStart(g_state_led_flicker, 50/portTICK_PERIOD_MS);
	}
	else
	{
		xTimerStop(g_state_led_flicker, 50/portTICK_PERIOD_MS);
		xTimerDelete(g_state_led_flicker, 50/portTICK_PERIOD_MS);
		g_state_led_flicker = NULL;
	}
}

void state_led_flicker_init()
{
	g_state_led_flicker = xTimerCreate((const char*)"state_led_flicker", STATE_LED_FLICKER_TIME/portTICK_PERIOD_MS, pdFALSE, &g_state_led_flicker,
                state_led_flicker);
	xTimerStart(g_state_led_flicker, 50/portTICK_PERIOD_MS);
}

void UserGpioInit()
{
	gpio_config_t  io_conf;
	memset(&io_conf, 0, sizeof(io_conf));
	io_conf.pin_bit_mask	 = GPIO_SEL_LED_GREEN | GPIO_SEL_LED_RED | BATTERY_POWER_SCOUTING_SEL
#if MODULE_BOBI_CASE
	| LED_EAR_LED_SEL
#endif
		;

	io_conf.mode			 = GPIO_MODE_INPUT_OUTPUT;
	io_conf.pull_up_en		 = GPIO_PULLUP_ENABLE;
	io_conf.pull_down_en     = GPIO_PULLDOWN_DISABLE;
	io_conf.intr_type		 = GPIO_PIN_INTR_DISABLE ;
	gpio_config(&io_conf);
	ESP_LOGI(LED_TAG, "LED sys=%d,net=%d", LED_INDICATOR_SYS, LED_INDICATOR_NET);

	gpio_set_level(LED_INDICATOR_NET, 0x01); // wifi green
	gpio_set_level(LED_INDICATOR_SYS, 0x01); // red

#if MODULE_BATTERY_POWER_SCOUTING_CASE
	memset(&io_conf, 0, sizeof(io_conf));
	io_conf.pin_bit_mask = BATTERY_POWER_SCOUTING_SEL;
	io_conf.mode		 = GPIO_MODE_INPUT;
	io_conf.pull_up_en	 = 0;
	io_conf.pull_down_en = GPIO_PULLDOWN_ENABLE;
	io_conf.intr_type	 = GPIO_PIN_INTR_DISABLE;
	gpio_config(&io_conf);
#endif

	gpio_install_isr_service(0);

//	mcu_serial_send_led_ctrl(1, 0);

#if MODULE_BOBI_CASE
	gpio_set_level(LED_EAR_LED_GPIO, 1);
#endif

	state_led_flicker_init();
}

