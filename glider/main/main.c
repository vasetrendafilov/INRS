#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#include "esp_log.h"
#include "tasks.h"

void app_main(void)
{
    xTaskCreatePinnedToCore(bmptask, "bmptask", 2048, NULL, 1, NULL,1); 
    xTaskCreatePinnedToCore(mputask, "mputask", 4096, NULL, 1, NULL,1); 
    gpstask();
}