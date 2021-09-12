#ifndef _MPU6050_APPLICATION_H
#define _MPU6050_APPLICATION_H
#include "pid.h" // najdobro e tuka da gi stavam
void weight_reading_task(void* arg);
extern ptrPIDdata servo;
extern int a;// tuka go prive extern za da ja koristi nekoj drug fajl
#endif
