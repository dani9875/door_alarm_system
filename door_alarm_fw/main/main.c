#include "driver/gpio.h"
#include "driver/touch_pad.h"
#include "driver/usb_serial_jtag.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char       TAG[] = "MAIN";

void app_main(void)
{

    while(1)
    {


        vTaskDelay(pdMS_TO_TICKS(2000));
    }

    return;
}