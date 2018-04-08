#include "driveWheelPid.h"

#define KD 0.01
#define KP 1
#define KI 0
//#define KI .1 

#define KP_ANGLE 512
#define KI_ANGLE 0
#define MOTOR_FWD 0
#define MOTOR_BACK 1

#define dt_msec 5
#define dt_nsec 5000000
#define dt_sec ((double)dt_nsec)/1000000000

#define MAX_SPEED 1
#define MIN_SPEED .05

#define FWD_BIAS 80
#define BACK_BIAS 103 
#define LEFT_BIAS 0 
#define RIGHT_BIAS 0 

//index 1: 
//0 foward
//1 right 
//2 down
//3 left 

//numbers are: 
//0 is FL 
//1 is FR
//2 is BR
//3 is BL

//if its drifting clockwise, reduce power to these motors
const int clockwise[4][2]  = {{0,3}, {0,1}, {1,2} , {2,3}};
//if its drifting counterclockwise, reduce these 
const int counterwise[4][2]  = {{1,2}, {2,3}, {2,3} , {0,4}};



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

   /*if(!(wheelCmd[FL][0] == 0 || wheelCmd[FR][0] == 0)) {
   printf("Finial wheel Power: , FL: , %d , FR: , %d , BR: , %d , BL , %d\n", \
         wheelCmd[FL][0],wheelCmd[FR][0], wheelCmd[BR][0], wheelCmd[BL][0]);
   }*/
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

void gradualStartUp(WheelPid *wheels,int wheelCmd[][2],enum dir direction){
   struct timespec sleepTime;
   sleepTime.tv_sec = 0;
   sleepTime.tv_nsec = 10000000;//10ms
   int wheelDir = 0;
   if(direction == Backward || direction == Left){
      wheelDir = 1;
   }
   //speed at 100 and increment by 50 is .38 seconds
   for(int speed = 50; speed <= 2000;speed = speed + 50){
      switch(direction)
      {
         case Forward:
         case Backward:
            for(int i = 0; i < 4; i++){
               wheelCmd[i][0] = abs(2000*wheelDir - speed);
               wheelCmd[i][1] = wheelDir;
            }
         break;
         case Left:
         case Right:
            wheelCmd[FL][0] = abs(2000*wheelDir - speed);
            wheelCmd[FL][1] = wheelDir;

            wheelCmd[FR][0] = abs(2000*((wheelDir)?0:1) - speed);
            wheelCmd[FR][1] = (wheelDir)?0:1;

            wheelCmd[BR][0] = abs(2000*wheelDir - speed);
            wheelCmd[BR][1] = wheelDir;

            wheelCmd[BL][0] = abs(2000*((wheelDir)?0:1) - speed);
            wheelCmd[BL][1] = (wheelDir)?0:1;
         break;
         default:
            assert(0);
         break;
      }
      writeToWheels(wheelCmd);
      nanosleep(&sleepTime,NULL);
   }
}

/*Reads from main.c data request and responds accordingly
 * Returns 1 if there was reques (so wheels can be updated)
 * Returns 0 if no request
 * Puts input recieved into msg
 */ 
