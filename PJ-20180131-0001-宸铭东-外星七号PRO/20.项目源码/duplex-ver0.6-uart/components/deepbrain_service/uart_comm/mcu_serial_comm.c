#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "driver/uart.h"
#include "freertos/queue.h"
#include "esp_log.h"
#include "soc/uart_struct.h"
#include "mcu_serial_comm.h"
#include "led.h"
#include "userconfig.h"
#include "keyboard_manage.h"
#include "keyboard_service.h"
#include "device_api.h"

#define TAG_MCU_COMM "MCU_COMM"

#define SERIAL_COMM_HEAD1 0xff
#define SERIAL_COMM_HEAD2 0x55

typedef enum
{
	MCU_SERIAL_COMM_STATUS_HEAD1 = 0,
	MCU_SERIAL_COMM_STATUS_HEAD2,
	MCU_SERIAL_COMM_STATUS_DATA_LEN,
	MCU_SERIAL_COMM_STATUS_DATA,
	MCU_SERIAL_COMM_STATUS_CHECKSUM,
	MCU_SERIAL_COMM_STATUS_DATA_END,
}MCU_SERIAL_PARSE_STATUS_T;

typedef enum
{
	SERIAL_COMM_CMD_KEY = 0x01,
	SERIAL_COMM_CMD_ESP32_STATUS_MODE=0x0B,
}SERIAL_COMM_CMD_T;

typedef struct
{
	MCU_SERIAL_PARSE_STATUS_T _status;
	int recv_count;
	unsigned char rxd_buf[128];
}UART_COMM_PARSE_t;

typedef struct
{
	int uart_num;
	UART_COMM_PARSE_t parse_handle;
	uint8_t recv_buff[128];
	uint8_t send_buff[128];
}UART_COMM_HANDLE_t;

SemaphoreHandle_t g_mutex_mcu_send = NULL;
UART_COMM_HANDLE_t * g_uart_comm_handle = NULL;
int g_mic_flag = 0;

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

void mcu_serial_init(int uart_num, int io_rx, int io_tx)
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
	uart_param_config(UART_NUM_1, &uart_config);
	//Set UART1 pins(TX: IO4, RX: I05, RTS: IO18, CTS: IO19)
	uart_set_pin(UART_NUM_1, io_tx, io_rx, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
	//Install UART driver (we don't need an event queue here)
	//In this example we don't even use a buffer for sending data.
	uart_driver_install(UART_NUM_1, 512, 512, 0, NULL, 0);
}


