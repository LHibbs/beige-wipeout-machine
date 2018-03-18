#ifndef DRIVEWHEELPID_H
#define DRIVEWHEELPID_H
#include "encoder.h"
typedef struct{
   double speedGoal;
   double curSpeed;//units : encoder count/ms
   double lastSpeed;
   long encoderCnt;
   long encoderGoal;
   double curDir;
}WheelPid;


pid_t  createDriveWheelChild(int ** writeToChild);
int pin(int index);
#endif
