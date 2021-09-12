#ifndef PID_H
#define PID_H


#include <string.h>


typedef struct _PIDdata {

    float prev_input;

    float target;

    // PID factors
    float kP;
    float kI;
    float kD;
    float dT;

    float Ierr;

    // PID terms limits
    float Outmin;
    float Outmax;
    float Ierrmin;
    float Ierrmax;
} PIDdata;
typedef PIDdata *ptrPIDdata;

void PID_SetTraget(ptrPIDdata pid, float target, float prev_input);
void PID_SetTuningParams(ptrPIDdata pid, float kP, float kI, float kD);
void PID_SetLimits(ptrPIDdata pid, float Outmin, float Outmax, float Ierrmin, float Ierrmax);
void PID_ResetIerr(ptrPIDdata pid);
float PID_Update(ptrPIDdata pid, float input);


#endif /* PID_H */