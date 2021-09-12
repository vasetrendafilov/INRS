#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "control.h"
//#include "pid.h"

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

int a = 10;// treba i da se deklarira

void weight_reading_task(void* arg){
    while(1){
    PID_SetTraget(servo,10.0,9.0);
    printf("yep\n");
    vTaskDelay(250/portTICK_RATE_MS);
    }
}
