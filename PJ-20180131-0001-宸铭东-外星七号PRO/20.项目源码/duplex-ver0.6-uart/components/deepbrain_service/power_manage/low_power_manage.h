#ifndef _LOW_POWER_INDICATION_H_
#define _LOW_POWER_INDICATION_H_

#include "driver/gpio.h"
#include "userconfig.h"

void set_battery_vol(float vol);
int get_battery_charging_state(void);
uint32_t get_device_sleep_time(void);
void update_device_sleep_time(void);
void low_power_manage_create();

#endif
