#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "filter.h"
#include "../control/control.h"// samo go zimame hederot
PIDdata pid_defaulsts = {

    .kP = 1.5f,
    .kI = 0.0f,
    .kD = 1.234f,
    .dT = 1,

    .Outmin = -255.0f,
    .Outmax = 255.0f,
    .Ierrmin = -10.0f,
    .Ierrmax = 10.0f
};
void test(void* arg)
{
     while(1){
          vTaskDelay(500/portTICK_RATE_MS);
    printf("%f\n",servo->target);
   
    }

}