static void mcu_serial_cmd_process(unsigned char *_data, size_t _data_len)
{
	SERIAL_COMM_CMD_T cmd = _data[3];
	static int wechat_flag = 0;
	static int esp32_flag = 0;
	
	switch (cmd)
	{
		case SERIAL_COMM_CMD_KEY:
		{
			switch (_data[4])
			{
				case KEY_VALUE_PREV:
				{
					printf("in MCU KEY_VALUE_PREV\n");
					key_matrix_callback(DEVICE_KEY_PREV_START);
					break;
				}
				case KEY_VALUE_PLAY:
				{
					printf("in MCU KEY_VALUE_PLAY\n");
					key_matrix_callback(DEVICE_KEY_PLAY_START);
					break;
				}
				case KEY_VALUE_NEXT: 
				{
					printf("in MCU KEY_VALUE_NEXT\n");
					key_matrix_callback(DEVICE_KEY_NEXT_START);
					break;
				}
				case KEY_VALUE_VOL_DOWN: 
				{
					printf("in MCU KEY_VALUE_VOL_DOWN\n");
					key_matrix_callback(DEVICE_KEY_VOL_DOWN_START);
					break;
				}
				case KEY_VALUE_VOL_UP:
				{
					printf("in MCU KEY_VALUE_VOL_UP\n");
					key_matrix_callback(DEVICE_KEY_VOL_UP_START);
					break;
				}
				case KEY_VALUE_WECHAT_START:
				{
					if (wechat_flag == 0)
					{
						printf("in MCU KEY_VALUE_WECHAT_START\n");
						wechat_flag = 1;
						key_matrix_callback(DEVICE_KEY_WECHAT_START);
					}
					break;
				}
				case KEY_VALUE_WECHAT_STOP:
				{
					printf("in MCU KEY_VALUE_WECHAT_STOP\n");
					wechat_flag = 0;
					key_matrix_callback(DEVICE_KEY_WECHAT_STOP);
					break;
				}
				case KEY_VALUE_CHAT_START:
				{
					printf("in MCU KEY_VALUE_CHAT_START\n");
					key_matrix_callback(DEVICE_KEY_CHAT_START);
					break;
				}
				case KEY_VALUE_CHAT_STOP:
				{
					printf("in MCU KEY_VALUE_CHAT_STOP\n");
					key_matrix_callback(DEVICE_KEY_CHAT_STOP);
					break;
				}
				case KEY_VALUE_WIFI_SET:
				{
					printf("in MCU KEY_VALUE_WIFI_SET\n");
					key_matrix_callback(DEVICE_KEY_WIFI_SET_START);
					break;
				}
				case KEY_VALUE_CH2EN:
				{
					printf("in MCU KEY_VALUE_CH2EN\n");
					key_matrix_callback(DEVICE_KEY_CH2EN_START);
					break;
				}
				case KEY_VALUE_EN2CH:
				{
					printf("in MCU KEY_VALUE_EN2CH\n");
					key_matrix_callback(DEVICE_KEY_EN2CH_START);
					break;
				}
				case KEY_VALUE_ESP32_START:
				{
					printf("in MCU KEY_VALUE_ESP32_START\n");
					if (esp32_flag == 1)//防止esp32开启状态下再次启动进入
					{
						key_matrix_callback(DEVICE_KEY_ESP32_START);
						esp32_flag = 0;
					}
					break;
				}
				case KEY_VALUE_ESP32_STOP:
				{
					printf("in MCU KEY_VALUE_ESP32_STOP\n");
					key_matrix_callback(DEVICE_KEY_ESP32_STOP);
					esp32_flag = 1;
					break;
				}
				case KEY_VALUE_COLLECTION:
				{
					printf("in MCU KEY_VALUE_COLLECTION\n");
					key_matrix_callback(DEVICE_KEY_COLLECTION_START);
					break;
				}
				case KEY_VALUE_PLAY_COLLECTION:
				{
					printf("in MCU KEY_VALUE_PLAY_COLLECTION\n");
					key_matrix_callback(DEVICE_KEY_PLAY_COLLECTION_START);
					break;
				}
				case KEY_VALUE_PLAY_NEWS:
				{
					printf("in MCU KEY_VALUE_PLAY_NEWS\n");
					key_matrix_callback(DEVICE_KEY_PLAY_NEWS_START);
					break;
				}
				case KEY_VALUE_LOW_POWER:
				{
					printf("in MCU KEY_VALUE_LOW_POWER\n");
					key_matrix_callback(DEVICE_KEY_LOW_POWER);
					break;
				}
				case KEY_VALUE_ROBOT_OFF:
				{
					printf("in MCU KEY_VALUE_ROBOT_OFF\n");
					key_matrix_callback(DEVICE_KEY_ROBOT_OFF);
					break;
				}
				case KEY_VALUE_MIC_IN:
				{
					printf("in MCU KEY_VALUE_MIC_IN\n");
					key_matrix_callback(DEVICE_KEY_MIC_IN);
					break;
				}
				case KEY_VALUE_MIC_OUT:
				{
					printf("in MCU KEY_VALUE_MIC_OUT\n");
					key_matrix_callback(DEVICE_KEY_MIC_OUT);
					break;
				}
				default:
					break;
			}
			break;
		}
		default:
			break;
	}
}

static void mcu_serial_parse(
	UART_COMM_PARSE_t *_handler, unsigned char *_data, size_t _data_len)
{
	int i = 0;
	
	if (_handler == NULL || _data == NULL || _data_len <= 0)
	{
		return;
	}
	
	for (i=0; i<_data_len; i++)
	{
		ESP_LOGE(TAG_MCU_COMM, "status:%d byte:%02x\n", _handler->_status, *(_data+i));
		switch (_handler->_status)
		{
			case MCU_SERIAL_COMM_STATUS_HEAD1:
			{
				memset(_handler, 0, sizeof(UART_COMM_PARSE_t));
				_handler->_status = MCU_SERIAL_COMM_STATUS_HEAD1;
				
				if (*(_data+i) == SERIAL_COMM_HEAD1)
				{
					_handler->rxd_buf[0] = SERIAL_COMM_HEAD1;
					_handler->_status = MCU_SERIAL_COMM_STATUS_HEAD2;
				}
				break;
			}
			case MCU_SERIAL_COMM_STATUS_HEAD2:
			{
				if (*(_data+i) == SERIAL_COMM_HEAD2)
				{
					_handler->rxd_buf[1] = SERIAL_COMM_HEAD2;
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
					_handler->_status = MCU_SERIAL_COMM_STATUS_CHECKSUM;
				}
				break;
			}
			case MCU_SERIAL_COMM_STATUS_CHECKSUM:
			{
				_handler->recv_count++;
				_handler->rxd_buf[3+_handler->recv_count] = *(_data+i);
				_handler->_status = MCU_SERIAL_COMM_STATUS_DATA_END;
				break;
			}
			default:
				break;
		}

		if (_handler->_status == MCU_SERIAL_COMM_STATUS_DATA_END)
		{
			//if (mcu_serial_check_sum(&_handler->rxd_buf, _handler->rxd_buf[2] + 3))
			{
				mcu_serial_cmd_process(&_handler->rxd_buf, _handler->rxd_buf[2] + 3);
			}

			_handler->_status = MCU_SERIAL_COMM_STATUS_HEAD1;
		}
	}
}

