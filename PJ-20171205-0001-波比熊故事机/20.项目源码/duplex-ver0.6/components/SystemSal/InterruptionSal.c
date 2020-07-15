#include "driver/gpio.h"
#include "InterruptionSal.h"

int GpioInterInstall()
{
    int res = 0;
    res = gpio_install_isr_service(GPIO_INTER_FLAG);

    return res;
}

