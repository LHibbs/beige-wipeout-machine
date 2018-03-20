#include "driveWheelPid.h"
#include <wiringPi.h>
#include "time.h"

#define KD 0.01
#define KP 100
#define KI 25 

#define MOTOR_FWD 0
#define MOTOR_BACK 1

#define dt_msec 5
#define dt_sec ((double)5)/1000
#define dt_nsec 5000000

#define MAX_SPEED 2000
#define MIN_SPEED 100

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
void micosleep(long mSec){
   struct timespec sleepTime;
   sleepTime.tv_nsec = mSec*1000;
   sleepTime.tv_sec = mSec/1000000;
   nanosleep(&sleepTime,NULL);
}



void writeToWheels(int wheelCmd[][2]) {
   char msg[100]; 
      for(int i = 0; i < 4; i ++){
         sprintf(msg,"echo %d=%d > /dev/servoblaster",i,wheelCmd[i][0]);
         system(msg);
         digitalWrite(pin(i),wheelCmd[i][1]);
      }
}

void updateWheels(WheelPid *wheels,double inputGoal,enum dir direction){
   switch(direction){
      case Forward:
         wheels[FL].curDir = 1;
         wheels[FR].curDir = 1;
         wheels[BR].curDir = 1;
         wheels[BL].curDir = 1;
      break;
      case Backward:
         wheels[FL].curDir = -1;
         wheels[FR].curDir = -1;
         wheels[BR].curDir = -1;
         wheels[BL].curDir = -1;
      break;
      case Right:
         wheels[FL].curDir = 1;
         wheels[FR].curDir = -1;
         wheels[BR].curDir = 1;
         wheels[BL].curDir = -1;
      break;
      case Left:
         wheels[FL].curDir = -1;
         wheels[FR].curDir = 1;
         wheels[BR].curDir = -1;
         wheels[BL].curDir = 1;
      break;
      default:
         assert(0);
   }
   for(int i = 0; i < 4; i++){
      //note outside this function we set child encoder proess encoder counts to 0
      wheels[i].encoderCnt = 0;
      wheels[i].encoderGoal = (inputGoal)*wheels[i].curDir;
      wheels[i].curError = 0;
   }
}
/*Reads from main.c data request and responds accordingly
 * Returns 1 if there was reques (so wheels can be updated)
 * Returns 0 if no request
 * Puts input recieved into msg
 */ 
int handleInput(WheelPid *wheels,char *msg, int wheelCmd[][2],int *encoderPipe,enum dir *direction) {

   double inputGoal[4];
   struct pollfd stdin_poll = {
     .fd = STDIN_FILENO, .events = POLLIN |  POLLPRI };

      if(poll(&stdin_poll,1,0)==1){
         MYREAD(STDIN_FILENO,msg,sizeof(char));
         switch(msg[0]){
            case 'p'://poll for the encoder states
               MYWRITE(STDOUT_FILENO,wheels,sizeof(WheelPid)*4);
               break;
            case 'm'://move -- looks for encoder move amount with direction
               MYREAD(STDIN_FILENO,inputGoal,sizeof(double));
               MYREAD(STDIN_FILENO,direction,sizeof(enum dir));
               printf("inputGoal:%g \t direction:%d\n\n\n",inputGoal[0],(int)*direction);
               updateWheels(wheels,inputGoal[0],*direction);
               
               char tempMsg[10];
               tempMsg[0] = 'r';
               tempMsg[1] = 'a';
               //reset encoder values in child encoder process
               MYWRITE(encoderPipe[1],tempMsg,sizeof(char)*2);
               break;
            case 'r'://reset
               for(int i = 0; i < 4;i++){
                  wheelCmd[i][0] = 0;
                  wheelCmd[i][1] = 0;
                  wheels[i].curError = 0;
                  wheels[i].encoderGoal = wheels[i].encoderCnt;
               }
               MYWRITE(encoderPipe[1],tempMsg,sizeof(char)*2);
               break;
            case 'q'://quit
               for(int i = 0; i < 4;i++){
                  wheelCmd[i][0] = 0;
                  wheelCmd[i][1] = 0;
               }
               writeToWheels(wheelCmd); 
               exit(EXIT_SUCCESS); 
            default:
               fprintf(stderr,"Error in drive Wheel Pid Control function: line:%d file:%s\n",__LINE__,__FILE__);
         }
         return 1; 
      }
      return 0; 
}

