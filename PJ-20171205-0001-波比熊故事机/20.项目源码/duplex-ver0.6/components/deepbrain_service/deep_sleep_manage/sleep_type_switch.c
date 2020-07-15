#include <stdio.h>
//#include "freertos/queue.h"
#include "countdown_manage.h"
#include "EspAudio.h"
#include "toneuri.h"
#include "player_middleware.h"

#define _15MIN		900		//秒
#define _30MIN		1800	//秒
#define _45MIN		2700	//秒

typedef enum TriggerSequenceNumber {
    FIRST_15MIN,
    SECOND_30MIN,
    THIRD_45MIN,
    FOURTH_OFF_SLEEP,
} TriggerSequenceNumber;

void sleep_type_switch()
{
	static TriggerSequenceNumber g_sleep_type = FIRST_15MIN;
	
	switch (g_sleep_type)
	{
		case FIRST_15MIN:
		{
			countdown_Manage_init(_15MIN);
			player_mdware_play_tone(FLASH_MUSIC_B_05_GET_INTO_SLEEP_15_MINUTES_LATER);
//			EspAudioTonePlay(tone_uri[TONE_TYPE_05_GET_INTO_SLEEP_15_MINUTES_LATER], TERMINATION_TYPE_NOW);
			g_sleep_type = SECOND_30MIN;
			break;
		}
		case SECOND_30MIN:
		{
			countdown_manage_stop();
			countdown_Manage_init(_30MIN);
			player_mdware_play_tone(FLASH_MUSIC_B_06_GET_INTO_SLEEP_30_MINUTES_LATER);
//			EspAudioTonePlay(tone_uri[TONE_TYPE_06_GET_INTO_SLEEP_30_MINUTES_LATER], TERMINATION_TYPE_NOW);
			g_sleep_type = THIRD_45MIN;
			break;
		}
		case THIRD_45MIN:
		{
			countdown_manage_stop();
			countdown_Manage_init(_45MIN);
			player_mdware_play_tone(FLASH_MUSIC_B_07_GET_INTO_SLEEP_45_MINUTES_LATER);
//			EspAudioTonePlay(tone_uri[TONE_TYPE_07_GET_INTO_SLEEP_45_MINUTES_LATER], TERMINATION_TYPE_NOW);
			g_sleep_type = FOURTH_OFF_SLEEP;
			break;
		}
		case FOURTH_OFF_SLEEP:
		{
			countdown_manage_stop();
			player_mdware_play_tone(FLASH_MUSIC_B_08_SLEEP_MODE_HAS_BEEN_CLOSED);
//			EspAudioTonePlay(tone_uri[TONE_TYPE_08_SLEEP_MODE_HAS_BEEN_CLOSED], TERMINATION_TYPE_NOW);
			g_sleep_type = FIRST_15MIN;
			break;
		}
		default:
		{
			break;
		}
	}
}
