#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "sdkconfig.h"
#include "esp_log.h"
static const char *REST_TAG = "esp-rest";

void printTask(void* arg);
void Task(void* arg);

void app_main(void)
{
    if (xTaskCreatePinnedToCore(printTask,"print",2048,NULL,1,NULL,0) == pdPASS){
       ESP_LOGW(REST_TAG, "Task is not created");
    }
    xTaskCreatePinnedToCore(Task,"prinst",2048,NULL,1,NULL,0);

}

void printTask(void* arg)
{
    uint32_t timestamp;
    TickType_t xLastWakeTime;
    const TickType_t xFrequency = 50;

    // Initialise the xLastWakeTime variable with the current time.
    xLastWakeTime = xTaskGetTickCount ();

    for( ;; )
    {
        // Wait for the next cycle.
        vTaskDelayUntil( &xLastWakeTime, xFrequency/ portTICK_PERIOD_MS );
        timestamp = esp_log_timestamp();
        printf("Time is: %d \n",timestamp);
       
    }
}
void Task(void* arg)
{
    uint32_t timestamp;
    TickType_t xLastWakeTime;
    const TickType_t xFrequency = 100;

    // Initialise the xLastWakeTime variable with the current time.
    xLastWakeTime = xTaskGetTickCount ();

    for( ;; )
    {
        // Wait for the next cycle.
        vTaskDelayUntil( &xLastWakeTime, xFrequency/ portTICK_PERIOD_MS );
        timestamp = esp_log_timestamp();
        printf("Tim2e is: %d \n",timestamp);
       
    }
}