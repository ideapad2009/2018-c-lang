#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include "esp_log.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "MediaService.h"
#include "freertos/queue.h"
#include "freertos/timers.h"
#include "EspAudio.h"
#include "toneuri.h"
#include "deep_sleep_manage.h"
#include "device_api.h"
#include "player_middleware.h"

xTimerHandle g_countdown = NULL;
time_t g_start = 0;
time_t g_end = 0;
time_t g_time_length = 0;

#define TAG_COUNTDOWN_MANAGE "COUNTDOWN_MANAGE"

void countdown_manage()
{
	if (g_countdown != NULL)
	{
		g_end = get_time_of_day();
		time_t time_length = difftime(g_end/1000, g_start/1000);
		printf("The time_length is %ld s\n", time_length);
		if (time_length >= g_time_length)
		{
			deep_sleep_manage();
		}
		xTimerStart(g_countdown, 50/portTICK_PERIOD_MS);
	}
}

void countdown_manage_stop()
{
	xTimerStop(g_countdown, 50/portTICK_PERIOD_MS);
	xTimerDelete(g_countdown, 50/portTICK_PERIOD_MS);
	g_countdown = NULL;
}

void countdown_Manage_init(time_t _time_length)
{
	g_time_length = _time_length;
	g_start = get_time_of_day();
	g_countdown = xTimerCreate((const char*)"countdown_manage", 1000/portTICK_PERIOD_MS, pdFALSE, &g_countdown,
                countdown_manage);
	xTimerStart(g_countdown, 50/portTICK_PERIOD_MS);
}


