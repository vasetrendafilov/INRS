#include "tasks.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include <math.h>
#include "esp_system.h"
#include "esp_err.h"
#include "esp_task_wdt.h"

calibration_t cal = {
    .mag_offset = {.x = 87.886719, .y = 25.531250, .z = 6.273438},
    .mag_scale = {.x = 1.070721, .y = 0.885044, .z = 1.068190},
    .accel_offset = {.x = 0.045010, .y = 0.001923, .z = -0.039888},
    .accel_scale_lo = {.x = 1.023406, .y = 1.004785, .z = 1.006032},
    .accel_scale_hi = {.x = -0.980382, .y = -1.001094, .z = -1.015309},

    .gyro_bias_offset = {.x = 0.768579, .y = -0.180539, .z = 0.919622}};

static const char *TAG = "main";
    
void transform_accel_gyro(vector_t *v)
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
void transform_mag(vector_t *v)
{
  float x = v->x;
  float y = v->y;
  float z = v->z;

  v->x = -y;
  v->y = z;
  v->z = -x;
}
void mputask(void* arg){
  i2c_mpu9250_init(&cal);
  MadgwickAHRSinit(SAMPLE_FREQ_Hz, 0.8);
  //SdCardLog(0,"mpu9250","Init succeful");
  uint64_t i = 0;
  float heading, pitch, roll;
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

      
      MadgwickGetEulerAnglesDegrees(&heading, &pitch, &roll);
      ESP_LOGI(TAG, "heading: %2.3f°, pitch: %2.3f°, roll: %2.3f°, Temp %2.3f°C", heading, pitch, roll, temp);
      char str[17];
      snprintf(&str[0], 17, "R:%2.0f P:%2.0f Y:%2.0f", roll, pitch,heading);
      //SdCardLog(4,"mpu9250",str);
      sdd1306_log(0,str);

      // Make the WDT happy
      esp_task_wdt_reset();
    }

    pausempu();
  }
}

void bmptask(void* arg)
{
      // Entry Point
    //ESP_ERROR_CHECK(nvs_flash_init());
    i2c_config_t i2c_cfg = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = GPIO_NUM_18,
        .scl_io_num = GPIO_NUM_19,
        .sda_pullup_en = false,
        .scl_pullup_en = false,
        .clk_flags = 0,
        .master = {
            .clk_speed = 100000
        }
    };

    ESP_ERROR_CHECK(i2c_param_config(I2C_NUM_1, &i2c_cfg));
    ESP_ERROR_CHECK(i2c_driver_install(I2C_NUM_1, I2C_MODE_MASTER, 0, 0, 0));

    bmx280_t* bmx280 = bmx280_create(I2C_NUM_1);

    if (!bmx280) { 
        ESP_LOGE("test", "Could not create bmx280 driver.");
        return;
    }

    ESP_ERROR_CHECK(bmx280_init(bmx280));

    bmx280_config_t bmx_cfg = BMX280_DEFAULT_CONFIG;
    ESP_ERROR_CHECK(bmx280_configure(bmx280, &bmx_cfg));
    ESP_ERROR_CHECK(bmx280_setMode(bmx280, BMX280_MODE_CYCLE));
    SdCardLog(0,"bmp280","bmp280 succeful init");
    while (1)
    {
        do {
            vTaskDelay(pdMS_TO_TICKS(5));
        } while(bmx280_isSampling(bmx280));

        float temp = 0, pres = 0;
        ESP_ERROR_CHECK(bmx280_readoutFloat(bmx280, &temp, &pres));
        float altitude = 44330 * (1.0 - pow(pres / (CONFIG_BMX280_ATMOSPHERIC*100), 0.1903));
        char str[128];
        snprintf(&str[0], 128, "%f %f %f", temp, pres, altitude);
        SdCardLog(3,"bmp280",str);
        ESP_LOGI("bmp280", "Read Values: temp = %f, pres = %f, alt = %f", temp, pres, altitude);
    }
}

static void gps_event_handler(void *event_handler_arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    gps_t *gps = NULL;
    switch (event_id) {
    case GPS_UPDATE:
        gps = (gps_t *)event_data;
        /* print information parsed from GPS statements */
        ESP_LOGI("gps", "%d/%d/%d %d:%d:%d =>  %.05f°N  %.05f°E %.02fm/s %fm/s %f and is %d",
                 gps->date.year , gps->date.month, gps->date.day,
                 gps->tim.hour, gps->tim.minute, gps->tim.second,
                 gps->latitude, gps->longitude, gps->cog, gps->speed, gps->variation, gps->valid);
        if (gps->valid){
         // char str[128];
         // snprintf(&str[0], 128, "%.05f  %.05f  %.02f  %.02f %f %d", gps->latitude, gps->longitude, gps->cog, gps->speed, gps->variation, gps->valid);
         // SdCardLog(5,"neo6m",str);
        
        }
          char str[17];
          snprintf(&str[0], 17, "%2.0fN %2.0fE %2.1fm/s", gps->latitude, gps->longitude,gps->speed);
          sdd1306_log(1,str);
        break;
    case GPS_UNKNOWN:
        /* print unknown statements */
        //ESP_LOGW("gps", "Unknown statement:%s", (char *)event_data);
        break;
    default:
        break;
    }
}
void gpstask(){
  
    /* NMEA parser configuration */
    nmea_parser_config_t config = NMEA_PARSER_CONFIG_DEFAULT();
    /* init NMEA parser library */
    nmea_parser_handle_t nmea_hdl = nmea_parser_init(&config);
    /* register event handler for NMEA parser library */
    nmea_parser_add_handler(nmea_hdl, gps_event_handler, NULL);

    //vTaskDelay(10000 / portTICK_PERIOD_MS);

    /* unregister event handler */
   // nmea_parser_remove_handler(nmea_hdl, gps_event_handler);
    /* deinit NMEA parser library */
   // nmea_parser_deinit(nmea_hdl);
}