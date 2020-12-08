/*
 * Example of a Arduino interruption and RTOS Binary Semaphore
 * https://www.freertos.org/Embedded-RTOS-Binary-Semaphores.html
 */


// Include Arduino FreeRTOS library
#include <Arduino_FreeRTOS.h>

// Include semaphore supoport
#include <semphr.h>

#include <Wire.h>
#include <MPU6050.h>

MPU6050 mpu;

// Pitch, Roll and Yaw values
float pitch = 0;
float roll = 0;
float yaw = 0;
/* 
 * Declaring a global variable of type SemaphoreHandle_t 
 * 
 */
SemaphoreHandle_t interruptSemaphore;
void TaskDigitalRead( void *pvParameters );
void TaskLed( void *pvParameters );
void setup() {

  // Configure pin 2 as an input and enable the internal pull-up resistor
  pinMode(2, INPUT_PULLUP);
  Serial.begin(115200);
    
    // Initialize MPU6050
    while(!mpu.begin(MPU6050_SCALE_2000DPS, MPU6050_RANGE_2G))
    {
      Serial.println("Could not find a valid MPU6050 sensor, check wiring!");
      delay(500);
    }
    
    // Calibrate gyroscope. The calibration must be at rest.
    // If you don't want calibrate, comment this line.
    mpu.calibrateGyro();
  
    // Set threshold sensivty. Default 3.
    // If you don't want use threshold, comment this line or set 0.
    mpu.setThreshold(3);
    mpu.setIntDataEnable(1);
 // Create task for Arduino led 
  xTaskCreate(TaskLed, // Task function
              "Led", // Task name
              128, // Stack size 
              NULL, 
              1, // Priority
              NULL );
  xTaskCreate(
      TaskDigitalRead
      ,  "digitalRead"
      ,  128  // Stack size
      ,  NULL
      ,  0  // Priority
      ,  NULL );
  /**
   * Create a binary semaphore.
   * https://www.freertos.org/xSemaphoreCreateBinary.html
   */
  interruptSemaphore = xSemaphoreCreateBinary();
  if (interruptSemaphore != NULL) {
    // Attach interrupt for Arduino digital pin
    attachInterrupt(digitalPinToInterrupt(2), interruptHandler, CHANGE);
  }

  
}

void loop() {}


void interruptHandler() {
  /**
   * Give semaphore in the interrupt handler
   * https://www.freertos.org/a00124.html
   */
  
  xSemaphoreGiveFromISR(interruptSemaphore, NULL);
}
void TaskDigitalRead( void *pvParameters __attribute__((unused)) )  // This is a Task.
{

  for (;;) // A Task shall never return or exit.
  {

      Serial.println("TAs");

    vTaskDelay(1);  // one tick delay (15ms) in between reads for stability
  }
}


/* 
 * Led task. 
 */
void TaskLed(void *pvParameters)
{
  (void) pvParameters;

  for (;;) {
    
    /**
     * Take the semaphore.
     * https://www.freertos.org/a00122.html
     */
    if (xSemaphoreTake(interruptSemaphore, portMAX_DELAY) == pdPASS) {
       Vector norm = mpu.readNormalizeGyro();

        // Calculate Pitch, Roll and Yaw
        pitch = pitch + norm.YAxis;
        roll = roll + norm.XAxis;
        yaw = yaw + norm.ZAxis;
        Serial.print(" Pitch = ");
        Serial.print(pitch);
        Serial.print(" Roll = ");
        Serial.print(roll);  
        Serial.print(" Yaw = ");
        Serial.println(yaw);
        vTaskDelay(1);
    }
    
  }
}
