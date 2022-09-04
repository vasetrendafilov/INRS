/*****************************************************************************
 *                                                                           *
 *  Copyright 2018 Simon M. Werner                                           *
 *                                                                           *
 *  Licensed under the Apache License, Version 2.0 (the "License");          *
 *  you may not use this file except in compliance with the License.         *
 *  You may obtain a copy of the License at                                  *
 *                                                                           *
 *      http://www.apache.org/licenses/LICENSE-2.0                           *
 *                                                                           *
 *  Unless required by applicable law or agreed to in writing, software      *
 *  distributed under the License is distributed on an "AS IS" BASIS,        *
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. *
 *  See the License for the specific language governing permissions and      *
 *  limitations under the License.                                           *
 *                                                                           *
 *****************************************************************************/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#include "esp_log.h"
#include "esp_system.h"
#include "esp_err.h"
#include "esp_task_wdt.h"

#include "driver/i2c.h"

#include "../components/ahrs/MadgwickAHRS.h"
#include "../components/mpu9250/mpu9250.h"
#include "../components/mpu9250/calibrate.h"
#include "../components/mpu9250/common.h"

static const char *TAG = "main";

#define I2C_MASTER_NUM I2C_NUM_0 /*!< I2C port number for master dev */
#define MAGNNETIC_DECLINATION 4.8
calibration_t cal = {
       .mag_offset = {.x = 9.437500, .y = 68.281250, .z = -37.640625},
    .mag_scale = {.x = 0.999212, .y = 1.039023, .z = 0.964536},
     .accel_offset = {.x = 0.045849, .y = 0.008582, .z = -0.047018},
    .accel_scale_lo = {.x = 1.026488, .y = 1.008129, .z = 1.012812},
    .accel_scale_hi = {.x = -0.975170, .y = -0.995973, .z = -1.008010},

     .gyro_bias_offset = {.x = 0.654400, .y = -0.285330, .z = 1.011093}};
/*
 .mag_offset = {.x = 10.617188, .y = 68.875000, .z = -39.921875},
    .mag_scale = {.x = 0.996100, .y = 1.023669, .z = 0.981155},
    
       .mag_offset = {.x = 9.437500, .y = 70.062500, .z = -38.781250},
    .mag_scale = {.x = 0.985117, .y = 1.046128, .z = 0.971830},

    .mag_offset = {.x = 10.617188, .y = 70.062500, .z = -41.062500},
    .mag_scale = {.x = 1.002208, .y = 1.032489, .z = 0.967428},

    .mag_offset = {.x = 9.437500, .y = 68.281250, .z = -37.640625},
    .mag_scale = {.x = 0.999212, .y = 1.039023, .z = 0.964536},

calibration_t cal = {
  .mag_offset = {.x = 79.628906, .y = 27.906250, .z = 3.421875},
    .mag_scale = {.x = 1.004934, .y = 0.981822, .z = 1.013792},
        .accel_offset = {.x = 0.036187, .y = 0.006879, .z = 0.012961},
    .accel_scale_lo = {.x = 1.021494, .y = 1.003609, .z = 1.004452},
    .accel_scale_hi = {.x = -0.983548, .y = -0.998936, .z = -1.016415},
     .gyro_bias_offset = {.x = 0.670064, .y = -0.149024, .z = 0.884596}};*/

/*
   .mag_offset = {.x = 93.195312, .y = 10.687500, .z = 0.000000},
    .mag_scale = {.x = 1.090124, .y = 1.016649, .z = 0.909877},
   .accel_offset = {.x = 0.035091, .y = 0.008815, .z = -0.011422},
    .accel_scale_lo = {.x = 1.020747, .y = 1.005207, .z = 1.004940},
    .accel_scale_hi = {.x = -0.982701, .y = -1.000773, .z = -1.014469},
    .gyro_bias_offset = {.x = 0.698556, .y = -0.170777, .z = 0.871709}
*/

/**
 * Transformation:
 *  - Rotate around Z axis 180 degrees
 *  - Rotate around X axis -90 degrees
 * @param  {object} s {x,y,z} sensor
 * @return {object}   {x,y,z} transformed
 */
static void transform_accel_gyro(vector_t *v)
{
  float x = v->x;
  float y = v->y;
  float z = v->z;

  v->x = -x;
  v->y = -z;
  v->z = -y;
}

/**
 * Transformation: to get magnetometer aligned
 * @param  {object} s {x,y,z} sensor
 * @return {object}   {x,y,z} transformed
 */
static void transform_mag(vector_t *v)
{
  float x = v->x;
  float y = v->y;
  float z = v->z;

  v->x = -y;
  v->y = z;
  v->z = -x;
}

void run_imu(void)
{

  i2c_mpu9250_init(&cal);
  MadgwickAHRSinit(SAMPLE_FREQ_Hz, 0.8);

  uint64_t i = 0;
  while (true)
  {
    vector_t va, vg, vm;

    // Get the Accelerometer, Gyroscope and Magnetometer values.
    ESP_ERROR_CHECK(get_accel_gyro_mag(&va, &vg, &vm));

    // Transform these values to the orientation of our device.
    transform_accel_gyro(&va);
    transform_accel_gyro(&vg);
    transform_mag(&vm);

    // Apply the AHRS algorithm
    MadgwickAHRSupdate(DEG2RAD(vg.x), DEG2RAD(vg.y), DEG2RAD(vg.z),
                       va.x, va.y, va.z,
                       vm.x, vm.y, vm.z);

    // Print the data out every 10 items
    if (i++ % 10 == 0)
    {
      float temp;
      ESP_ERROR_CHECK(get_temperature_celsius(&temp));

      float heading, pitch, roll;
      MadgwickGetEulerAnglesDegrees(&heading, &pitch, &roll);

      heading += MAGNNETIC_DECLINATION - 360*(heading + MAGNNETIC_DECLINATION > 360);

      ESP_LOGI(TAG, "%2.3f %2.3f %2.3f",heading,pitch,roll);

      // Make the WDT happy
      esp_task_wdt_reset();
    }

    pause();
  }
}

static void imu_task(void *arg)
{

#ifdef CONFIG_CALIBRATION_MODE
  //calibrate_gyro();
  //calibrate_accel();
  calibrate_mag();
#else
  run_imu();
#endif

  // Exit
  vTaskDelay(100 / portTICK_RATE_MS);
  i2c_driver_delete(I2C_MASTER_NUM);

  vTaskDelete(NULL);
}

void app_main(void)
{
  //start i2c task
  xTaskCreate(imu_task, "imu_task", 4096, NULL, 10, NULL);
}