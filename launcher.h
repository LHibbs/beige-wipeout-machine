#ifndef LAUNCHER_H
#define LAUNCHER_H

#include "helpFunctions.h"
#include <time.h>
#include <wiringPi.h>

#define FEEDER_PIN 4
#define FEEDER_DIR_PIN 13
#define FEEDER_SWITCH_PIN 23 
#define FEEDER_SWITCH_ON 1
#define LAUNCHER_PIN 5

pid_t createLaunchingChild(int** writeToChild);
void lanchingChildFunct(int new_stdin);

#endif
