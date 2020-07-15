/*---------------------------------------------------------------------------------------------------------*/
/*                                                                                                         */
/* Copyright(c) 2017 Nuvoton Technology Corp. All rights reserved.                                         */
/*                                                                                                         */
/*---------------------------------------------------------------------------------------------------------*/

//***********************************************************************************************************
//  Website: http://www.nuvoton.com
//  E-Mail : MicroC-8bit@nuvoton.com
//  Date   : Jan/21/2017
//***********************************************************************************************************

//***********************************************************************************************************
//  File Function: N76E003  ADC demo code
//***********************************************************************************************************
#include "N76E003.h"
#include "SFR_Macro.h"
#include "Function_define.h"
#include "Common.h"
#include "Delay.h"

typedef signed short int16_t;

//*****************  The Following is in define in Fucntion_define.h  ***************************
//****** Always include Function_define.h call the define you want, detail see main(void) *******
//***********************************************************************************************
#if 0
//#define Enable_ADC_AIN0			ADCCON0&=0xF0;P17_Input_Mode;AINDIDS=0x00;AINDIDS|=SET_BIT0;ADCCON1|=SET_BIT0									//P17
//#define Enable_ADC_AIN1			ADCCON0&=0xF0;ADCCON0|=0x01;P30_Input_Mode;AINDIDS=0x00;AINDIDS|=SET_BIT1;ADCCON1|=SET_BIT0		//P30
//#define Enable_ADC_AIN2			ADCCON0&=0xF0;ADCCON0|=0x02;P07_Input_Mode;AINDIDS=0x00;AINDIDS|=SET_BIT2;ADCCON1|=SET_BIT0		//P07
//#define Enable_ADC_AIN3			ADCCON0&=0xF0;ADCCON0|=0x03;P06_Input_Mode;AINDIDS=0x00;AINDIDS|=SET_BIT3;ADCCON1|=SET_BIT0		//P06
//#define Enable_ADC_AIN4			ADCCON0&=0xF0;ADCCON0|=0x04;P05_Input_Mode;AINDIDS=0x00;AINDIDS|=SET_BIT4;ADCCON1|=SET_BIT0		//P05
//#define Enable_ADC_AIN5			ADCCON0&=0xF0;ADCCON0|=0x05;P04_Input_Mode;AINDIDS=0x00;AINDIDS|=SET_BIT5;ADCCON1|=SET_BIT0		//P04
//#define Enable_ADC_AIN6			ADCCON0&=0xF0;ADCCON0|=0x06;P03_Input_Mode;AINDIDS=0x00;AINDIDS|=SET_BIT6;ADCCON1|=SET_BIT0		//P03
//#define Enable_ADC_AIN7			ADCCON0&=0xF0;ADCCON0|=0x07;P11_Input_Mode;AINDIDS=0x00;AINDIDS|=SET_BIT7;ADCCON1|=SET_BIT0		//P11
//#define Enable_ADC_BandGap	ADCCON0|=SET_BIT3;ADCCON0&=0xF8;																															//Band-gap 1.22V

//#define PWM0_FALLINGEDGE_TRIG_ADC		ADCCON0&=~SET_BIT5;ADCCON0&=~SET_BIT4;ADCCON1&=~SET_BIT3;ADCCON1&=~SET_BIT2;ADCCON1|=SET_BIT1
//#define PWM2_FALLINGEDGE_TRIG_ADC		ADCCON0&=~SET_BIT5;ADCCON0|=SET_BIT4;ADCCON1&=~SET_BIT3;ADCCON1&=~SET_BIT2;ADCCON1|=SET_BIT1
//#define PWM4_FALLINGEDGE_TRIG_ADC		ADCCON0|=SET_BIT5;ADCCON0&=~SET_BIT4;ADCCON1&=~SET_BIT3;ADCCON1&=~SET_BIT2;ADCCON1|=SET_BIT1
//#define PWM0_RISINGEDGE_TRIG_ADC		ADCCON0&=~SET_BIT5;ADCCON0&=~SET_BIT4;ADCCON1&=~SET_BIT3;ADCCON1|=SET_BIT2;ADCCON1|=SET_BIT1
//#define PWM2_RISINGEDGE_TRIG_ADC		ADCCON0&=~SET_BIT5;ADCCON0|=SET_BIT4;ADCCON1&=~SET_BIT3;ADCCON1|=SET_BIT2;ADCCON1|=SET_BIT1
//#define PWM4_RISINGEDGE_TRIG_ADC		ADCCON0|=SET_BIT5;ADCCON0&=~SET_BIT4;ADCCON1&=~SET_BIT3;ADCCON1|=SET_BIT2;ADCCON1|=SET_BIT1