void mcu_serial_send_data(uint8_t cmd, uint8_t *data, uint8_t data_len)
{
	if (g_uart_comm_handle  == NULL)
	{
		return;
	}
	
	int i = 0;
	int send_len = 0;
	char *rxd_buf = &g_uart_comm_handle->send_buff;
	uint8_t check_sum = 0;
	
	xSemaphoreTake(g_mutex_mcu_send, portMAX_DELAY);
	rxd_buf[0] = SERIAL_COMM_HEAD1;
	rxd_buf[1] = SERIAL_COMM_HEAD2;
	rxd_buf[2] = data_len + 1;
	rxd_buf[3] = cmd;
	send_len = 4;
	if (data_len > 0)
	{
		memcpy(&rxd_buf[4], data, data_len);
	}
	send_len += data_len;
	for (i=2; i<send_len; i++)
	{
		check_sum -= rxd_buf[i];
	}
	rxd_buf[send_len] = check_sum;
	send_len++;

	printf("uart send: ");
	for (i=0; i<send_len; i++)
	{
		printf("%02x ", rxd_buf[i]);
	}
	printf("\r\n");
	
	int len = uart_write_bytes(UART_NUM_1, rxd_buf, send_len);
	if (len != send_len)
	{
		ESP_LOGE(TAG_MCU_COMM, "mcu_serial_send_data fail(%d)", len);
	}
	xSemaphoreGive(g_mutex_mcu_send);
}

void uart_esp32_status_mode(UART_ESP32_STATUS_MODE_t mode)
{
	uint8_t esp32_status_mode = mode;
	mcu_serial_send_data(SERIAL_COMM_CMD_ESP32_STATUS_MODE, &esp32_status_mode, 1);
}

static void task_uart_recv(void *pv)
{
	UART_COMM_HANDLE_t *uart_handle = (UART_COMM_HANDLE_t *)pv; 
	uint32_t sleep_time = 20;
	int len = 0;
	UART_COMM_PARSE_t parse_handle = {0};
	while (1)
	{		
		len = uart_read_bytes(UART_NUM_1, uart_handle->recv_buff, sizeof(uart_handle->recv_buff), sleep_time / portTICK_RATE_MS);
		if (len > 0)
		{
			printf("uart_read_bytes len = %d\n", len);
			mcu_serial_parse(&parse_handle, uart_handle->recv_buff, len);
		}
#if 0
		else
		{
			uart_handle->recv_buff[0] = 0xff;
			uart_handle->recv_buff[1] = 0x55;
			uart_handle->recv_buff[2] = 0x02;
			uart_handle->recv_buff[3] = 0x01;
			uart_handle->recv_buff[4] = 0xA5;
			uart_handle->recv_buff[5] = 0x58;
			mcu_serial_parse(&parse_handle, uart_handle->recv_buff, 6);
		}
#endif
	}
	vTaskDelete(NULL);
}

void mcu_serial_comm_create(int uart_num, int io_rx, int io_tx)
{
	g_uart_comm_handle = (UART_COMM_HANDLE_t*)esp32_malloc(sizeof(UART_COMM_HANDLE_t));
	if (g_uart_comm_handle == NULL)
	{
		return;
	}
	memset(g_uart_comm_handle, 0 , sizeof(UART_COMM_HANDLE_t));
	g_mutex_mcu_send = xSemaphoreCreateMutex();
	g_uart_comm_handle->uart_num = uart_num;
	
	mcu_serial_init(uart_num, io_rx, io_tx);
	xTaskCreate(task_uart_recv, "task_uart_recv", 2048, &g_mutex_mcu_send, 24, NULL);
}

