#ifndef _LOW_POWER_INDICATION_H_
#define _LOW_POWER_INDICATION_H_

#include "driver/gpio.h"
#include "userconfig.h"

void low_power_manage_init();
void low_power_manage_set(float vol);
void set_battery_vol(float vol);

#endif
