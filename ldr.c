#include "ldr.h"
#include <wiringPi.h>
#include "time.h"
void runLDRTest(){

   wiringPiSetup();

   struct timeval *before,*after;

   struct timespec sleepTime;
   sleepTime.tv_sec = 0;
   sleepTime.tv_nsec = 1000;

   //double resistance;
//   double cap = 1/10000;

   before = malloc(sizeof(struct timespec));
   after = malloc(sizeof(struct timespec));
   double dt_usec,dt_msec;
   double avg = 0;
   while(1){
      avg = 0;
      for(int i= 0 ; i < 5; i++){

        pinMode(7,OUTPUT);
        nanosleep(&sleepTime,NULL);
        digitalWrite(7,LOW);
        pinMode(7,INPUT);
        gettimeofday(before,NULL);
        while(digitalRead(7)!=1){
         nanosleep(&sleepTime,NULL);
        }
        gettimeofday(after,NULL);
        dt_usec =  (after->tv_usec - before->tv_usec) +\
              1000000*(after->tv_sec - before->tv_sec);
        dt_msec  = dt_usec/1000;

        //resistance = dt_msec;

avg += dt_msec;
        nanosleep(&sleepTime,NULL);
        }
      avg /= 5;
        printf("time(msec):%7.4g\n",avg);;
   }

}
