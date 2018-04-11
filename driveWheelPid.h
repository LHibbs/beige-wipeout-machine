#ifndef DRIVEWHEELPID_H
#define DRIVEWHEELPID_H

#include "imuInteract.h"
#include "encoder.h"
#include <assert.h>
#include <wiringPi.h>
#include "time.h"
#include "lineSensor.h"

typedef struct{
   long encoderCnt;
   long encoderGoal;
   double curError;
   //this is for each wheel independet of the larger task ahead
   int curDir;
   int tempCurDir;
   int powToWheels;
   double pow;//out of 1. 1 for max power 
}WheelPid;

typedef struct{
   float Rx;
   float Ry;
   float Rz;
   double curError;
}ImuDir;

enum commandType{Line, Distance, Align}; 

typedef struct {
    enum commandType cmdType;
    double encoderDist;
    // line sensor combo that indicates stop
    unsigned char lineSensorConfig; 
    //information about what config of line sensors indicates a stop
    //...
}Command; 

enum dir{Forward, Right, Backward, Left, Clockwise, Counterclockwise}; 




pid_t  createDriveWheelChild(int ** writeToChild);
int pin(int index);
#endif
