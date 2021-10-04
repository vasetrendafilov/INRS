#include "bmx280.h"
#include "mpu9250.h"
#include "filter.h"
#include "calibrate.h"
#include "neo6m.h"
#include "common.h"
#include "sdcard.h"
#include "ssd1306.h"

#define I2C_MASTER_NUM I2C_NUM_0 /*!< I2C port number for master dev */


void transform_accel_gyro(vector_t *v);
void transform_mag(vector_t *v);

void bmptask(void* arg);
void mputask(void* arg);
void gpstask();
