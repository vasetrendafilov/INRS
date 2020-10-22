#include <Arduino_FreeRTOS.h>

//define task handles
TaskHandle_t TaskBlink_Handler;
TaskHandle_t TaskSerial_Handler;

// define two tasks for Blink & Serial
void TaskBlink( void *pvParameters );
void TaskSerial(void* pvParameters);

// the setup function runs once when you press reset or power the board
void setup() {
  // initialize serial communication at 9600 bits per second:
  Serial.begin(9600);
  
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB, on LEONARDO, MICRO, YUN, and other 32u4 based boards.
  }
  
  // Now set up two tasks to run independently.
   xTaskCreate(
    TaskBlink
    ,  "Blink"   // A name just for humans
    ,  128  // This stack size can be checked & adjusted by reading the Stack Highwater
    ,  NULL //Parameters passed to the task function
    ,  2  // Priority, with 2 (configMAX_PRIORITIES - 1) being the highest, and 0 being the lowest.
    ,  &TaskBlink_Handler );//Task handle

   xTaskCreate(
    TaskSerial
    ,  "Serial"
    ,  128  // Stack size
    ,  NULL //Parameters passed to the task function
    ,  1  // Priority
    ,  &TaskSerial_Handler );  //Task handle
}
    

void loop()
{
  // Empty. Things are done in Tasks.
}

/*--------------------------------------------------*/
/*---------------------- Tasks ---------------------*/
/*--------------------------------------------------*/

void TaskSerial(void* pvParameters){
/*
 Serial
 Send "s" or "r" through the serial port to control the suspend and resume of the LED light task.
 This example code is in the public domain.
*/
  (void) pvParameters;
   for (;;) // A Task shall never return or exit.
   {
    long num;
    while(Serial.available()>0){
      num = Serial.parseInt();
      if( num != 0){
      if(xTaskNotify(TaskBlink_Handler,(uint32_t)num, eSetValueWithOverwrite) == pdPASS){
        Serial.println("Sent");
      }
      vTaskDelay(1);
      }
    }
   }
}

void TaskBlink(void *pvParameters)  // This is a task.
{
  (void) pvParameters;
  uint32_t ulNotifiedValue;
  for (;;) // A Task shall never return or exit.
  {   
           xTaskNotifyWait(   0x00,              /* Don’t clear any notification bits on entry. */
                              0xffffffff,        /* Reset the notification value to 0 on exit. */
                              &ulNotifiedValue,  /* Notified value pass out in ulNotifiedValue. */
                              portMAX_DELAY );    /* TicksToWait */
                              
      if(ulNotifiedValue != 0){
        Serial.println(ulNotifiedValue);   
        } 
  }
}