//#define P04_FALLINGEDGE_TRIG_ADC		ADCCON0|=0x30;ADCCON1&=0xF3;ADCCON1|=SET_BIT1;ADCCON1&=~SET_BIT6
//#define P13_FALLINGEDGE_TRIG_ADC		ADCCON0|=0x30;ADCCON1&=0xF3;ADCCON1|=SET_BIT1;ADCCON1|=SET_BIT6
//#define P04_RISINGEDGE_TRIG_ADC			ADCCON0|=0x30;ADCCON1&=~SET_BIT3;ADCCON1|=SET_BIT2;ADCCON1|=SET_BIT1;ADCCON1&=~SET_BIT6
//#define P13_RISINGEDGE_TRIG_ADC			ADCCON0|=0x30;ADCCON1&=~SET_BIT3;ADCCON1|=SET_BIT2;ADCCON1|=SET_BIT1;ADCCON1|=SET_BIT6
#endif

/*
串口通讯协议设计:
包头(2字节) +     数据长度（1字节，包括命令+数据+校验和）+ 命令码(1字节)+数据(n字节)+校验和(1字节)
包头：0x55 0x55
*/
#define SERIAL_COMM_HEAD 0x55

#define PRINT_ADC_VALUE 1

#define KEY_MATRIX_IDEL_VOLTAGE			(4095)
#define KEY_MATRIX_CHAT_VOLTAGE			(3700)	
#define KEY_MATRIX_LED_VOLTAGE			(2440)	
#define KEY_MATRIX_VOL_UP_VOLTAGE		(950)	
#define KEY_MATRIX_VOL_DOWN_VOLTAGE		(0)		
#define KEY_MATRIX_MENU_VOLTAGE			(3385)	
#define KEY_MATRIX_PREV_VOLTAGE			(1560)	
#define KEY_MATRIX_PLAY_VOLTAGE			(3120)	
#define KEY_MATRIX_NEXT_VOLTAGE			(1950)	
#define KEY_MATRIX_STORY_VOLTAGE		(2860)	
#define KEY_MATRIX_FW_UPDATE_VOLTAGE	(1100)
#define KEY_MATRIX_VOLTAGE_ERROR 		(125)//0.1V
#define KEY_MATRIX_VOLTAGE_RANGE(a,b) ((a >= ((b)-KEY_MATRIX_VOLTAGE_ERROR)) && (a <= ((b)+KEY_MATRIX_VOLTAGE_ERROR)))
#define KEY_LONG_PRESS_MS (3*1000)	//3秒
#define KEY_SHORT_PRESS_MS 500		//500ms
#define ADC_BATTERY_COUNT 500

typedef enum
{
	SERIAL_COMM_CMD_KEY = 0x01,
	SERIAL_COMM_CMD_BATTERY_VOL,
	SERIAL_COMM_CMD_START = 0x50,
	SERIAL_COMM_CMD_SLEEP,
}SERIAL_COMM_CMD_T;

typedef enum
{
	KEY_MATRIX_IDEL=0,	//idel
	KEY_MATRIX_CHAT,	//wechat	 	
	KEY_MATRIX_LED,		//led on/off	
	KEY_MATRIX_VOL_UP,	//voice up	
	KEY_MATRIX_VOL_DOWN,//voice down	
	KEY_MATRIX_MENU,	//menu	 	
	KEY_MATRIX_PREV,	//music preview 	
	KEY_MATRIX_PLAY,	//music play/pause
	KEY_MATRIX_NEXT,	//music next	 	
	KEY_MATRIX_STORY,	//story inside chip	
	KEY_MATRIX_FW_UPDATE//fireware update, press prev and next at the same time
}KEY_MATRIX_NUM_T;


