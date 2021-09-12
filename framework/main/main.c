#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
//#include "control.h"
//#include "pid.h"
#include "filter.h"
#include "../components/control/control.h"
#include "filter.h"

/*
PIDdata pid_defaults = {

    .kP = 1.5f,
    .kI = 0.0f,
    .kD = 1.234f,
    .dT = 1,

    .Outmin = -255.0f,
    .Outmax = 255.0f,
    .Ierrmin = -10.0f,
    .Ierrmax = 10.0f
};
ptrPIDdata servo = &pid_defaults;
*/

void app_main(void)
{
    xTaskCreatePinnedToCore(weight_reading_task, "weight_reading_task", 2048, NULL, 1, NULL,1); 
    xTaskCreatePinnedToCore(test, "s", 2048, NULL, 1, NULL,0); 
}
