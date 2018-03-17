#include "driveWheelPid.h"
#include <wiringPi.h>
#include "time.h"


#define SPEED_KD 0.01
#define SPEED_KP 1
#define SPEED_KI 10 

int pin(int index){
   switch(index){
      case FL:
         return 31;
      case FR:
         return 5;
      case BR:
         return 6;
      case BL:
         return 27;
      default:
         fprintf(stderr,"wrong index\n\n\n\n\n\n\n");
         return 15;
   }
}
long max(long A, long B){
   if(A > B)
      return A;
   return B;
}
long min(long A, long B){
   if(A > B)
      return B;
   return A;
}
void micosleep(long micoSec){

   struct timespec sleepTime;
   sleepTime.tv_nsec = mSec*1000;
   sleepTime.tv_sec = mSec/1000000;
   nanosleep(&sleepTime,NULL);
}
void driveWheelPidControl(){
   int* encoderPipe;
   char msg[1000];
   //double speedGoalMsg[4];
   //MotorMsg dirMsg[4];
   Encoder curEnco[4];
   WheelPid wheels[4];
   //long changeDis[4];

   double dt_nsec = 5000000;//5ms

   double dt = dt_nsec / 1000000; //5
   dt = dt + 0;
   struct timespec sleepTime;
   sleepTime.tv_nsec = 10000000;//5ms
   sleepTime.tv_sec = 1;

   //struct pollfd stdin_poll = {
    // .fd = STDIN_FILENO, .events = POLLIN |  POLLPRI };

   createEncoderChild(&encoderPipe);
   int wheelCmd[4][2];
   wheelCmd[0][1] = 0;
   wheelCmd[1][1] = 0;
   wheelCmd[2][1] = 0;
   wheelCmd[3][1] = 0;
   
   wheelCmd[0][0] = 0;
   wheelCmd[1][0] = 0;
   wheelCmd[2][0] = 0;
   wheelCmd[3][0] = 0;


   while(1){

      sprintf(msg,"p");
      printf("about to read encoder values\n");
      MYWRITE(encoderPipe[1],msg,sizeof(char));
      MYREAD(encoderPipe[0],curEnco,sizeof(Encoder)*4);
      printf("just read them\n");
      
      for(int i = 0; i < 4; i++){
         wheels[i].lastSpeed = wheels[i].curSpeed;
         wheels[i].curSpeed = ((double)(curEnco[i].count - wheels[i].encoderCnt )) / (dt)  ;
         wheels[i].encoderCnt = curEnco[i].count;
      }
      printf("FL:%ld, FR:%ld, BR:%ld, BL:%ld\n\n",wheels[0].encoderCnt,wheels[1].encoderCnt,wheels[2].encoderCnt,wheels[3].encoderCnt);
      /*
      for(int i = 0; i < 4; i++){
         long diff = wheels[i].encoderCnt - wheels[i].encoderGoal;
         if(diff > 200){
            wheelCmd[i][0] = min(2000,max(100,diff/100));
            wheelCmd[i][1] = 1;
         }
         else if(diff < - 200){
            wheelCmd[i][0] = min(2000,max(100,(-1*diff)/100));
            wheelCmd[i][1] = 0;
         }
      }*/
      
      for(int i = 0; i < 4; i ++){
         sprintf(msg,"echo %d=%d > /dev/servoblaster",i,wheelCmd[i][0]);
         system(msg);
         digitalWrite(pin(i),wheelCmd[i][1]);
      }
      printf("drive wheel off\n");


      nanosleep(&sleepTime,NULL);

      wheelCmd[3][0] = 2000;
      for(int i = 0; i < 4; i ++){
         sprintf(msg,"echo %d=%d > /dev/servoblaster",i,wheelCmd[i][0]);
         system(msg);
         digitalWrite(pin(i),wheelCmd[i][1]);
      }
      printf("drive wheel on\n");

      nanosleep(&sleepTime,NULL);

      wheelCmd[0][0] = 0;
      wheelCmd[1][0] = 0;
      wheelCmd[2][0] = 0;
      wheelCmd[3][0] = 0;

   }
   exit(EXIT_SUCCESS);

}
void driveWheelPidControlBAISC(){

   int* encoderPipe;
   char msg[1000];
   double speedGoalMsg[4];
   //MotorMsg dirMsg[4];
   Encoder curEnco[4];
   WheelPid wheels[4];
   long changeDis[4];

   double dt_nsec = 5000000;//5ms
   double dt = dt_nsec / 1000000; //5
   struct timespec sleepTime;
   sleepTime.tv_sec = 0;
   sleepTime.tv_nsec = dt_nsec;//5ms
   struct pollfd stdin_poll = {
     .fd = STDIN_FILENO, .events = POLLIN |  POLLPRI };

   createEncoderChild(&encoderPipe);
   int wheelCmd[4][2];
   while(1){

      sprintf(msg,"p");
      MYWRITE(encoderPipe[1],msg,sizeof(char));
      MYREAD(encoderPipe[0],curEnco,sizeof(Encoder)*4);
      for(int i = 0; i < 4; i++){
         wheels[i].lastSpeed = wheels[i].curSpeed;
         wheels[i].curSpeed = ((double)(curEnco[i].count - wheels[i].encoderCnt )) / (dt)  ;
         wheels[i].encoderCnt = curEnco[i].count;
      }
      for(int i = 0; i < 4; i++){
         long diff = wheels[i].encoderCnt - wheels[i].encoderGoal;
         if(diff > 200){
            wheelCmd[i][0] = min(2000,max(100,diff/100));
            wheelCmd[i][1] = 1;
         }
         else if(diff < - 200){
            wheelCmd[i][0] = min(2000,max(100,(-1*diff)/100));
            wheelCmd[i][1] = 0;
         }
      }

      if(poll(&stdin_poll,1,0)==1){
         scanf("%c",msg);
         switch(msg[0]){
            case 'c'://change 4 wheels speed goal
               MYREAD(STDIN_FILENO,&speedGoalMsg,sizeof(double)*4);
               for(int i = 0; i < 4; i++){
                  wheels[i].speedGoal = speedGoalMsg[i];
               }
               break;
            case 'p'://poll for the encoder states
               MYWRITE(STDOUT_FILENO,wheels,sizeof(WheelPid)*4);
               break;
            case 'm':
               MYREAD(STDIN_FILENO,&changeDis,sizeof(long)*4);
               for(int i = 0; i < 4; i ++ ){
                  wheels[i].encoderGoal += changeDis[i];
               }
            case 'r':
               for(int i = 0; i < 4;i++){
                  wheelCmd[i][0] = 0;
                  wheels[i].encoderGoal = wheels[i].encoderCnt;
               }
            default:
               fprintf(stderr,"Error in drive Wheel Pid Control function: line:%d file:%s\n",__LINE__,__FILE__);
         }
      }
      for(int i = 0; i < 4; i ++){
         sprintf(msg,"echo %d = %d > /dev/servoblaster",i,wheelCmd[i][0]);
         system(msg);
         digitalWrite(pin(i),wheelCmd[i][1]);
      }


      nanosleep(&sleepTime,NULL);
   }
   exit(EXIT_SUCCESS);

}
void driveWheelPidControlFANCY(){

   int* encoderPipe;
   char msg[1000];
   double speedGoalMsg[4];
   //MotorMsg dirMsg[4];
   Encoder curEnco[4];
   WheelPid wheels[4];
   double dt_nsec = 5000000;//5ms
   double dt = dt_nsec / 1000000; //5
   /*struct timespect sleepTime;
   sleepTime.tv_sec = 0;
   sleepTime.tv_nsec = dt_nsec;//5ms*/
   struct pollfd stdin_poll = {
     .fd = STDIN_FILENO, .events = POLLIN |  POLLPRI };

   createEncoderChild(&encoderPipe);
   while(1){

      sprintf(msg,"p");
      MYWRITE(encoderPipe[1],msg,sizeof(char));
      MYREAD(encoderPipe[0],curEnco,sizeof(Encoder)*4);
      for(int i = 0; i < 4; i++){
         wheels[i].lastSpeed = wheels[i].curSpeed;
         wheels[i].curSpeed = ((double)(curEnco[i].count - wheels[i].encoderCnt )) / (dt)  ;
         wheels[i].encoderCnt = curEnco[i].count;
      }


      if(poll(&stdin_poll,1,0)==1){
         scanf("%c",msg);
         switch(msg[0]){
            case 'c'://change 4 wheels speed goal
               MYREAD(STDIN_FILENO,&speedGoalMsg,sizeof(double)*4);
               for(int i = 0; i < 4; i++){
                  wheels[i].curSpeed = speedGoalMsg[i];
               }
               break;
            case 'p'://poll for the encoder states
               MYWRITE(STDOUT_FILENO,wheels,sizeof(WheelPid)*4);
               break;

            default:
               fprintf(stderr,"Error in drive Wheel Pid Control function: line:%d file:%s\n",__LINE__,__FILE__);
         }
      }
      //nanosleep(&sleepTime,NULL);
   }
   exit(EXIT_SUCCESS);

}
/*create child that executes open drive wheel pid control*/
void createDriveWheelChild(int** writeToChild){
 
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
     // dup2(pipeToMe[1],STDOUT_FILENO);
      dup2(pipeToIt[0],STDIN_FILENO);
      driveWheelPidControl();
      fprintf(stderr,"execl failure");
      perror(NULL);
      exit(EXIT_FAILURE);
   }

   myclose(pipeToMe[1]);
   myclose(pipeToIt[0]);

   *writeToChild = malloc(sizeof(int)*2);
   (*writeToChild)[0] = pipeToMe[0];
   (*writeToChild)[1] = pipeToIt[1];
}


