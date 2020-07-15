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

#define LED_CTRL_LOG_TAG			"LED_CTRL"
#define LED_CTRL_SERVICE_QUIT		-1
#define LED_TAG 					"LED"
#define LED_LIGHT					1					//led状态为亮
#define LED_CLOSE					(!LED_LIGHT)		//led状态为灭
#define DELAY_TIME					200		//200毫秒
#define LED_FAST_FLASH_TIME			200		//400毫秒
#define LED_SLOW_FLASH_TIME			800		//800毫秒

static xQueueHandle g_led_ctrl_msg_queue = NULL;
static LED_CTRL_MODE_E g_led_mode = LED_CTRL_EYE_FLASHING_SLOW;	//眼灯初始状态设为1秒闪1次
int g_time = 0;

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

//眉毛灯和眼睛灯 状态接口
#if MODULE_EXTRATERRESTRIAL_SEVEN_CASE
void change_eyebrow_led_state(void)
{
	int led_level = gpio_get_level(EYEBROW_LED_GPIO);
	ESP_LOGI(LED_TAG, "led_level=%d", led_level);
	gpio_set_level(EYEBROW_LED_GPIO, !led_level);	//1为点亮，0为灭
}

void change_eye_led_state(void)
{
	int led_level = gpio_get_level(EYE_LED_GPIO);
	ESP_LOGI(LED_TAG, "led_level=%d", led_level);
	gpio_set_level(EYE_LED_GPIO, !led_level);		//1为点亮，0为灭
}

//眉毛灯常亮
void eyebrow_led_light()
{
	gpio_set_level(EYEBROW_LED_GPIO, LED_LIGHT);
}

//眉毛灯灭
void eyebrow_led_close()
{
	gpio_set_level(EYEBROW_LED_GPIO, LED_CLOSE);
}

//眼睛灯常亮
void eye_led_light()
{
	gpio_set_level(EYE_LED_GPIO, LED_LIGHT);
}

//眼睛灯灭
void eye_led_close()
{
	gpio_set_level(EYE_LED_GPIO, LED_CLOSE);
}

//嘴巴灯打开
void mouth_led_open()
{
	gpio_set_level(MOUTH_LED_GPIO, LED_LIGHT);
}

//嘴巴灯关闭
void mouth_led_close()
{
	gpio_set_level(MOUTH_LED_GPIO, LED_CLOSE);
}


//眼睛灯慢速闪动（0.8秒1次）
void eye_led_slow_flashing()
{
	int delay_time = g_time * DELAY_TIME;
	if (delay_time < LED_SLOW_FLASH_TIME)
	{
		eye_led_light();
	}
	else if (delay_time >= LED_SLOW_FLASH_TIME && delay_time < 2 * LED_SLOW_FLASH_TIME)
	{
		eye_led_close();
	}
	else
	{
		eye_led_light();
		g_time = 0;
	}
	g_time++;
}

//眼睛灯快速闪动（0.2秒1次）
void eye_led_fast_flashing()
{
	int delay_time = g_time * DELAY_TIME;
	if (delay_time < LED_FAST_FLASH_TIME)
	{
		eye_led_light();
	}
	else if (delay_time >= LED_FAST_FLASH_TIME && delay_time < 2 * LED_FAST_FLASH_TIME)
	{
		eye_led_close();
	}
	else
	{
		eye_led_light();
		g_time = 0;
	}
	g_time++;
}

//灯模式设置接口
void led_ctrl_set_mode(LED_CTRL_MODE_E mode)
{	
	if (g_led_ctrl_msg_queue != NULL)
	{
		xQueueSend(g_led_ctrl_msg_queue, &mode, 0);
	}
}

//初始化状态（眉毛灯常亮，眼睛灯慢闪）
void eyebrow_eye_led_state_init()
{
	eyebrow_led_light();
}
#endif