typedef enum 
{
    KEY_MATRIX_EVENT_TAP = 0,
	KEY_MATRIX_EVENT_PUSH,
	KEY_MATRIX_EVENT_RELEASE,
    KEY_MATRIX_EVENT_LONG_PRESSED
}KEY_MATRIX_EVENT_T;

typedef enum  
{
	DEVICE_NOTIFY_KEY_START = 0,
	DEVICE_NOTIFY_KEY_MATRIX_CHAT_TAP,
	DEVICE_NOTIFY_KEY_MATRIX_CHAT_PUSH,
	DEVICE_NOTIFY_KEY_MATRIX_CHAT_RELEASE,
	DEVICE_NOTIFY_KEY_MATRIX_CHAT_LONGPRESSED,

	DEVICE_NOTIFY_KEY_MATRIX_LED_TAP,
	DEVICE_NOTIFY_KEY_MATRIX_LED_PUSH,
	DEVICE_NOTIFY_KEY_MATRIX_LED_RELEASE,
	DEVICE_NOTIFY_KEY_MATRIX_LED_LONGPRESSED,

	DEVICE_NOTIFY_KEY_MATRIX_VOL_UP_TAP,
	DEVICE_NOTIFY_KEY_MATRIX_VOL_UP_PUSH,
	DEVICE_NOTIFY_KEY_MATRIX_VOL_UP_RELEASE,
	DEVICE_NOTIFY_KEY_MATRIX_VOL_UP_LONGPRESSED,

	DEVICE_NOTIFY_KEY_MATRIX_VOL_DOWN_TAP,
	DEVICE_NOTIFY_KEY_MATRIX_VOL_DOWN_PUSH,
	DEVICE_NOTIFY_KEY_MATRIX_VOL_DOWN_RELEASE,
	DEVICE_NOTIFY_KEY_MATRIX_VOL_DOWN_LONGPRESSED,

	DEVICE_NOTIFY_KEY_MATRIX_MENU_TAP,
	DEVICE_NOTIFY_KEY_MATRIX_MENU_PUSH,
	DEVICE_NOTIFY_KEY_MATRIX_MENU_RELEASE,
	DEVICE_NOTIFY_KEY_MATRIX_MENU_LONGPRESSED,

	DEVICE_NOTIFY_KEY_MATRIX_PREV_TAP,
	DEVICE_NOTIFY_KEY_MATRIX_PREV_PUSH,
	DEVICE_NOTIFY_KEY_MATRIX_PREV_RELEASE,
	DEVICE_NOTIFY_KEY_MATRIX_PREV_LONGPRESSED,

	DEVICE_NOTIFY_KEY_MATRIX_PLAY_TAP,
	DEVICE_NOTIFY_KEY_MATRIX_PLAY_PUSH,
	DEVICE_NOTIFY_KEY_MATRIX_PLAY_RELEASE,
	DEVICE_NOTIFY_KEY_MATRIX_PLAY_LONGPRESSED,

	DEVICE_NOTIFY_KEY_MATRIX_NEXT_TAP,
	DEVICE_NOTIFY_KEY_MATRIX_NEXT_PUSH,
	DEVICE_NOTIFY_KEY_MATRIX_NEXT_RELEASE,
	DEVICE_NOTIFY_KEY_MATRIX_NEXT_LONGPRESSED,

	DEVICE_NOTIFY_KEY_MATRIX_STORY_TAP,
	DEVICE_NOTIFY_KEY_MATRIX_STORY_PUSH,
	DEVICE_NOTIFY_KEY_MATRIX_STORY_RELEASE,
	DEVICE_NOTIFY_KEY_MATRIX_STORY_LONGPRESSED,

	DEVICE_NOTIFY_KEY_MATRIX_FW_UPDATE_TAP,
	DEVICE_NOTIFY_KEY_MATRIX_FW_UPDATE_PUSH,
	DEVICE_NOTIFY_KEY_MATRIX_FW_UPDATE_RELEASE,
	DEVICE_NOTIFY_KEY_MATRIX_FW_UPDATE_LONGPRESSED,
	
	DEVICE_NOTIFY_KEY_END,
}DEVICE_NOTIFY_KEY_T;