void updateEncoderStatus(int *encoderPipe, Encoder *curEnco, WheelPid *wheels, int wheelCmd[][2]) {
    char msg[10]; 
    sprintf(msg,"p");
    MYWRITE(encoderPipe[1],msg,sizeof(char));
    MYREAD(encoderPipe[0],curEnco,sizeof(Encoder)* 4);
    for(int i = 0; i < 4; i++){
       wheels[i].encoderCnt = curEnco[i].count;
    }
}
//this takes the pow that is a double that can be negitive or postiive and then direction and turns wheelCmd with the correct power and dir output to be controled
void limitPowerWheels(double pow,int wheelCmd[][2],enum dir direction){
   int dir = 0;
   printf("power:%10.10f \t direction:%d\n",pow,direction);
   if(abs(pow)>MAX_SPEED){
      pow = MAX_SPEED;
   }
   if(abs(pow)<MIN_SPEED){
      pow = 0;
      printf("tooLowSpeed\n");
   }
   if(pow<0){
      dir = 1;
      pow = pow*-1;
   }
   //printf("pow modified:%g\n",pow);
   
   switch(direction){
      case Forward:
         for(int i = 0; i < 4;i++){
            wheelCmd[i][0] = (int)abs(2000*dir - pow);//default state is fowards
            wheelCmd[i][1] = dir;
         }
      break;
      case Backward:
         for(int i = 0; i < 4;i++){
            wheelCmd[i][0] = (int)abs(2000*(1-dir) - pow);//default state is backwords
            wheelCmd[i][1] = 1-dir; 
         }
      break;
      case Left:
            //FL
            wheelCmd[0][0] = (int)abs(2000*(1-dir) - pow);//defualt backwords
            wheelCmd[0][1] = 1-dir;

            //FR
            wheelCmd[1][0] = (int)abs(2000*(dir) - pow);//default fowards
            wheelCmd[1][1] = dir;

            //BR
            wheelCmd[2][0] = (int)abs(2000*(dir) - pow);//default fowards
            wheelCmd[2][1] = dir;

            //BL
            wheelCmd[3][0] = (int)abs(2000*(1-dir) - pow);//default backwords
            wheelCmd[3][1] = 1-dir;
      break;
      case Right:
            //FL
            wheelCmd[0][0] = (int)abs(2000*(dir) - pow);//defualt fowards 
            wheelCmd[0][1] = dir;

            //FR
            wheelCmd[1][0] = (int)abs(2000*(1-dir) - pow);//default backwards
            wheelCmd[1][1] = 1-dir;

            //BR
            wheelCmd[2][0] = (int)abs(2000*(1-dir) - pow);//default backwards
            wheelCmd[2][1] = 1-dir;

            //BL
            wheelCmd[3][0] = (int)abs(2000*(dir) - pow);//default fowords
            wheelCmd[3][1] = dir;
      break;
      default:
         assert(0);
   }
   //this is for testing should be able to remove with no problems
   for(int i = 0 ; i< 4;i++){
    //  printf("[i][0]:%d  [i][1]:%d\n",wheelCmd[i][0],wheelCmd[i][1]);
      assert(wheelCmd[i][0]>=0);
      assert(wheelCmd[i][0]<=2000);
      assert(wheelCmd[i][1]==0||wheelCmd[i][1]==1);
   }

}

/* logic that interprets WheelPid struct data and outputs wheelCmd to control the motors physically
 */
void distancePIDControl(WheelPid *wheels, int wheelCmd[][2],enum dir direction) {
   int indexEncoderToUse = 0;
   long encoderToUse = wheels[indexEncoderToUse].encoderCnt;;
   long error_new;
   /* this grabs the wheels that moved the least. THis is crazy sam's idea for best PI control change later if foundn he was just dumb
    */
   for(int i = 1; i < 4; i++){
      if(direction == Forward){
         if(wheels[i].encoderCnt < encoderToUse){
            encoderToUse = wheels[i].encoderCnt;
            encoderToUse = i;
         }
      }
      if(direction == Backward){
         if(wheels[i].encoderCnt > encoderToUse){
            encoderToUse = wheels[i].encoderCnt;
            encoderToUse = i;
         }
      }
      //TODO add RIGHT and LEFT

   }
   // Calculate errors:
   error_new = wheels[indexEncoderToUse].encoderGoal - encoderToUse;
   printf("wheels[indexEncoderToUse].encoderGoal:%ld encoderToUse:%ld\n",wheels[indexEncoderToUse].encoderGoal,encoderToUse);
   //TODO add for BACK and LEFT and RIGHT thsi is only for FOWARD!!!

   // PI control
   for(int i = 0; i< 4;i++){
      wheels[i].curError += error_new;
   }
   double pow;
   //using wheels[0] becaues all wheels have the same curError. this will probally change 
   pow = KP*error_new + KI*dt_sec*(wheels[0].curError);
   printf("error_new:%ld CurError%g\n",error_new,wheels[0].curError);
   
   limitPowerWheels(pow,wheelCmd,direction);
   if(pow <1){
    for(int i = 0 ; i < 4; i++){
      wheelCmd[i][0] = 0;
      wheelCmd[i][1] = 0;
      wheels[i].curError = 0;
      wheels[i].curDir = 1;
      wheels[i].encoderCnt = 0;
      wheels[i].encoderGoal = 0;
   }
   }
}

void driveWheelPidControl(){

   int* encoderPipe;
   char msg[1000];
   //MotorMsg dirMsg[4];
   Encoder curEnco[4];
   WheelPid wheels[4];
   enum dir direction = Forward;


   struct timespec sleepTime;
   sleepTime.tv_sec = 0;
   sleepTime.tv_nsec = dt_nsec;//5ms

   createEncoderChild(&encoderPipe);
   int wheelCmd[4][2];
   for(int i = 0 ; i < 4; i++){
      wheelCmd[i][0] = 0;
      wheelCmd[i][1] = 0;
      wheels[i].curError = 0;
      wheels[i].curDir = 1;
      wheels[i].encoderCnt = 0;
      wheels[i].encoderGoal = 0;
   }

   while(1){

      updateEncoderStatus(encoderPipe, curEnco, wheels,wheelCmd);

      if(handleInput(wheels, msg, wheelCmd,encoderPipe,&direction)) {
        writeToWheels(wheelCmd); 
      }
      distancePIDControl(wheels,wheelCmd,direction);
      writeToWheels(wheelCmd); 

      nanosleep(&sleepTime,NULL);
   }
   exit(EXIT_SUCCESS);

}

/*create child that executes open drive wheel pid control*/
pid_t createDriveWheelChild(int** writeToChild){
 
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
   return pid;
}


