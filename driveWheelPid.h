#ifndef DRIVEWHEELPID_H
#define DRIVEWHEELPID_H
#include "imuInteract.h"
#include "encoder.h"
#include <assert.h>
#include <wiringPi.h>
#include "time.h"

typedef struct{
   long encoderCnt;
   long encoderGoal;
   double curError;
   //this is for each wheel independet of the larger task ahead
   int curDir;
   double pow;//out of 1. 1 for max power 
}WheelPid;
typedef struct{
   float Rx;
   float Ry;
   float Rz;
   double curError;
}ImuDir;

enum dir{Forward, Backward, Right, Left}; 

pid_t  createDriveWheelChild(int ** writeToChild);
int pin(int index);
#endif