int handleInput(struct pollfd *stdin_poll,WheelPid *wheels,char *msg, int wheelCmd[][2],int *encoderPipe,enum dir *direction, Command *command,int *startUpPhase) {

   double inputGoal[4];

      if(poll(stdin_poll,1,0)==1){
         MYREAD(stdin_poll->fd,msg,sizeof(char));
         switch(msg[0]){
             //not sure if p is ever being called? 
            case 'p'://poll for the encoder states
               MYWRITE(STDOUT_FILENO,wheels,sizeof(WheelPid)*4);
               break;
            case 'm'://move -- looks for encoder move amount with direction
               MYREAD(stdin_poll->fd,inputGoal,sizeof(double));
               MYREAD(stdin_poll->fd,direction,sizeof(enum dir));
               fprintf(stderr,"inputGoal:%g \t direction:%d\n\n\n",inputGoal[0],(int)*direction);
               updateWheels(wheels,inputGoal[0],*direction);
               //also doesn't seem like this needs to be here 
               resetEncoder(encoderPipe[1]);
               *startUpPhase = 1;
               //gradualStartUp(wheels,wheelCmd,*direction);
               command->cmdType = Distance; 
               command->encoderDist = *inputGoal; 

               printf("in handle input\n");
               for(int j = 0 ; j < 4;j++){
                  printf("wheelCmd[%d][0] = %d wheelCmd[%d,[1]] = %d\n",j,wheelCmd[j][0],j,wheelCmd[j][1]);
               }
               break;
            case 'r'://reset
               for(int i = 0; i < 4;i++){
                  wheelCmd[i][0] = 0;
                  wheelCmd[i][1] = 0;
                  wheels[i].curError = 0;
                  wheels[i].encoderGoal = wheels[i].encoderCnt;
               }
               break;
            case 'l':
               MYREAD(stdin_poll->fd,inputGoal,sizeof(double));
               MYREAD(stdin_poll->fd,direction,sizeof(enum dir));
               updateWheels(wheels,inputGoal[0],*direction);
               resetEncoder(encoderPipe[1]);
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

void updateEncoderStatus(int *encoderPipe, Encoder *curEnco, WheelPid *wheels) {
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
void limitPowerWheels(WheelPid *wheels,int wheelCmd[][2],enum dir direction,int taskComplete){
   //pow is valid from -2000 to 2000 if neg is then turned postive with dirValueTemp 1
   //printf("power:%10.10f \t direction:%d\n",*pow,direction);


   for(int i = 0 ; i < 4; i ++ ){

       //if pow is negitive invert it and set dirValue Temp = 1
       if((wheels[i].pow) < 0){
          wheels[i].tempCurDir = 1;
          wheels[i].pow *= -1;
       }
       assert(wheels[i].pow >=0);
       //if greater then maxumim speed then set it to max speed
       if((wheels[i].pow)>MAX_SPEED){
          wheels[i].pow = MAX_SPEED;
       }
       //if less then minamum speed then set it to zero we are done
       if((wheels[i].pow) < MIN_SPEED){
           if(taskComplete == 1){
              wheels[i].pow  = 0;
           }
           else{
               wheels[i].pow = MIN_SPEED;
           }
       }
       wheels[i].powToWheels = wheels[i].pow * 2000;
   }
  
   
   switch(direction){
      case Backward:
      case Forward:
         for(int i = 0; i < 4;i++){
            wheelCmd[i][0] = (int)abs(2000*wheels[i].tempCurDir - (wheels[i].powToWheels));//default state is fowards
            wheelCmd[i][1] = wheels[i].tempCurDir;
         }
      break;
      case Left:
      case Right:
      //need to change 1-dirValueTemp this may not work
            //FL
            wheelCmd[FL][0] = (int)abs(2000*(1-wheels[FL].tempCurDir) - wheels[FL].powToWheels);//defualt backwords
            wheelCmd[FL][1] = 1-wheels[FL].tempCurDir;

            //FR
            wheelCmd[FR][0] = (int)abs(2000*(wheels[FR].tempCurDir) - wheels[FR].powToWheels);//default fowards
            wheelCmd[FR][1] = wheels[FR].tempCurDir;

            //BR
            wheelCmd[BR][0] = (int)abs(2000*(wheels[BR].tempCurDir) - wheels[BR].powToWheels);//default fowards
            wheelCmd[BR][1] = wheels[BR].tempCurDir;

            //BL
            wheelCmd[BL][0] = (int)abs(2000*(1-wheels[BL].tempCurDir) - wheels[BL].powToWheels);//default backwords
            wheelCmd[BL][1] = 1-wheels[BL].tempCurDir;
      break;
      default:
         assert(0);
   }
   for(int i = 0 ; i< 4;i++){
      printf("wheelCmd[%d][0] = %d wheelCmd[%d,[1]] = %d\n",i,wheelCmd[i][0],i,wheelCmd[i][1]);
      assert(wheelCmd[i][0]>=0);
      assert(wheelCmd[i][0]<=2000);
      assert(wheelCmd[i][1]==0||wheelCmd[i][1]==1);

      wheels[i].curDir = (wheelCmd[i][1]==0)?1:-1;
   }

}
double abs_double(double a){
   return (a<0)?-1*a:a;
}
double angleToValue(float angle){
   assert(angle >= 0);
   if(abs(angle) > 90){
      double diff = 0;
      diff = abs_double(angle) - 180;
      return (diff * ((angle>0)?1.0:-1.0));
   }
   return angle;
}
/* corrects for what value of angle we have acumulated over the trip*/
void straightBias(WheelPid *wheels,enum dir direction,double powerMult) {
   double pow;
   switch(direction){
      case Forward:
         pow = FWD_BIAS;
      break;
      case Backward:
         pow = -BACK_BIAS;
      break;
      case Left:
         pow = -LEFT_BIAS;
      break;
      case Right:
         pow = RIGHT_BIAS;
      break;
      default:
         assert(0);
      break;
   }
   pow /= 2000;



   //if abs(powerMult) is less then 1 then we are adding and subtracting so reduce bias by factor of two
   if(abs(powerMult) >= .99){
      pow /= 2;
   }

   switch(direction){
      case Forward:
         wheels[FL].pow = powerMult - (powerMult)*pow;
         wheels[BL].pow = powerMult - (powerMult)*pow;

         wheels[FR].pow = powerMult + (powerMult)*pow;
         wheels[BR].pow = powerMult + (powerMult)*pow;

      break;
      case Backward:

         wheels[FL].pow = powerMult + (powerMult)*pow;
         wheels[BL].pow = powerMult + (powerMult)*pow;

         wheels[FR].pow = powerMult - (powerMult)*pow;
         wheels[BR].pow = powerMult - (powerMult)*pow;

      break;
      case Left:
      case Right:
      //TODO
      break;
   }
   for(int i = 0; i < 4;i++){
      if(wheels[i].pow > MAX_SPEED){
         wheels[i].pow = MAX_SPEED;
      }
      else if(wheels[i].pow < MIN_SPEED){
         wheels[i].pow = 0;
      }
   }
}
/* logic that outputs a double indicating wheel power multiplier for all 4 wheels.
 * Used to speed up and slow down at beginning and end. 
 */
double distancePIDControl(WheelPid *wheels, enum dir direction, int * startupPhase, double lastDistPowVal) {

   int indexEncoderToUse = 0;
       long error_new;
   long encoderToUse = wheels[indexEncoderToUse].encoderCnt;
  
    if(*startupPhase) {
        lastDistPowVal += .01; 
        
        if( lastDistPowVal >= 1) {
            *startupPhase = 0; 
        } 
        
        return lastDistPowVal;
    }
    else {
       // this grabs the wheels that moved the least. THis is crazy sam's idea for best PI control change later if foundn he was just dumb
       for(int i = 1; i < 4; i++){
          if(abs(wheels[i].encoderCnt) < abs(encoderToUse)){
             encoderToUse = wheels[i].encoderCnt;
             indexEncoderToUse = i;
          }
       }
       // Calculate errors:
       error_new = wheels[indexEncoderToUse].encoderGoal - encoderToUse;
       //TODO add for BACK and LEFT and RIGHT thsi is only for FOWARD!!!

       // PI control
       for(int i = 0; i< 4;i++){
          wheels[i].curError += error_new;
       }

   }

   double pow;
   //using wheels[0] becaues all wheels have the same curError. this will probally change 
   pow = (KP*error_new + KI*dt_sec*(wheels[0].curError) )/ 2000;
   if(pow >  1 ){
       pow = 1;
   }
   else if(pow < -1){
       pow = -1;
   }
   return pow;

} 


void resetImu(int  imuPipe){
   char msg[] = "r";
   MYWRITE(imuPipe,msg,sizeof(char));
}
void updateImuStatus(ImuDir *curImu){
   struct pollfd IMU_poll = {
     .fd = STDIN_FILENO, .events = POLLIN |  POLLPRI };
   while(poll(&IMU_poll,1,0)==1){
      scanf("%g %g %g\n",&(curImu->Rx),&(curImu->Ry),&(curImu->Rz));
   }
}

int isTaskComplete(WheelPid *wheelPid, Command *command) {

    if(command->cmdType == Distance) {
        if(wheelPid[0].encoderCnt >= command->encoderDist) 
            return 1; 
    } 

    return 0; 
} 
void resetWheels(int wheelCmd[][2], WheelPid *wheels) {

   printf("reseting wheels!\n");
   for(int i = 0 ; i < 4; i++){
      wheelCmd[i][0] = 0;
      wheelCmd[i][1] = 0;
      wheels[i].curError = 0;
      wheels[i].curDir = 1;
      wheels[i].encoderCnt = 0;
      wheels[i].encoderGoal = 0;
      wheels[i].pow = 0; 
   }

}

int isValInArray(int val, int *arr, int size){
    int i;
    for (i=0; i < size; i++) {
        if (arr[i] == val)
            return 1;
    }
    return 0;
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
   struct timeval *before,*after;
   double timer_u, timer_m;
   struct timespec sleepTime;
   //not sure if this is right 
   int startUpPhase = 0;
   Command command; 
   sleepTime.tv_sec = dt_sec;
   sleepTime.tv_nsec = dt_nsec;//5ms

   double distancePowerMult = 0; 

   before = malloc(sizeof(struct timespec));
   after = malloc(sizeof(struct timespec));

   createEncoderChild(&encoderPipe);
   printf("pipe:%d, %d\n\n",encoderPipe[0],encoderPipe[1]);


   createAcceleromoterChild(&stdInPipe,&imuPipe);
   //stdInPipe = STDIN_FILENO;
   //imuPipe = STDOUT_FILENO;


   struct pollfd stdin_poll = {
     .fd = stdInPipe, .events = POLLIN |  POLLPRI };

   int wheelCmd[4][2];

   resetWheels(wheelCmd, wheels); 
   writeToWheels(wheelCmd); 
//this is beacues the IMU takes a coucple of seconds to start working
   //TODO change these values but for testing making them small
   for(int i =0;i < 3; i++){
      scanf("%g %g %g\n",&(curImu.Rx),&(curImu.Ry),&(curImu.Rz));
   }
   resetImu(imuPipe);
   for(int i =0;i < 0; i++){
      scanf("%g %g %g\n",&(curImu.Rx),&(curImu.Ry),&(curImu.Rz));
      printf("%g %g %g\n",(curImu.Rx),(curImu.Ry),(curImu.Rz));
   }
   fprintf(stderr, "IMU READY!!\n");

   resetWheels(wheelCmd, wheels); 
   while(1){
      gettimeofday(before,NULL);
      

      printf("before updating encoders!\n");
      for(int j = 0 ; j < 4;j++){
         printf("wheelCmd[%d][0] = %d wheelCmd[%d,[1]] = %d\n",j,wheelCmd[j][0],j,wheelCmd[j][1]);
      }

      //sets current encoder count of wheels
      updateEncoderStatus(encoderPipe, curEnco, wheels);
      updateImuStatus(&curImu);

      printf("after updating encoder and IMU\n");
      for(int j = 0 ; j < 4;j++){
         printf("wheelCmd[%d][0] = %d wheelCmd[%d,[1]] = %d\n",j,wheelCmd[j][0],j,wheelCmd[j][1]);
      }

      if(handleInput(&stdin_poll,wheels, msg, wheelCmd,encoderPipe,&direction,&command,&startUpPhase)) {
          // a new command was issued: so now it is "active" 
          encoderReset = 0; 
          resetWheels(wheelCmd, wheels); 
      }

      //if encoder reset = 1 then we have already reset the encoders and are not moving again. this is to repeat encoder resetting actions
      if(encoderReset == 0) {
        //change: this no longer controlls when it ends, it only control pid before then ... 
        distancePowerMult = distancePIDControl(wheels,direction,&startUpPhase,distancePowerMult);
        
        //takes distancePowerMUlt and translates that to pow for each wheels control depending on direction adding bias 
        straightBias(wheels, direction, distancePowerMult);
        

        //anglePIDControl(wheels,wheelCmd,direction,&curImu);
        //the line here is whatever the thing is that controlls when its done. Depending on the command being listened to this might be different things. 
        //for example, this might be a line, a limit switch, a distance ... 
        int taskComplete = isTaskComplete(wheels, &command);

        //this is what translates from WheelPid to wheelCmd 
        limitPowerWheels(wheels,wheelCmd,direction,taskComplete);

        if(taskComplete) { 
            if(encoderReset == 0){
             resetWheels(wheelCmd, wheels); 
             resetEncoder(encoderPipe[1]);
                curImu.curError = 0;
            }
             encoderReset = 1;
        }
        
      }
      writeToWheels(wheelCmd); 

      if(!(wheelCmd[FL][0] == 2000 && wheelCmd[FR][0] == 2000)) {
         printf("timer_u: , %g , timer_m: , %g , Finial wheel Power: , FL: , %d , FR: , %d , BR: , %d , BL , %d\n", \
               timer_u, timer_m, wheelCmd[FL][0],wheelCmd[FR][0], wheelCmd[BR][0], wheelCmd[BL][0]);
      }


      gettimeofday(after,NULL);
      timer_u =  (after->tv_usec - before->tv_usec) +\
            1000000*(after->tv_sec - before->tv_sec);
      timer_m  = timer_u/1000;

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


