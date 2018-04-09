#ifndef LINESENSOR_H
#define LINESENSOR_H

#include "helpFunctions.h"
#include <time.h>
#include <wiringPi.h>

enum State{Charge, IO, Wait}; 

typedef struct {
   struct timeval *start; 
   enum State state; 
   int count;
} lightSensor; 


void createLineSensorChild(int** writeToChild);

void lineChildFunct();
#endif
