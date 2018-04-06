#ifndef ENCODER_H
#define ENCODER_H

#include "helpFunctions.h"
#include <time.h>
#include <wiringPi.h>


void createLauncherChild(int** writeToChild);
void encoderChildFunct();

#endif
