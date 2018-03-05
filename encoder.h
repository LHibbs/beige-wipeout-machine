#ifndef ENCODER_H
#define ENCODER_H

#include "helpFunctions.h"
#include <time.h>

//array index
#define FR 0
#define FL 1
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
