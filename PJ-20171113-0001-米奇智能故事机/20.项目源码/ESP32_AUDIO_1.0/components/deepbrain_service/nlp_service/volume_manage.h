#ifndef __VOLUME_MANAGE_H__
#define __VOLUME_MANAGE_H__

#define ATTR_NAME_VOL_CTRL			"音量控制"
#define ATTR_NAME_VOL_SCALE			"音量调节刻度"

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
