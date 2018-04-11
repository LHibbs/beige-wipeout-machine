#include "lineSensor.h"
#include <time.h>
//encoder pinout
/*
#define LS_12_THRESHOLD 400  //white was 350-360 black was 430
#define LS_14_THRESHOLD 420  //ehite was 290 black was 480
#define LS_30_THRESHOLD 1000 //may need to change this hard to tell black was infinate
#define LS_21_THRESHOLD 2000 //may need to change this hard to tell black was infinate
#define LS_11_THRESHOLD 2000 //white is 750 black is really high
#define LS_10_THRESHOLD 1000 //white is 750 black is really high
*/

#define LS_12_THRESHOLD 3000  //white was 350-360 black was 430
#define LS_14_THRESHOLD 3000  //ehite was 290 black was 430
#define LS_30_THRESHOLD 3000 //may need to change this hard to tell black was infinate
#define LS_21_THRESHOLD 3000 //may need to change this hard to tell black was infinate
#define LS_11_THRESHOLD 3000 //white is 750 black is really high
#define LS_10_THRESHOLD 3000 //white is 750 black is really high

#define LS_0_PIN 10
#define LS_0_THRESHOLD LS_10_THRESHOLD

#define LS_1_PIN 11
#define LS_1_THRESHOLD LS_11_THRESHOLD

#define LS_2_PIN 21 
#define LS_2_THRESHOLD LS_21_THRESHOLD

#define LS_3_PIN 14 
#define LS_3_THRESHOLD LS_14_THRESHOLD

#define LS_4_PIN 12 
#define LS_4_THRESHOLD LS_12_THRESHOLD

#define LS_5_PIN 30
#define LS_5_THRESHOLD LS_30_THRESHOLD


/*function that is called that will never return that wil always loop around*/
void lineChildFunct(){
   char msg[100];
   struct timespec sleepTime;
   sleepTime.tv_nsec = 10000;//10usec
   sleepTime.tv_sec = sleepTime.tv_nsec/1000000000;
   struct timeval *after; 
   double diffTime; 
   unsigned char output; 
   unsigned char maskPin[] = {1,2,4,8,16,32};
   int pins[] = {LS_0_PIN,LS_1_PIN,LS_2_PIN,LS_3_PIN,LS_4_PIN,LS_5_PIN};
   int threshold[] = {LS_0_THRESHOLD,LS_1_THRESHOLD,LS_2_THRESHOLD,LS_3_THRESHOLD,LS_4_THRESHOLD,LS_5_THRESHOLD};
   struct pollfd stdin_poll = {
     .fd = STDIN_FILENO, .events = POLLIN |  POLLPRI };

   pinMode(LS_0_PIN,OUTPUT);
   pinMode(LS_1_PIN,OUTPUT);
   pinMode(LS_2_PIN,OUTPUT);
   pinMode(LS_3_PIN,OUTPUT);
   pinMode(LS_4_PIN,OUTPUT);
   pinMode(LS_5_PIN,OUTPUT);

   lightSensor lightSensors[6]; 
   after = malloc(sizeof(struct timeval));

   for (int i = 0 ; i < 6; i++) {
       lightSensors[i].start = malloc(sizeof(struct timeval)); 
       lightSensors[i].state = IO; 
       lightSensors[i].count = 0; 
   }

   fprintf(stderr,"got in lineSensor almost to while loop\n");
   while(1){

      for(int i = 0 ; i < 6 ; i++) {
      
         switch(lightSensors[i].state) {
            case IO:
               lightSensors[i].state = Charge;
               digitalWrite(pins[i],1);
               break; 
            case Charge: 
               pinMode(pins[i],INPUT);
               gettimeofday(lightSensors[i].start,NULL);
               lightSensors[i].state = Wait;
               break; 
            case Wait:
               gettimeofday(after,NULL);
               diffTime =  (after->tv_usec - lightSensors[i].start->tv_usec) +\
                  1000000*(after->tv_sec - lightSensors[i].start->tv_sec);
               if(digitalRead(pins[i])==0 || diffTime > threshold[i]){
                  if(diffTime > threshold[i]){
                     if(lightSensors[i].count < 5){
                        lightSensors[i].count++;
                     }
                     else{
                        output |= ~(maskPin[i]);
                     }
                  }
                  else{
                     lightSensors[i].count = 0;
                     output &= ~maskPin[i];
                  }

                  pinMode(pins[i],OUTPUT);
                  lightSensors[i].state = IO;
               }
               break; 
            default: 
               break; 
         }
      
      }

      if(poll(&stdin_poll,1,0)==1){
         scanf("%c",msg);
         //fprintf(stderr,"got message in encoder going to scanf:'%c'\n",msg[0]);
         switch(msg[0]){

            case 'p'://poll for the light sensor states
               MYWRITE(STDOUT_FILENO,&output,(sizeof(unsigned char)));
            break;
            default:
               fprintf(stderr,"Error in encoder function switch statment recieved:%c line:%d, file:%s\n",msg[0],__LINE__,__FILE__);
         }
      }
      nanosleep(&sleepTime,NULL);
   }
   exit(EXIT_SUCCESS);
}

/*create child that executes open source code interacting with gyoscope*/
void createLineSensorChild(int** writeToChild){
 
   pid_t pid;

   int pipeToMe[2];
   int pipeToIt[2];

   mypipe(pipeToMe);
   mypipe(pipeToIt);

   if((pid = fork()) < 0){
      perror(NULL);
      fprintf(stderr,"fork");
      exit(EXIT_FAILURE);
   }
   if(pid == 0){
      myclose(pipeToMe[0]);
      myclose(pipeToIt[1]);
      if(dup2(pipeToMe[1],STDOUT_FILENO)<0){
         fprintf(stderr,"execl failure %d in  %s",__LINE__,__FILE__);
         perror(NULL);
         exit(EXIT_FAILURE);
      }
      if(dup2(pipeToIt[0],STDIN_FILENO)<0){
         fprintf(stderr,"execl failure %d in  %s",__LINE__,__FILE__);
         perror(NULL);
         exit(EXIT_FAILURE);
      }
      lineChildFunct();
      fprintf(stderr,"encoderCHildFUnction failure %d in  %s",__LINE__,__FILE__);
      perror(NULL);
      exit(EXIT_FAILURE);
   }

   myclose(pipeToMe[1]);
   myclose(pipeToIt[0]);

   *writeToChild = malloc(sizeof(int)*2);
   (*writeToChild)[0] = pipeToMe[0];
   (*writeToChild)[1] = pipeToIt[1];
}


