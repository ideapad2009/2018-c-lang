#ifndef __USER_CONFIG_H__
#define __USER_CONFIG_H__

#include "sdkconfig.h"

#define SD_CARD_OPEN_FILE_NUM_MAX   5 //define how many files can be opened simultaneously
#define DEVICE_SN_MAX_LEN 18

#define IDF_3_0             0
#define WROAR_BOARD         1
#define DIFFERENTIAL_MIC    0
#define TEST_PLAYER         0
#define EN_STACK_TRACKER    0 //STACK_TRACKER can track every task stack and print the info

/******************************************************************
//设备信息，包括序列号前缀、版本号、账号等
*******************************************************************/
#define DEVICE_SN_PREFIX	"DQMQ"
#define ESP32_FW_VERSION	"V1.1.2build20180816"
#define DEEP_BRAIN_APP_ID 	"aadbee5cf0ec11e79430801844e30cac"
#define DEEP_BRAIN_ROBOT_ID "2bc0400a-f0f4-11e7-9430-801844e30cac"

/******************************************************************
//代码中所有用到的服务器地址
*******************************************************************/
//百度语音识别URL
#define BAIDU_DEFAULT_ASR_URL		"http://vop.baidu.com/server_api"
#define BAIDU_DEFAULT_TTS_URL		"http://tsn.baidu.com/text2audio"
#define BAIDU_DEFAULT_APP_ID		"10734778"
#define BAIDU_DEFAULT_APP_KEY		"LDxCKuuug7qolGBUBWqecR0p"
#define BAIDU_DEFAULT_SECRET_KEY	"BAiGnLSxqGeKrg2f2HW7GCn7Nm8NoeTO"

//讯飞语音识别
#define XFL2W_DEFAULT_ASR_URL		"http://api.xfyun.cn/v1/service/v1/iat"
#define XFL2W_DEFAULT_APP_ID		"5ad6b053"
#define XFL2W_DEFAULT_API_KEY		"71278aebbe262a054e24b85395256916"

//捷通华声语音识别URL
#define SINOVOICE_DEFAULT_ASR_URL	"http://api.hcicloud.com:8880/asr/recognise"
#define SINOVOICE_DEFAULT_TTS_URL	"http://api.hcicloud.com:8880/tts/SynthText"
#define SINOVOICE_DEFAULT_APP_KEY	"745d54c3"
#define SINOVOICE_DEFAULT_DEV_KEY	"ba1a617ff498135a324a109c89db2823"

//云知声账号
#define UNISOUND_DEFAULT_APP_KEY	"gnhsr6flozejxxn4floxbny6nudx5dsmlhilo3qk"
#define UNISOUND_DEFAULT_USER_ID	"YZS15021927852069956"

//OTA升级服务器地址
#define OTA_UPDATE_SERVER_URL		"http://file.yuyiyun.com:2088/ota/PJ-20171113-0001/version.txt"
//OTA升级服务器测试地址
#define OTA_UPDATE_SERVER_URL_TEST	"http://file.yuyiyun.com:2088/ota/test/PJ-20171113-0001/version.txt"
//deepbrain open api 地址
#define DeepBrain_TEST_URL          "http://api.deepbrain.ai:8383/open-api/service"

/******************************************************************
//产品功能模块控制
*******************************************************************/
#define MODULE_DEEPBRAIN					1
#define MODULE_ADC_KEYBOARD					0
#define MODULE_MCU_SERIAL_COMM				1
#define MODULE_DEVICE_BIND					1 
#define MODULE_EXTRATERRESTRIAL_SEVEN_CASE	1
#define MODULE_CHECK_CHARGING_STATE			1
#define MODULE_LOW_POWER_MANAGE				1
#define MODULE_DEEP_SLEEP_MANAGE			1
#define MODULE_MPUSH						1
#define MODULE_DAC_DEFFERENT_OUT			0//差分输出
#define DEBUG_RECORD_PCM 					0//record pcm data, used to debug pcm
#define PRINT_USER_EXPERIENCE_TEST_DATE		0//用户体验测试数据打印开关
#define MODULE_FREE_TALK					1//连续对话模块
#define DATA_STREAM_SHARE_ENABLE 			1//录音数据共享
#define MODULE_KEYWORD_WAKEUP				0//关键词唤醒模块
#define SPEECH_CTRL_TRANSLATE_MODEL			1//用语音说命令词控制翻译模式

/******************************************************************
//产品硬件IO定义
*******************************************************************/
#if WROAR_BOARD
#define IIS_SCLK            5
#define IIS_LCLK            25
#define IIS_DSIN            26
#define IIS_DOUT            35

/* LED related */
#define LED_INDICATOR_SYS    19  //red
#define LED_INDICATOR_NET    22  //green
#define GPIO_SEL_LED_RED     GPIO_SEL_19  //red
#define GPIO_SEL_LED_GREEN   GPIO_SEL_22  //green

/* PA */
#define GPIO_PA_EN           GPIO_NUM_21
#define GPIO_SEL_PA_EN       GPIO_SEL_21

#define IIC_CLK 23
#define IIC_DATA 18

/* Press button related */
#define GPIO_SEL_REC         GPIO_SEL_36    //SENSOR_VP
#define GPIO_SEL_MODE        GPIO_SEL_39    //SENSOR_VN
#define GPIO_REC             GPIO_NUM_36
#define GPIO_MODE            GPIO_NUM_39

/* AUX-IN related */
#define GPIO_SEL_AUX         GPIO_SEL_12    //Aux in
#define GPIO_AUX             GPIO_NUM_12

/* SD card relateed */
#define SD_CARD_INTR_GPIO    GPIO_NUM_34
#define SD_CARD_INTR_SEL  	 GPIO_SEL_34
#define LED_QICAI_GPIO		 GPIO_NUM_33
#define LED_QICAI_SEL		 GPIO_SEL_33

#if MODULE_CHECK_CHARGING_STATE
#define CHARGING_STATE_GPIO		GPIO_NUM_32
#define CHARGING_STATE_SEL		GPIO_SEL_32
#endif

#else
#define IIC_CLK              19
#define IIC_DATA             21

#define IIS_SCLK             26
#define IIS_LCLK             25
#define IIS_DSIN             22
#define IIS_DOUT             35

#define LED_INDICATOR_SYS    18  //red
#define LED_INDICATOR_NET    5  //green

#define GPIO_SEL_LED_RED     GPIO_SEL_18  //red
#define GPIO_SEL_LED_GREEN   GPIO_SEL_5  //green
#endif


#endif /* __USER_CONFIG_H__ */
