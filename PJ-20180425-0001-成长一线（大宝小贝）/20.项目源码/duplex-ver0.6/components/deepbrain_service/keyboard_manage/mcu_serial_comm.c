#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "driver/uart.h"
#include "freertos/queue.h"
#include "esp_log.h"
#include "soc/uart_struct.h"

#include "mcu_serial_comm.h"
#include "device_api.h"
#include "keyboard_service.h"

#define TAG_MCU_COMM "MCU_COMM"

#define SERIAL_TXD GPIO_NUM_21
#define SERIAL_RXD GPIO_NUM_19
#define SERIAL_UART_NUM UART_NUM_1

#define SERIAL_COMM_HEAD 0x55

typedef enum
{
	MCU_SERIAL_COMM_STATUS_HEAD1 = 0,
	MCU_SERIAL_COMM_STATUS_HEAD2,
	MCU_SERIAL_COMM_STATUS_DATA_LEN,
	MCU_SERIAL_COMM_STATUS_DATA,
	MCU_SERIAL_COMM_STATUS_DATA_END,
}MCU_SERIAL_PARSE_STATUS_T;

typedef enum
{
	SERIAL_COMM_CMD_KEY = 0x01,
	SERIAL_COMM_CMD_BATTERY_VOL,
	SERIAL_COMM_CMD_START = 0x50,
	SERIAL_COMM_CMD_SLEEP,
}SERIAL_COMM_CMD_T;

typedef struct
{
	MCU_SERIAL_PARSE_STATUS_T _status;
	int recv_count;
	unsigned char rxd_buf[16];
}MCU_SERIAL_PARSE_HANDLER_T;

mcu_key_cb g_key_cb = NULL;
mcu_battery_cb g_battery_cb = NULL;
static SemaphoreHandle_t g_lock_keyborad = NULL;
static int g_keyboard_lock_status = 0;

PLAYER_STATUS player_status = PLAY_NULL;

const LED_EXP_CMD_T led_exp_cmd[] = { 
	{{0x55, 0x55, 0x02, 0x50, 0xfc}},	//开机
	{{0x55, 0x55, 0x02, 0x51, 0xfd}},	//
	{{0x55, 0x55, 0x02, 0x52, 0xfe}},	//
	{{0x55, 0x55, 0x02, 0x53, 0xff}},	//
	{{0x55, 0x55, 0x02, 0x54, 0x00}},	//	
	{{0x55, 0x55, 0x02, 0x55, 0x01}},	//
	{{0x55, 0x55, 0x02, 0x56, 0x02}},	//
	{{0x55, 0x55, 0x02, 0x57, 0x03}},	//
	{{0x55, 0x55, 0x02, 0x58, 0x04}},	//
	{{0x55, 0x55, 0x02, 0x59, 0x05}},	//
	{{0x55, 0x55, 0x02, 0x5a, 0x06}},	//
	{{0x55, 0x55, 0x02, 0x5b, 0x07}},	//
	{{0x55, 0x55, 0x02, 0x5c, 0x08}},	//
	{{0x55, 0x55, 0x02, 0x5d, 0x09}},	//
	{{0x55, 0x55, 0x02, 0x55, 0x01}},	//成功
	{{0x55, 0x55, 0x02, 0xaa, 0x56}},	//失败
};

static int mcu_serial_check_sum(unsigned char *_data, size_t _data_len)
{
	int i = 0;
	unsigned char sum = 0;
	
	for (i=0; i < _data_len - 1; i++)
	{
		sum += *(_data+i);
	}
	//printf("sum=%02x,%02x\n", sum, *(_data+_data_len-1));
	if (*(_data+_data_len-1) == sum)
	{
		return 1;
	}
	else
	{
		return 0;
	}
}

