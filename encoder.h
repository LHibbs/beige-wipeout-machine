#ifndef ENCODER_H
#define ENCODER_H

#include "helpFunctions.h"
#include <time.h>
#include <wiringPi.h>

//array index
#define FL 0
#define FR 1
#define BR 2
#define BL 3

typedef struct{
   long count;
   long goal;
   char lastVal;
   char dir;
}Encoder;

typedef struct{
   char dir;
}MotorMsg;

void createEncoderChild(int** writeToChild);

void encoderChildFunct();
#endif
