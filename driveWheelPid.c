#include "driveWheelPid.h"

#define KD 0.01
#define KP 1
#define KI .1 

#define KP_ANGLE 10
#define KI_ANGLE 0.5
#define MOTOR_FWD 0
#define MOTOR_BACK 1

#define dt_msec 5
#define dt_nsec 5000000
#define dt_sec ((double)dt_nsec)/1000000000

#define MAX_SPEED 2000
#define MIN_SPEED 300

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
void resetEncoder(int encoderPipe){
   char tempMsg[10];
   tempMsg[0] = 'r';
   tempMsg[1] = 'a';
   //reset encoder values in child encoder process
   MYWRITE(encoderPipe,tempMsg,sizeof(char)*2);

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
      wheels[i].encoderGoal = (inputGoal)*(wheels[i].curDir);
      wheels[i].curError = 0;
   }
}

/*Reads from main.c data request and responds accordingly
 * Returns 1 if there was reques (so wheels can be updated)
 * Returns 0 if no request
 * Puts input recieved into msg
 */ 
int handleInput(struct pollfd *stdin_poll,WheelPid *wheels,char *msg, int wheelCmd[][2],int *encoderPipe,enum dir *direction) {

   double inputGoal[4];

      if(poll(stdin_poll,1,0)==1){
         MYREAD(stdin_poll->fd,msg,sizeof(char));
         switch(msg[0]){
            case 'p'://poll for the encoder states
               MYWRITE(STDOUT_FILENO,wheels,sizeof(WheelPid)*4);
               break;
            case 'm'://move -- looks for encoder move amount with direction
               MYREAD(stdin_poll->fd,inputGoal,sizeof(double));
               MYREAD(stdin_poll->fd,direction,sizeof(enum dir));
               printf("inputGoal:%g \t direction:%d\n\n\n",inputGoal[0],(int)*direction);
               updateWheels(wheels,inputGoal[0],*direction);
               resetEncoder(encoderPipe[1]);
//               char tempMsg[10];
 //              tempMsg[0] = 'r';
  //             tempMsg[1] = 'a';
               //reset encoder values in child encoder process
   //            MYWRITE(encoderPipe[1],tempMsg,sizeof(char)*2);
               break;
            case 'r'://reset
               for(int i = 0; i < 4;i++){
                  wheelCmd[i][0] = 0;
                  wheelCmd[i][1] = 0;
                  wheels[i].curError = 0;
                  wheels[i].encoderGoal = wheels[i].encoderCnt;
               }
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
       if(wheels[i].curDir >0){
         wheels[i].encoderCnt += (curEnco[i].count-wheels[i].encoderCnt);
       }
       else{
         wheels[i].encoderCnt -= (curEnco[i].count+wheels[i].encoderCnt);
       }
    }
}
//this takes the pow that is a double that can be negitive or postiive and then direction and turns wheelCmd with the correct power and dir output to be controled
void limitPowerWheels(double *pow,int wheelCmd[][2],enum dir direction){
   int dirValueTemp = 0;
   //pow is valid from -2000 to 2000 if neg is then turned postive with dirValueTemp 1
   //printf("power:%10.10f \t direction:%d\n",*pow,direction);

   //if pow is negitive invert it and set dirValue Temp = 1
   if((*pow) < 0){
      dirValueTemp = 1;
      *pow = *pow * -1;
   }
   assert(*pow>=0);
   //if greater then maxumim speed then set it to max speed
   if((*pow)>MAX_SPEED){
      *pow = MAX_SPEED;
   }
   //if less then minamum speed then set it to zero we are done
   if((*pow) < MIN_SPEED){
      *pow = 0;
      //printf("tooLowSpeed\n");
   }
  
   
   switch(direction){
      case Backward:
      case Forward:
         for(int i = 0; i < 4;i++){
            wheelCmd[i][0] = (int)abs(2000*dirValueTemp - *pow);//default state is fowards
            wheelCmd[i][1] = dirValueTemp;
         }
      break;
         /*for(int i = 0; i < 4;i++){
            wheelCmd[i][0] = (int)abs(2000*(dirValueTemp) - *pow);//default state is backwords
            wheelCmd[i][1] = dirValueTemp; 
         }
      break;*/
      case Left:
      //need to change 1-dirValueTemp this may not work
            //FL
            wheelCmd[0][0] = (int)abs(2000*(1-dirValueTemp) - *pow);//defualt backwords
            wheelCmd[0][1] = 1-dirValueTemp;

            //FR
            wheelCmd[1][0] = (int)abs(2000*(dirValueTemp) - *pow);//default fowards
            wheelCmd[1][1] = dirValueTemp;

            //BR
            wheelCmd[2][0] = (int)abs(2000*(dirValueTemp) - *pow);//default fowards
            wheelCmd[2][1] = dirValueTemp;

            //BL
            wheelCmd[3][0] = (int)abs(2000*(1-dirValueTemp) - *pow);//default backwords
            wheelCmd[3][1] = 1-dirValueTemp;
      break;
      case Right:
            //FL
            wheelCmd[0][0] = (int)abs(2000*(dirValueTemp) - *pow);//defualt fowards 
            wheelCmd[0][1] = dirValueTemp;

            //FR
            wheelCmd[1][0] = (int)abs(2000*(1-dirValueTemp) - *pow);//default backwards
            wheelCmd[1][1] = 1-dirValueTemp;

            //BR
            wheelCmd[2][0] = (int)abs(2000*(1-dirValueTemp) - *pow);//default backwards
            wheelCmd[2][1] = 1-dirValueTemp;

            //BL
            wheelCmd[3][0] = (int)abs(2000*(dirValueTemp) - *pow);//default fowords
            wheelCmd[3][1] = dirValueTemp;
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
double angleToValue(float angle){
   assert(angle >= 0);
   if(abs(angle) > 90){
      double diff = 0;
      diff = abs(angle) - 180;
      return (diff * ((angle>0)?1:-1));
   }
   return angle;
}
/* corrects for what value of angle we have acumulated over the trip*/
void anglePIDControl(WheelPid *wheels, int wheelCmd[][2],enum dir direction,ImuDir *curImu) {

   double error_new  = angleToValue(curImu->Rx);
   double pow;
   curImu->curError += error_new;
   //using wheels[0] becaues all wheels have the same curError. this will probally change 
   pow = KP_ANGLE*error_new + KI_ANGLE*dt_sec*(curImu->curError);
   //Error_new will be negitice if it is turning slightly to the left going fowards ie counter clockwise
   printf("angle: %g angle correction power:%g\n",error_new,pow);
   switch(direction){
      case Forward:
         wheelCmd[FR][0] -= pow*((wheelCmd[FR][1]==0)?1:-1);
         wheelCmd[BR][0] -= pow*((wheelCmd[BR][1]==0)?1:-1);
         wheelCmd[FL][0] += pow*((wheelCmd[FL][1]==0)?1:-1);
         wheelCmd[BL][0] += pow*((wheelCmd[BL][1]==0)?1:-1);
      break;
      case Backward:
         wheelCmd[FR][0] += pow*((wheelCmd[FR][1]==0)?1:-1);
         wheelCmd[BR][0] += pow*((wheelCmd[BR][1]==0)?1:-1);
         wheelCmd[FL][0] -= pow*((wheelCmd[FL][1]==0)?1:-1);
         wheelCmd[BL][0] -= pow*((wheelCmd[BL][1]==0)?1:-1);
      break;
      case Left:
      case Right:
      //TODO
      break;
   }
   for(int i = 0; i < 4;i++){
      if(wheelCmd[i][0] > MAX_SPEED){
         wheelCmd[i][0] = MAX_SPEED;
      }
      else if(wheelCmd[i][0] < MIN_SPEED){
         wheelCmd[i][0] = 0;
      }
   }

}
/* logic that interprets WheelPid struct data and outputs wheelCmd to control the motors physically
 */
int distancePIDControl(WheelPid *wheels, int wheelCmd[][2],enum dir direction) {
   int indexEncoderToUse = 0;
   long encoderToUse = wheels[indexEncoderToUse].encoderCnt;
   //printf("encoderToUse:%ld\n",encoderToUse);
   long error_new;
   /* this grabs the wheels that moved the least. THis is crazy sam's idea for best PI control change later if foundn he was just dumb
    */
   for(int i = 1; i < 4; i++){
      if(abs(wheels[i].encoderCnt) < abs(encoderToUse)){
         encoderToUse = wheels[i].encoderCnt;
         indexEncoderToUse = i;
      }
   }
   //printf("FL:%7ld FR:%7ld BR:%7ld BLL%7ld\n",wheels[0].encoderCnt,wheels[1].encoderCnt,wheels[2].encoderCnt,wheels[3].encoderCnt);
   // Calculate errors:
   error_new = wheels[indexEncoderToUse].encoderGoal - encoderToUse;
   //printf("wheels[indexEncoderToUse].encoderGoal:%ld encoderToUse:%ld\n",wheels[indexEncoderToUse].encoderGoal,encoderToUse);
   //TODO add for BACK and LEFT and RIGHT thsi is only for FOWARD!!!

   // PI control
   for(int i = 0; i< 4;i++){
      wheels[i].curError += error_new;
   }
   double pow;
   //using wheels[0] becaues all wheels have the same curError. this will probally change 
   pow = KP*error_new + KI*dt_sec*(wheels[0].curError);
   //printf("error_new:%ld CurError%g\n",error_new,wheels[0].curError);
   
   limitPowerWheels(&pow,wheelCmd,direction);
   for(int i = 0; i < 4;i++){
      wheels[i].curDir = (wheelCmd[i][1]==0)?1:-1;
   }

   if(wheelCmd[0][1] == 1){
    //  printf("going backwords\n");
   }

   if(pow < 10){
   //   printf("turning everything off!!\n");
      for(int i = 0 ; i < 4; i++){
         wheelCmd[i][0] = 0;
         wheelCmd[i][1] = 0;
         wheels[i].curError = 0;
         wheels[i].curDir = 1;
         wheels[i].encoderCnt = 0;
         wheels[i].encoderGoal = 0;
      }
      return 1;
   }
   return 0;
}
void resetImu(int  imuPipe){
   char msg[] = "r";
   MYWRITE(imuPipe,msg,sizeof(char));
}
void updateImuStatus(ImuDir *curImu){
   struct pollfd IMU_poll = {
     .fd = STDIN_FILENO, .events = POLLIN |  POLLPRI };
   if(poll(&IMU_poll,1,0)==1){
      scanf("%g %g %g\n",&(curImu->Rx),&(curImu->Ry),&(curImu->Rz));
   }
}

void driveWheelPidControl(){

   int* encoderPipe;
   int imuPipe;
   int stdInPipe;
   char msg[1000];
   //MotorMsg dirMsg[4];
   Encoder curEnco[4];
   WheelPid wheels[4];
   enum dir direction = Forward;
   ImuDir curImu;
   curImu.Rx = curImu.Ry = curImu.Rz = curImu.curError = 0;
   //flag that is either 1 or 0. 0 means the encoders have not been reset recently and we have moved so we will need to reset once we finish moving
   //1 means that we have already reset the encoders and dont need to keep resetting them 
   char encoderReset = 0;
   struct timespec sleepTime;
   sleepTime.tv_sec = 0;
   sleepTime.tv_nsec = dt_nsec;//5ms

   createEncoderChild(&encoderPipe);
   printf("pipe:%d, %d\n\n",encoderPipe[0],encoderPipe[1]);


   createAcceleromoterChild(&stdInPipe,&imuPipe);
   //stdInPipe = STDIN_FILENO;
   //imuPipe = STDOUT_FILENO;


   struct pollfd stdin_poll = {
     .fd = stdInPipe, .events = POLLIN |  POLLPRI };

   int wheelCmd[4][2];
   for(int i = 0 ; i < 4; i++){
      wheelCmd[i][0] = 0;
      wheelCmd[i][1] = 0;
      wheels[i].curError = 0;
      wheels[i].curDir = 1;
      wheels[i].encoderCnt = 0;
      wheels[i].encoderGoal = 0;
   }

//this is beacues the IMU takes a coucple of seconds to start working
   scanf("%g %g %g\n",&(curImu.Rx),&(curImu.Ry),&(curImu.Rz));
   resetImu(imuPipe);

   fprintf(stderr,"iipe:%d, %d\n\n",encoderPipe[0],encoderPipe[1]);
   while(1){

      updateEncoderStatus(encoderPipe, curEnco, wheels,wheelCmd);
      updateImuStatus(&curImu);

      if(handleInput(&stdin_poll,wheels, msg, wheelCmd,encoderPipe,&direction)) {
        writeToWheels(wheelCmd); 
      }
      //if encoder reset = 1 then we have already reset the encoders and are not moving again. this is to repeat encoder resetting actions
      if(distancePIDControl(wheels,wheelCmd,direction)==1 && (encoderReset = 0)){
        resetEncoder(encoderPipe[1]);
        encoderReset = 1;

      }
      //if else then we are moving the wheels meaning the encoders need to be reset eventually
      else{
         encoderReset = 0;
      }

      //if case so that is we are temporarly stop dont keep acumulating error in angle
      if(encoderReset == 0){
         anglePIDControl(wheels,wheelCmd,direction,&curImu);
      }
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