static void mcu_serial_init(void)
{
	uart_config_t uart_config = 
	{
		.baud_rate = 9600,
		.data_bits = UART_DATA_8_BITS,
		.parity = UART_PARITY_DISABLE,
		.stop_bits = UART_STOP_BITS_1,
		.flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
		.rx_flow_ctrl_thresh = 122,
	};
	//Configure UART1 parameters
	uart_param_config(SERIAL_UART_NUM, &uart_config);
	//Set UART1 pins(TX: IO4, RX: I05, RTS: IO18, CTS: IO19)
	uart_set_pin(SERIAL_UART_NUM, SERIAL_TXD, SERIAL_RXD, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
	//Install UART driver (we don't need an event queue here)
	//In this example we don't even use a buffer for sending data.
	uart_driver_install(SERIAL_UART_NUM, 512, 512, 0, NULL, 0);
}


static void mcu_serial_cmd_process(unsigned char *_data, size_t _data_len)
{
	SERIAL_COMM_CMD_T cmd = _data[3];

	switch (cmd)
	{
		case SERIAL_COMM_CMD_KEY:
		{
			//printf("cmd=%02x,key=%02x\n", cmd, _data[4]);
			if (_data[4] > DEVICE_KEY_START && _data[4] < DEVICE_KEY_END)
			{
				if (g_key_cb != NULL)
				{
					g_key_cb(_data[4]);
				}
			}
			break;
		}
		case SERIAL_COMM_CMD_BATTERY_VOL:
		{
			float f_bat_vol = 0.0;
			unsigned short i_bat_vol = _data[4];
			i_bat_vol = i_bat_vol << 8;
			i_bat_vol += _data[5];
			f_bat_vol = (3.3/4095)*i_bat_vol;
			//ESP_LOGI(TAG_MCU_COMM, "battery vol:i_bat_vol = %d, f_bat_vol=%.2f", i_bat_vol, f_bat_vol);
			if (g_battery_cb != NULL)
			{
				g_battery_cb(f_bat_vol*2);
			}
			break;
		}
		default:
			break;
	}
}

static void mcu_serial_parse(
	MCU_SERIAL_PARSE_HANDLER_T *_handler, unsigned char *_data, size_t _data_len)
{
	int i = 0;
	
	if (_handler == NULL || _data == NULL || _data_len <= 0)
	{
		return;
	}
	
	for (i=0; i<_data_len; i++)
	{
		//ESP_LOGE(TAG_MCU_COMM, "status:%d byte:%02x\n", _handler->_status, *(_data+i));
		switch (_handler->_status)
		{
			case MCU_SERIAL_COMM_STATUS_HEAD1:
			{
				memset(_handler, 0, sizeof(MCU_SERIAL_PARSE_HANDLER_T));
				_handler->_status = MCU_SERIAL_COMM_STATUS_HEAD1;
				
				if (*(_data+i) == SERIAL_COMM_HEAD)
				{
					_handler->rxd_buf[0] = SERIAL_COMM_HEAD;
					_handler->_status = MCU_SERIAL_COMM_STATUS_HEAD2;
				}
				break;
			}
			case MCU_SERIAL_COMM_STATUS_HEAD2:
			{
				if (*(_data+i) == SERIAL_COMM_HEAD)
				{
					_handler->rxd_buf[1] = SERIAL_COMM_HEAD;
					_handler->_status = MCU_SERIAL_COMM_STATUS_DATA_LEN;
				}
				else
				{
					_handler->_status = MCU_SERIAL_COMM_STATUS_HEAD1;
				}
				break;
			}
			case MCU_SERIAL_COMM_STATUS_DATA_LEN:
			{
				if ((unsigned char)*(_data+i) > 32)
				{
					_handler->_status = MCU_SERIAL_COMM_STATUS_HEAD1;
				}
				else
				{
					_handler->rxd_buf[2] = *(_data+i);
					_handler->_status = MCU_SERIAL_COMM_STATUS_DATA;
				}
				break;
			}
			case MCU_SERIAL_COMM_STATUS_DATA:
			{
				_handler->rxd_buf[3+_handler->recv_count] = *(_data+i);
				_handler->recv_count++;
				if (_handler->recv_count == _handler->rxd_buf[2])
				{
					_handler->_status = MCU_SERIAL_COMM_STATUS_DATA_END;
				}
				break;
			}
			default:
				break;
		}

		if (_handler->_status == MCU_SERIAL_COMM_STATUS_DATA_END)
		{
			if (mcu_serial_check_sum(&_handler->rxd_buf, _handler->rxd_buf[2] + 3))
			{
				mcu_serial_cmd_process(&_handler->rxd_buf, _handler->rxd_buf[2] + 3);
			}

			_handler->_status = MCU_SERIAL_COMM_STATUS_HEAD1;
		}
	}
}

int led_mcu_cmd(LED_CMD_NUM_T cmd_num)
{
	char rxd_buf[SERIAL_RXD_NUM] = {0};
	int  times      = 2;	

	while(times--)
	{
		uart_write_bytes(SERIAL_UART_NUM, &led_exp_cmd[cmd_num], SERIAL_TXD_NUM);
			
		int rx_len = uart_read_bytes(SERIAL_UART_NUM, (uint8_t *)rxd_buf, sizeof(rxd_buf), 10/portTICK_RATE_MS);
		if (rx_len <= 0 || strncmp(rxd_buf, &led_exp_cmd[LED_CMD_NUM_OK], SERIAL_RXD_NUM) != 0) {
			ESP_LOGE(TAG_MCU_COMM, "uart_read_bytes    rx_len = %d", rx_len);
			ESP_LOGE(TAG_MCU_COMM, "led_mcu_cmd fail, cmd num = %d", cmd_num);
			vTaskDelay(50);
			continue;
		} else {
			//ESP_LOGE(TAG_MCU_COMM, "led_mcu_cmd success, cmd num = %d", cmd_num);
			return 0;
		}
	}
	return -1;
}

void mcu_serial_process(void)
{
	mcu_serial_init();
}


void mcu_serial_comm_create(mcu_key_cb _key_cb, mcu_battery_cb _battery_cb)
{
	//g_key_cb = _key_cb;
	//g_battery_cb = _battery_cb;
	xTaskCreate(mcu_serial_process, "mcu_serial_process", 4096, NULL, 10, NULL);
}

void keyboard_lock_init(void)
{
	g_lock_keyborad = xSemaphoreCreateMutex();
}

int keyboard_lock_status(void)
{
	int status = 0 ;
	xSemaphoreTake(g_lock_keyborad, portMAX_DELAY);	
	status = g_keyboard_lock_status;
	xSemaphoreGive(g_lock_keyborad);

	return status;
}


int keyboard_lock(void)
{
	int ret = 0;
	static uint64_t lock_time = 0;
	
	xSemaphoreTake(g_lock_keyborad, portMAX_DELAY);	
	if (g_keyboard_lock_status == 1)
	{
		if (abs(lock_time - get_time_of_day()) > 10000)//10 secs
		{
			lock_time = get_time_of_day();
			ret = 0;
		}
		else
		{
			ret = 1;
		}
	}
	else
	{
		g_keyboard_lock_status  = 1;
		lock_time = get_time_of_day();
		ret = 0;
	}
	xSemaphoreGive(g_lock_keyborad);
	//printf("g_keyboard_lock_status=%d\r\n", g_keyboard_lock_status);
	return ret;
}

void keyboard_unlock(void)
{
	xSemaphoreTake(g_lock_keyborad, portMAX_DELAY);	
	g_keyboard_lock_status = 0;
	xSemaphoreGive(g_lock_keyborad);
	//printf("g_keyboard_lock_status=%d\r\n", g_keyboard_lock_status);
}