typedef struct {
	uint8_t	curr_key_num;	//current key num
	uint8_t last_key_num;	//last key num
	uint32_t key_hold_ms;	//key press hold time
	uint8_t key_hold_flag;	//
} key_matrix_t;


/******************************************************************************
The main C function.  Program execution starts
here after stack initialization.
******************************************************************************/

//获取键盘电压值
int16_t get_adc_keyborad_value(void)
{
	int16_t adc_value = 0;
	
	Enable_ADC_AIN0;// Enable AIN0 P1.7 as ADC input
	clr_ADCF;
	set_ADCS;									// ADC start trig signal
	while(ADCF == 0);
	adc_value = ADCRH;
	adc_value = adc_value << 4 | ADCRL;
#if PRINT_ADC_VALUE == 1
	//printf ("keyborad:%d\n", adc_value);
	//printf ("%bx%bx\n", ADCRH,ADCRL);
#endif

	return adc_value;
}

//获取电池电压值
int16_t get_adc_battery_value(void)
{
	int16_t adc_value = 0;
	
	Enable_ADC_AIN1;// Enable AIN1 P3.0 as ADC input
	clr_ADCF;
	set_ADCS;									// ADC start trig signal
	while(ADCF == 0);
	adc_value = ADCRH;
	adc_value = adc_value << 4 | ADCRL;
#if PRINT_ADC_VALUE == 1
	//printf ("battery:%d\n", adc_value);
	//printf ("%bx%bx\n", ADCRH,ADCRL);
#endif

	return adc_value;
}

void uart_send_data(uint8_t _cmd, uint8_t *_data, uint8_t _data_len)
{
	uint8_t i = 0;
	uint8_t sum = 0;
	
	Send_Data_To_UART0(SERIAL_COMM_HEAD);
	sum += SERIAL_COMM_HEAD;
	Send_Data_To_UART0(SERIAL_COMM_HEAD);
	sum += SERIAL_COMM_HEAD;
	Send_Data_To_UART0(_data_len+2);
	sum += _data_len+2;
	Send_Data_To_UART0(_cmd);
	sum += _cmd;
	for (i=0; i<_data_len; i++)
	{
		Send_Data_To_UART0(*(_data+i));
		sum += *(_data+i);
	}
	Send_Data_To_UART0(sum);
}

void uart_send_key_cmd(
	uint8_t _key_num, uint8_t _key_event)
{	
	uint8_t event = 0;
	switch (_key_num)
	{
		case KEY_MATRIX_CHAT:	//wechat		
		{
			event = DEVICE_NOTIFY_KEY_MATRIX_CHAT_TAP;
			break;
		}
		case KEY_MATRIX_LED:		//led on/off	
		{
			event = DEVICE_NOTIFY_KEY_MATRIX_LED_TAP;
			break;
		}
		case KEY_MATRIX_VOL_UP: //voice up	
		{
			event = DEVICE_NOTIFY_KEY_MATRIX_VOL_UP_TAP;
			break;
		}
		case KEY_MATRIX_VOL_DOWN://voice down	
		{
			event = DEVICE_NOTIFY_KEY_MATRIX_VOL_DOWN_TAP;
			break;
		}
		case KEY_MATRIX_MENU:	//menu		
		{
			event = DEVICE_NOTIFY_KEY_MATRIX_MENU_TAP;
			break;
		}
		case KEY_MATRIX_PREV:	//music preview 	
		{
			event = DEVICE_NOTIFY_KEY_MATRIX_PREV_TAP;
			break;
		}
		case KEY_MATRIX_PLAY:	//music play/pause
		{
			event = DEVICE_NOTIFY_KEY_MATRIX_PLAY_TAP;
			break;
		}
		case KEY_MATRIX_NEXT:	//music next		
		{
			event = DEVICE_NOTIFY_KEY_MATRIX_NEXT_TAP;
			break;
		}
		case KEY_MATRIX_STORY:	//story inside chip 
		{
			event = DEVICE_NOTIFY_KEY_MATRIX_STORY_TAP;
			break;
		}
		case KEY_MATRIX_FW_UPDATE://fireware update, press prev and next at the same time
		{
			event = DEVICE_NOTIFY_KEY_MATRIX_FW_UPDATE_TAP;
			break;
		}
		default:
			return;
			break;
	}

	switch (_key_event)
	{
		case KEY_MATRIX_EVENT_TAP:
		{
			break;
		}
		case KEY_MATRIX_EVENT_PUSH:
		{
			event += KEY_MATRIX_EVENT_PUSH - KEY_MATRIX_EVENT_TAP;
			break;
		}
		case KEY_MATRIX_EVENT_RELEASE:
		{
			event += KEY_MATRIX_EVENT_RELEASE - KEY_MATRIX_EVENT_TAP;
			break;
		}
		case KEY_MATRIX_EVENT_LONG_PRESSED:
		{
			event += KEY_MATRIX_EVENT_LONG_PRESSED - KEY_MATRIX_EVENT_TAP;
			break;
		}
		default:
			return;
			break;
	}
	uart_send_data(SERIAL_COMM_CMD_KEY, &event, 1);
}


