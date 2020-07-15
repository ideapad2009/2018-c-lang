#include <stdio.h>
#include "ctypes.h"
#include "DeviceCommon.h"
//#include "TouchControlService.h"
#include "EspAudio.h"
#include "toneuri.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/timers.h"
#include "functional_running_state_check.h"
#include "player_middleware.h"

#define PREVENT_CHILD_TRIGGER_OFF 	0	//童锁开关，默认为关

void prevent_child_trigger()
{
	static int8_t prevent_child_trigger_switch = PREVENT_CHILD_TRIGGER_OFF;
	if (prevent_child_trigger_switch == PREVENT_CHILD_TRIGGER_OFF)
	{
		prevent_child_trigger_switch = !PREVENT_CHILD_TRIGGER_OFF;
		set_functional_tyigger_key_state(FUNCTIONAL_TYIGGER_KEY_PREVENT_CHILD_TRIGGER_STATE);
		player_mdware_play_tone(FLASH_MUSIC_CHILD_LOCK_HAS_BEEN_OPENED);
		vTaskDelay(2000);
		printf("prevent_child_trigger opend!\n");
	}
	else
	{
		prevent_child_trigger_switch = PREVENT_CHILD_TRIGGER_OFF;
		set_functional_tyigger_key_state(FUNCTIONAL_TYIGGER_KEY_INITIAL_STATE);
		player_mdware_play_tone(FLASH_MUSIC_CHILD_LOCK_HAS_BEEN_CLOSED);
		vTaskDelay(2000);
		printf("prevent_child_trigger off!\n");
	}
}
