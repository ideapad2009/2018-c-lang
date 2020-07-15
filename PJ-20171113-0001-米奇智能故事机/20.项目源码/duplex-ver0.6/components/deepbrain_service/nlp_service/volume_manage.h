#ifndef __VOLUME_MANAGE_H__
#define __VOLUME_MANAGE_H__
#include "cJSON.h"

#define ATTR_NAME_VOL_CTRL			"音量控制"
#define ATTR_NAME_VOL_SCALE			"音量调节刻度"
//extern int g_vol_leave;				//用于固定每个级别的音量值

//以下宏放在头文件内，用以保证按键控制的音量范围和网络控制的音量范围一致
#define MIN_VOLUME			0										//最低音量
#define MAX_VOLUME			100										//最高音量
#define MODERATE_VOLUME		55										//适中音量
#define VOL_LEAVE           10										//10个级别
#define VOL_STEP_VALUE		((MAX_VOLUME-MIN_VOLUME)/VOL_LEAVE)		//音量梯度值
//#define NEED_SET_VALUE		(g_vol_leave * VOL_STEP_VALUE + MIN_VOLUME)	//需要设置成的音量

typedef enum
{
	VOLUME_CONTROL_INITIAL,			//初始状态,无控制命令状态
	VOLUME_CONTROL_HIGHER,			//调高音量
	VOLUME_CONTROL_LOWER,			//调低音量
	VOLUME_CONTROL_MODERATE,		//音量适中
	VOLUME_CONTROL_MAX,				//音量最大
	VOLUME_CONTROL_MIN				//音量最小
}VOLUME_CONTROL_T;

typedef enum
{
	VOLUME_TYPE_INITIAL,			//初始状态
	VOLUME_TYPE_SCALE_MODE,			//有音量刻度模式
	VOLUME_TYPE_NOT_SCALE_MODE		//无音量刻度模式
}VOLUME_TYPE_T;

void volume_control(cJSON *json_data);

#endif