void adc_keyborad_process(key_matrix_t *_key_matrix)
{	
	int16_t adc_keyboard_value = get_adc_keyborad_value();
	
	if (KEY_MATRIX_VOLTAGE_RANGE(adc_keyboard_value, KEY_MATRIX_IDEL_VOLTAGE))
	{
		//printf("keyboard matrix:KEY_MATRIX_IDEL\n");
		_key_matrix->curr_key_num = KEY_MATRIX_IDEL; 
	}
	else if (KEY_MATRIX_VOLTAGE_RANGE(adc_keyboard_value, KEY_MATRIX_CHAT_VOLTAGE))
	{
		//printf("keyboard matrix:KEY_MATRIX_CHAT\n");
		_key_matrix->curr_key_num = KEY_MATRIX_CHAT; 
	}
	else if (KEY_MATRIX_VOLTAGE_RANGE(adc_keyboard_value, KEY_MATRIX_LED_VOLTAGE))
	{
		//printf("keyboard matrix:KEY_MATRIX_LED\n");
		_key_matrix->curr_key_num = KEY_MATRIX_LED;
	}
	else if (KEY_MATRIX_VOLTAGE_RANGE(adc_keyboard_value, KEY_MATRIX_VOL_UP_VOLTAGE))
	{
		//printf("keyboard matrix:KEY_MATRIX_VOL_UP\n");
		_key_matrix->curr_key_num = KEY_MATRIX_VOL_UP; 
	}
	else if (KEY_MATRIX_VOLTAGE_RANGE(adc_keyboard_value, KEY_MATRIX_VOL_DOWN_VOLTAGE))
	{
		//printf("keyboard matrix:KEY_MATRIX_VOL_DOWN\n");
		_key_matrix->curr_key_num = KEY_MATRIX_VOL_DOWN;
	}
	else if (KEY_MATRIX_VOLTAGE_RANGE(adc_keyboard_value, KEY_MATRIX_MENU_VOLTAGE))
	{
		//printf("keyboard matrix:KEY_MATRIX_MENU\n");
		_key_matrix->curr_key_num = KEY_MATRIX_MENU;
	}
	else if (KEY_MATRIX_VOLTAGE_RANGE(adc_keyboard_value, KEY_MATRIX_PREV_VOLTAGE))
	{
		//printf("keyboard matrix:KEY_MATRIX_PREV\n");
		_key_matrix->curr_key_num = KEY_MATRIX_PREV;
	}
	else if (KEY_MATRIX_VOLTAGE_RANGE(adc_keyboard_value, KEY_MATRIX_PLAY_VOLTAGE))
	{
		//printf("keyboard matrix:KEY_MATRIX_PLAY\n");
		_key_matrix->curr_key_num = KEY_MATRIX_PLAY; 
	}
	else if (KEY_MATRIX_VOLTAGE_RANGE(adc_keyboard_value, KEY_MATRIX_NEXT_VOLTAGE))
	{
		//printf("keyboard matrix:KEY_MATRIX_NEXT\n");
		_key_matrix->curr_key_num = KEY_MATRIX_NEXT; 
	}
	else if (KEY_MATRIX_VOLTAGE_RANGE(adc_keyboard_value, KEY_MATRIX_STORY_VOLTAGE))
	{
		//printf("keyboard matrix:KEY_MATRIX_STORY\n");
		_key_matrix->curr_key_num = KEY_MATRIX_STORY; 
	}
	else if (KEY_MATRIX_VOLTAGE_RANGE(adc_keyboard_value, KEY_MATRIX_FW_UPDATE_VOLTAGE))
	{
		//printf("keyboard matrix:KEY_MATRIX_FW_UPDATE\n");
		_key_matrix->curr_key_num = KEY_MATRIX_FW_UPDATE;
	}
	else
	{
		goto adc_keyborad_process_error;
	}

	_key_matrix->key_hold_ms += 10;
	if (_key_matrix->curr_key_num == _key_matrix->last_key_num)
	{
		if (_key_matrix->curr_key_num != KEY_MATRIX_IDEL)
		{
			if (_key_matrix->key_hold_ms >= KEY_LONG_PRESS_MS && _key_matrix->key_hold_flag == 0)
			{
				uart_send_key_cmd(_key_matrix->curr_key_num, KEY_MATRIX_EVENT_LONG_PRESSED);
				_key_matrix->key_hold_flag = 1;
			}
		}
	}
	else
	{	
		//key press flow, KEY_MATRIX_IDEL -> any key -> KEY_MATRIX_IDEL
		switch (_key_matrix->curr_key_num)
		{
			case KEY_MATRIX_IDEL:
			{
				uart_send_key_cmd(_key_matrix->last_key_num, KEY_MATRIX_EVENT_RELEASE);
				
				if (_key_matrix->key_hold_ms <= KEY_SHORT_PRESS_MS)
				{
					uart_send_key_cmd(_key_matrix->last_key_num, KEY_MATRIX_EVENT_TAP);
				}
				else if (_key_matrix->key_hold_ms >= KEY_LONG_PRESS_MS && _key_matrix->key_hold_flag == 0)
				{
					uart_send_key_cmd(_key_matrix->last_key_num, KEY_MATRIX_EVENT_LONG_PRESSED);
				}
				_key_matrix->key_hold_flag = 0;
				break;
			}
			case KEY_MATRIX_CHAT:
			case KEY_MATRIX_LED:	
			case KEY_MATRIX_VOL_UP:
			case KEY_MATRIX_VOL_DOWN:
			case KEY_MATRIX_MENU:
			case KEY_MATRIX_PREV: 	
			case KEY_MATRIX_PLAY:
			case KEY_MATRIX_NEXT: 	
			case KEY_MATRIX_STORY:
			case KEY_MATRIX_FW_UPDATE:
			{
				uart_send_key_cmd(_key_matrix->curr_key_num, KEY_MATRIX_EVENT_PUSH);
				break;
			}
			default:
				break;
		}
		_key_matrix->key_hold_ms = 0;
		_key_matrix->last_key_num = _key_matrix->curr_key_num;
	}
	
adc_keyborad_process_error:

	return;
}

void adc_battery_vol_process(void)
{
	static uint32_t count = 0;
	static uint32_t adc_battery_value = 0;
	uint16_t adc_battery_vol = 0;

	adc_battery_value += get_adc_battery_value();
	count++;
	if (count == ADC_BATTERY_COUNT)
	{
		adc_battery_vol = adc_battery_value/ ADC_BATTERY_COUNT;
		uart_send_data(SERIAL_COMM_CMD_BATTERY_VOL, (uint8_t*)&adc_battery_vol, 2);
		count = 0;
		adc_battery_value = 0;
	}
}

void main (void) 
{
	key_matrix_t key_matrix = {0};
	
	//串口初始化
	InitialUART0_Timer1(115200);
	
	while(1)
	{
		adc_keyborad_process(&key_matrix);
		adc_battery_vol_process();
		Timer0_Delay1ms(10);
	}
}


