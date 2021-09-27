
#ifndef FTILTER_h
#define FTILTER_h

void MadgwickAHRSinit(float sampleFreqDef, float betaDef);
void MadgwickAHRSupdate(float gx, float gy, float gz, float ax, float ay, float az, float mx, float my, float mz);
void MadgwickAHRSupdateIMU(float gx, float gy, float gz, float ax, float ay, float az);
void MadgwickGetEulerAnglesDegrees(float *heading, float *pitch, float *roll);

#endif