static void task_led_ctrl_process(void *pv)
{
//	printf("in task_led_ctrl_process 1 !\n");
	LED_CTRL_MODE_E led_ctrl_event = LED_CTRL_INITIAL;
    xQueueHandle *queue = (xQueueHandle *) pv;

	while (1)	
	{
		if (xQueueReceive(queue, &led_ctrl_event, DELAY_TIME) == pdPASS) 
		{
			switch (led_ctrl_event)
			{
				case LED_CTRL_EYE_LIGHT:
				{
					g_led_mode = LED_CTRL_EYE_LIGHT;
					eye_led_light();
					break;
				}
				case LED_CTRL_EYE_FLASHING_SLOW:
				{
					g_led_mode = LED_CTRL_EYE_FLASHING_SLOW;
					break;
				}
				case LED_CTRL_EYE_FLASHING_FAST:
				{
					g_led_mode = LED_CTRL_EYE_FLASHING_FAST;
					break;
				}
				case LED_CTRL_ALL_CLOSE:
				{
					eye_led_close();
					eyebrow_led_close();
					mouth_led_close();
					g_led_mode = LED_CTRL_ALL_CLOSE;
					break;
				}
				default:
					break;
			}
			
			if (led_ctrl_event == LED_CTRL_SERVICE_QUIT)
			{
				break;
			}
		}
		
		switch (g_led_mode)
		{
			case LED_CTRL_EYE_FLASHING_SLOW:
			{
				eye_led_slow_flashing();
				break;
			}
			case LED_CTRL_EYE_FLASHING_FAST:
			{
				eye_led_fast_flashing();
				break;
			}
			default:
			{
				break;
			}
		}

		if (g_led_mode == LED_CTRL_ALL_CLOSE)
		{
			break;
		}
	}
    vTaskDelete(NULL);

}

void led_ctrl_service_create(void)
{
	g_led_ctrl_msg_queue = xQueueCreate(5, sizeof(char *));
	
	if (xTaskCreate(task_led_ctrl_process,
					"task_led_ctrl_process",
					512*2,
					g_led_ctrl_msg_queue,
					4,
					NULL) != pdPASS)
	{

		ESP_LOGE(LED_CTRL_LOG_TAG, "ERROR creating task_led_ctrl_process task! Out of memory?");
	}

	led_ctrl_set_mode(LED_CTRL_EYE_FLASHING_SLOW);
}

void led_ctrl_service_delete(void)
{
	int led_ctrl_event = LED_CTRL_SERVICE_QUIT;
	
	if (g_led_ctrl_msg_queue != NULL)
	{
		xQueueSend(g_led_ctrl_msg_queue, &led_ctrl_event, 0);
	}
}

void UserGpioInit()
{
    gpio_config_t  io_conf;
    memset(&io_conf, 0, sizeof(io_conf));
    io_conf.pin_bit_mask     = EYEBROW_LED_SEL | EYE_LED_SEL | MOUTH_LED_SEL;
	io_conf.mode			 = GPIO_MODE_OUTPUT;
    io_conf.pull_up_en       = GPIO_PULLUP_DISABLE;
	io_conf.pull_down_en     = GPIO_PULLDOWN_ENABLE;
    io_conf.intr_type        = GPIO_PIN_INTR_DISABLE;
    gpio_config(&io_conf);

	memset(&io_conf, 0, sizeof(io_conf));
    io_conf.pin_bit_mask     = CHARGING_STATE_SEL;
	io_conf.mode			 = GPIO_MODE_INPUT;
    io_conf.pull_up_en       = GPIO_PULLUP_ENABLE;
    io_conf.intr_type        = GPIO_PIN_INTR_DISABLE ;
    gpio_config(&io_conf);

#if MODULE_EXTRATERRESTRIAL_SEVEN_CASE
	mouth_led_open();
	led_ctrl_service_create();		//启动LED线程
	vTaskDelay(DELAY_TIME - 2);		//延迟使眼灯和眉毛灯同时亮起
	eyebrow_eye_led_state_init();	//初始化眉毛灯和眼灯，实际只初始化了眉毛灯为常亮
#endif
}

