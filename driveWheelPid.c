#include "driveWheelPid.h"

#define KD 0
#define KP 1
#define KI 0
//#define KI .1 

#define KP_ANGLE 512
#define KI_ANGLE 0
#define MOTOR_FWD 0
#define MOTOR_BACK 1

#define BIAS_BACK 103
#define BIAS_FWD 80

#define dt_msec 5
#define dt_nsec 5000000
#define dt_sec ((double)dt_nsec)/1000000000

#define MAX_SPEED 1
#define MIN_SPEED .08

#define FWD_BIAS 80
#define BACK_BIAS 103 
#define LEFT_BIAS -80 //positive is more arcing and moving up when going left
#define RIGHT_BIAS -300//positive is more arcing and moving down when going right 
#define CLOCKWISE_BIAS 0
#define COUNTERCLOCKWISE_BIAS 0

//distance to center of ramp 23 

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
         return 5;
      case FR:
         return 31;
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
      case Clockwise:
         wheels[FL].curDir = 1;
         wheels[FR].curDir = -1;
         wheels[BR].curDir = -1;
         wheels[BL].curDir = 1;
      break;
      case Counterclockwise:
         wheels[FL].curDir = -1;
         wheels[FR].curDir = 1;
         wheels[BR].curDir = 1;
         wheels[BL].curDir = -1;
      default:
         assert(0);
   }
   for(int i = 0; i < 4; i++){
      //note outside this function we set child encoder proess encoder counts to 0
      wheels[i].encoderCnt = 0;
      wheels[i].encoderGoal = (inputGoal)*(wheels[i].curDir);
      wheels[i].curError = 0;
   }
   printf("reset encoderCnt: wheels[0].encoderGoal:%ld\n\n\n",wheels[0].encoderGoal);
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
         case Right:
         case Left:
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

double abs_double(double a){
   return (a<0)?-1*a:a;
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
               if(*direction==Backward || *direction==Left){
                  command->encoderDist = *inputGoal*-1; 
               }
               else{
                  command->encoderDist = *inputGoal; 
               }

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
               MYREAD(stdin_poll->fd,&(command->lineSensorConfig),sizeof(char));
               fprintf(stderr,"inputGoal:%g \t direction:%d\n\n\n",inputGoal[0],(int)*direction);
               updateWheels(wheels,inputGoal[0],*direction);
               
               //also doesn't seem like this needs to be here 
               resetEncoder(encoderPipe[1]);
               *startUpPhase = 1;
               //gradualStartUp(wheels,wheelCmd,*direction);
               command->cmdType = Line; 
               if(*direction==Backward || *direction==Left){
                  command->encoderDist = abs(*inputGoal*-1); 
               }
               else{
                  command->encoderDist = abs(*inputGoal); 
               }
               break; 
            case 'a': 
               command->cmdType = Align;  
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

void updateLineSensor(int *linePipe,unsigned char* curLine){

    char msg[10]; 
    sprintf(msg,"p");
    MYWRITE(linePipe[1],msg,sizeof(char));
    MYREAD(linePipe[0],curLine,sizeof(unsigned char));
    //printf("curLine:%x\n",(unsigned int)*curLine);
}
void updateEncoderStatus(int *encoderPipe, Encoder *curEnco, WheelPid *wheels) {
    char msg[10]; 
    sprintf(msg,"p");
    MYWRITE(encoderPipe[1],msg,sizeof(char));
    MYREAD(encoderPipe[0],curEnco,sizeof(Encoder)* 4);
    for(int i = 0; i < 4; i++){
       if(wheels[i].curDir > 0){
         wheels[i].encoderCnt += (curEnco[i].count-wheels[i].encoderCnt);
       }
       else{
         wheels[i].encoderCnt -= (curEnco[i].count+wheels[i].encoderCnt);
       }
    }
}

//this takes the pow that is a double that can be negitive or postiive and then direction and turns wheelCmd with the correct power and dir output to be controled
void limitPowerWheels(WheelPid *wheels,int wheelCmd[][2],enum dir direction,int taskComplete,int ignoreEncoder){
   //pow is valid from -2000 to 2000 if neg is then turned postive with dirValueTemp 1
   //printf("power:%10.10f \t direction:%d\n",*pow,direction);
   switch(direction){
      case Forward:
         wheels[FL].tempCurDir = 0;
         wheels[FR].tempCurDir = 0;
         wheels[BR].tempCurDir = 0;
         wheels[BL].tempCurDir = 0;
      break;
      case Backward:
         wheels[FL].tempCurDir = 1;
         wheels[FR].tempCurDir = 1;
         wheels[BR].tempCurDir = 1;
         wheels[BL].tempCurDir = 1;
      break;
      case Left:
         wheels[FL].tempCurDir = 1;
         wheels[FR].tempCurDir = 1;
         wheels[BR].tempCurDir = 1;
         wheels[BL].tempCurDir = 1;
      break;
      case Right:
         wheels[FL].tempCurDir = 0;
         wheels[FR].tempCurDir = 0;
         wheels[BR].tempCurDir = 0;
         wheels[BL].tempCurDir = 0;
      break;
      case Clockwise:
         wheels[FL].tempCurDir = 0;
         wheels[FR].tempCurDir = 0;
         wheels[BR].tempCurDir = 0;
         wheels[BL].tempCurDir = 0;
      break;
      case Counterclockwise:
         wheels[FL].tempCurDir = 1;
         wheels[FR].tempCurDir = 1;
         wheels[BR].tempCurDir = 1;
         wheels[BL].tempCurDir = 1;
      break;
   }
   for(int i = 0 ; i < 4; i ++ ){

       //if pow is negitive invert it and set dirValue Temp = 1
       if(!ignoreEncoder){
         if((wheels[i].pow) < 0){
            wheels[i].tempCurDir = 1;
            wheels[i].pow *= -1;
         }
         else {
            wheels[i].tempCurDir = 0;
         }
       }
       else{
          wheels[i].pow = MIN_SPEED;
       }
       
       assert(wheels[i].pow >=0);
       //if greater then maxumim speed then set it to max speed
       if((wheels[i].pow)>MAX_SPEED){
          wheels[i].pow = MAX_SPEED;
       }
       //if less then minamum speed then set it to zero we are done
       //added double_abs
       if(abs_double(wheels[i].pow) < MIN_SPEED){
           if(taskComplete == 1){
              wheels[i].pow  = 0;
           }
           else{
               wheels[i].pow = MIN_SPEED;
           }
       }
       wheels[i].powToWheels = wheels[i].pow * 2000;
   }
   if(ignoreEncoder){
      switch(direction){
         case Right:
            wheels[FL].powToWheels = MIN_SPEED*2000 -30;
            wheels[FR].powToWheels = MIN_SPEED*2000-30;
            wheels[BR].powToWheels = MIN_SPEED*2000 + 50;
            wheels[BL].powToWheels = MIN_SPEED*2000 + 30;
         break;       
         case Left:  
            wheels[FL].powToWheels = MIN_SPEED*2000-30;
            wheels[FR].powToWheels = MIN_SPEED*2000-30;
            wheels[BR].powToWheels = MIN_SPEED*2000 + 30;
            wheels[BL].powToWheels = MIN_SPEED*2000 + 30;
         break;     
         case Forward:
            wheels[FL].powToWheels = MIN_SPEED*2000;
            wheels[FR].powToWheels = MIN_SPEED*2000;
            wheels[BR].powToWheels = MIN_SPEED*2000;
            wheels[BL].powToWheels = MIN_SPEED*2000;
         break;      
         case Backward:
            wheels[FL].powToWheels = MIN_SPEED*2000;
            wheels[FR].powToWheels = MIN_SPEED*2000;
            wheels[BR].powToWheels = MIN_SPEED*2000;
            wheels[BL].powToWheels = MIN_SPEED*2000;
         break;       
         case Counterclockwise:
            wheels[FL].powToWheels = MIN_SPEED*2000;
            wheels[FR].powToWheels = MIN_SPEED*2000;
            wheels[BR].powToWheels = MIN_SPEED*2000;
            wheels[BL].powToWheels = MIN_SPEED*2000;
         break;        
         case Clockwise:
            wheels[FL].powToWheels = MIN_SPEED*2000;
            wheels[FR].powToWheels = MIN_SPEED*2000;
            wheels[BR].powToWheels = MIN_SPEED*2000;
            wheels[BL].powToWheels = MIN_SPEED*2000;
         break;
      }
   }
  
   for(int j = 0 ; j < 4; j++){
      printf("wheels[%d].tempCurDir: %d ,.pow: %g, powToWheels:%d\n",j,wheels[j].tempCurDir,wheels[j].pow,wheels[j].powToWheels);
   }
   
   for(int j = 0 ; j < 4; j++){
      printf("[%d][1]: %d , [%d][0]: %d \n",j,wheelCmd[j][0],j,wheelCmd[j][1]);
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
      
      //TODO FL should be going fwd , FR backwords, BR fws, BL backwords
            //FL
            wheelCmd[FL][0] = (int)abs(2000*(wheels[FL].tempCurDir) - wheels[FL].powToWheels);//defualt backwords
            wheelCmd[FL][1] = wheels[FL].tempCurDir;

            //FR
            wheelCmd[FR][0] = (int)abs(2000*(1-wheels[FR].tempCurDir) - wheels[FR].powToWheels);//default fowards
            wheelCmd[FR][1] = 1-wheels[FR].tempCurDir;

            //BR
            wheelCmd[BR][0] = (int)abs(2000*(wheels[BR].tempCurDir) - wheels[BR].powToWheels);//default fowards
            wheelCmd[BR][1] = wheels[BR].tempCurDir;

            //BL
            wheelCmd[BL][0] = (int)abs(2000*(1-wheels[BL].tempCurDir) - wheels[BL].powToWheels);//default backwords
            wheelCmd[BL][1] = 1-wheels[BL].tempCurDir;
      break;
      case Clockwise:
      case Counterclockwise:
            //FL
            wheelCmd[FL][0] = (int)abs(2000*(wheels[FL].tempCurDir) - wheels[FL].powToWheels);//defualt backwords
            wheelCmd[FL][1] = wheels[FL].tempCurDir;

            //FR
            wheelCmd[FR][0] = (int)abs(2000*(1-wheels[FR].tempCurDir) - wheels[FR].powToWheels);//default fowards
            wheelCmd[FR][1] = 1-wheels[FR].tempCurDir;

            //BR
            wheelCmd[BR][0] = (int)abs(2000*(1-wheels[BR].tempCurDir) - wheels[BR].powToWheels);//default fowards
            wheelCmd[BR][1] = 1-wheels[BR].tempCurDir;

            //BL
            wheelCmd[BL][0] = (int)abs(2000*(wheels[BL].tempCurDir) - wheels[BL].powToWheels);//default backwords
            wheelCmd[BL][1] = wheels[BL].tempCurDir;
            break;
      default:
         assert(0);
   }

   /*for(int i = 0 ; i< 4;i++){
      printf("wheelCmd[%d][0] = %d wheelCmd[%d,[1]] = %d\n",i,wheelCmd[i][0],i,wheelCmd[i][1]);
   }*/

   for(int i = 0 ; i< 4;i++){
      assert(wheelCmd[i][0]>=0);
      assert(wheelCmd[i][0]<=2000);
      assert(wheelCmd[i][1]==0||wheelCmd[i][1]==1);

      wheels[i].curDir = (wheelCmd[i][1]==0)?1:-1;
   }
/*
   for(int i = 0 ; i< 4;i++){
      printf("wheelCmd[%d][0] = %d wheelCmd[%d,[1]] = %d\n",i,wheelCmd[i][0],i,wheelCmd[i][1]);
   }
   for(int i = 0 ; i< 4;i++){
      printf("%d:wheelCmd.curDir = %d wheel.encoderCnt = %ld\n",i,wheels[i].curDir,wheels[i].encoderCnt);
   }*/

}
double angleToValue(double angle){
   assert(angle >= 0);
   if(abs(angle) > 90){
      double diff = 0;
      diff = abs_double(angle) - 180;
      return (diff * ((angle>0)?1.0:-1.0));
   }
   return angle;
}
/* corrects for what value of angle we have acumulated over the trip*/
void straightBias(WheelPid *wheels,enum dir direction,double powerMult, int ignoreEncoder) {
   double pow;
   switch(direction){
      case Forward:
         pow = FWD_BIAS;
      break;
      case Backward:
         pow = -BACK_BIAS;
      break;
      case Left:
         pow = LEFT_BIAS;///positive is more arcing and moving up when going left
      break;
      case Right:
         pow = RIGHT_BIAS;
      break;
      case Clockwise:
         pow = CLOCKWISE_BIAS;
         break;
      case Counterclockwise:
         pow = COUNTERCLOCKWISE_BIAS;
      break;
      default:
         assert(0);
      break;
   }
   pow /= 2000;



   //if abs(powerMult) is less then 1 then we are adding and subtracting so reduce bias by factor of two
   if(abs_double(powerMult) <= .9){
      pow /= 2;
   }
   if(ignoreEncoder) {
      pow = 0; 
   }
   //TODO add bias to single wheel BL!!

   switch(direction){
      case Forward:
         wheels[FL].pow = powerMult - (powerMult)*pow;
         wheels[BL].pow = powerMult - (powerMult)*pow;

         wheels[FR].pow = powerMult + (powerMult)*pow;
         wheels[BR].pow = powerMult + (powerMult)*pow;

      break;
      case Backward:
      printf("powerMult:%g, powerMult*pow:%g\n",powerMult,(powerMult*pow));

         wheels[FL].pow = powerMult + (powerMult)*pow;
         wheels[BL].pow = powerMult + (powerMult)*pow;

         wheels[FR].pow = powerMult - (powerMult)*pow;
         wheels[BR].pow = powerMult - (powerMult)*pow;

      break;
      case Left://positive is more arcing and moving up when going left
         wheels[BR].pow = powerMult - (powerMult)*pow;
         wheels[FL].pow = powerMult - (powerMult)*pow;

         wheels[FR].pow = powerMult + (powerMult)*pow;
         wheels[BL].pow = powerMult + (powerMult)*pow;
      break; 
      case Right://positive bias means more clockwise drift right
         wheels[BL].pow = powerMult + (powerMult)*pow;
         wheels[FR].pow = powerMult + (powerMult)*pow;

         wheels[FL].pow = powerMult + (powerMult)*pow;
         wheels[BR].pow = powerMult - 2*(powerMult)*pow;
      break;
      case Clockwise:
         wheels[BL].pow = powerMult + (powerMult)*pow;
         wheels[FR].pow = powerMult + (powerMult)*pow;

         wheels[FL].pow = powerMult - (powerMult)*pow;
         wheels[BR].pow = powerMult - (powerMult)*pow;
      case Counterclockwise:
         wheels[BL].pow = powerMult + (powerMult)*pow;
         wheels[FR].pow = powerMult + (powerMult)*pow;

         wheels[FL].pow = powerMult - (powerMult)*pow;
         wheels[BR].pow = powerMult - (powerMult)*pow;
      break;
   }
   for(int i = 0; i < 4;i++){

      printf("wheels[%d].pow:%g\n",i,wheels[i].pow);
      if(abs_double(wheels[i].pow) > MAX_SPEED){
         wheels[i].pow = MAX_SPEED *((wheels[i].pow > 0)?1:-1);
      }
      //else if(abs_double(wheels[i].pow) < MIN_SPEED){
      //   wheels[i].pow = 0;
      //}
      printf("wheels[%d].pow:%g\n",i,wheels[i].pow);
   }
}
/* logic that outputs a double indicating wheel power multiplier for all 4 wheels.
 * Used to speed up and slow down at beginning and end. 
 */
double distancePIDControl(WheelPid *wheels, enum dir direction, int * startupPhase, int ignoreEncoder, double lastDistPowVal) {

   int indexEncoderToUse = 0;
       long error_new;
   long encoderToUse = wheels[indexEncoderToUse].encoderCnt;
 
   if(ignoreEncoder) {
      return 0;
   }

    if(*startupPhase){
        if(direction==Forward || direction==Right){
            lastDistPowVal += .01; 
        }
        if(direction==Backward || direction==Left){
           printf("Backward startup\n\n");
            lastDistPowVal -= .01; 
        }
        
        if( abs_double(lastDistPowVal) >= 1) {
	    printf("done with startup phase"); 
            *startupPhase = 0; 
        } 
        return lastDistPowVal;
        
    }
       // this grabs the wheels that moved the least. THis is crazy sam's idea for best PI control change later if foundn he was just dumb
       /*for(int i = 1; i < 4; i++){
          if(abs(wheels[i].encoderCnt) < abs(encoderToUse)){
             encoderToUse = wheels[i].encoderCnt;
             indexEncoderToUse = i;
          }
       }*/
       // Calculate errors:
       error_new = wheels[indexEncoderToUse].encoderGoal - encoderToUse;

       /*if((error_new > 0 && wheels[indexEncoderToUse].encoderGoal < 0) ||
		       (error_new < 0 && wheels[indexEncoderToUse].encoderGoal > 0)) {
	      return 0; 
       }  */
       //TODO add for BACK and LEFT and RIGHT thsi is only for FOWARD!!!

       // PI control
       for(int i = 0; i< 4;i++){
          wheels[i].curError += error_new;
       }


   double pow;
   //using wheels[0] becaues all wheels have the same curError. this will probally change 
   pow = (KP*error_new + KI*dt_sec*(wheels[0].curError));
      printf("pow:%g, encoderGoal:%ld, encoderVal:%ld, errorNew:%ld\n",pow,wheels[indexEncoderToUse].encoderGoal,encoderToUse,error_new);
   if(pow > 1){
      return 1;
   }
   else if(pow < -1){
      return -1;
   }
   else
      return 0;
   if(pow >  2000 ){
       pow =   1;
   }
   else if(pow < -2000){
      pow = -1;
   }
   else{
	   pow = pow/2000;
   }
   if(*startupPhase){
	   if(abs_double(pow) < abs_double(lastDistPowVal)){
		   *startupPhase = 0;
		   return pow;
	   }
	   else{
		   return lastDistPowVal;
	   }
   }


   printf("pow returned by distance:%g\n",pow);
   return pow;
} 


void resetImu(int  imuPipe){
   char msg[] = "r";
   MYWRITE(imuPipe,msg,sizeof(char));
}
void updateImuStatus(ImuDir *curImu){
   struct pollfd IMU_poll = {
     .fd = STDIN_FILENO, .events = POLLIN |  POLLPRI };
   float x,y,z;
   while(poll(&IMU_poll,1,0)==1){
      scanf("%g %g %g\n",&(x),&(y),&(z));
   }
   curImu->Rx = (x);
}

//currently assumes that 0s in lineConfig are not important, can change this later
//i.e these must be sensing a line, but it doesn't matter either way for other ones
int lineConditionsMet(unsigned char lineSensorConfig, unsigned char curLineSensor) {

   printf("line sensor config: %x curLineSensor: %x", lineSensorConfig, curLineSensor); 
   if((lineSensorConfig & curLineSensor) == lineSensorConfig){
	   printf("TASK COMPLEETE!!!\n");
      return 1;
   }

   return 0;

}

int isTaskComplete(WheelPid *wheelPid, Command *command, unsigned char curLineSensor, ImuDir *curImu,int *ignoreEncoder) {
 
    printf("isTask complete : encoderCnt:%ld encoderDist:%g\n",wheelPid[0].encoderCnt,command->encoderDist);
   //speculatively uncommented v
    if(command->cmdType == Distance) {
        if(abs_double(wheelPid[0].encoderCnt) >= abs_double(command->encoderDist))
            return 1; 

    } else if (command->cmdType == Line){
	    if(abs_double((wheelPid[0].encoderCnt)) >= abs_double(command->encoderDist)){
                  printf("ENCODER DISTANCE REACHED IGNORING ENCODER\n\n");
		    //*ignoreEncoder = 1;
		 return lineConditionsMet(command->lineSensorConfig, curLineSensor);      
	    }
    } else if (command->cmdType == Align) { 
       *ignoreEncoder = 1;
       fprintf(stderr,"angle is:%g\n",angleToValue(curImu->Rx));
        if(abs_double(angleToValue(curImu->Rx)) < .3)
        {
            return 1;
        }
    }
    return 0; 
} 
void resetWheels(int wheelCmd[][2], WheelPid *wheels) {

   printf("\n\n\nreseting wheels!\n\n\n");
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

void align(enum dir *dir, WheelPid *wheelPid, Command *command, ImuDir *curImu) {
    
    if(command->cmdType == Align) { 
        if(angleToValue(curImu->Rx) < 0) { 
            *dir = Clockwise;
            wheelPid[FL].pow = MIN_SPEED;
            wheelPid[FR].pow = -MIN_SPEED; 
            wheelPid[BR].pow = -MIN_SPEED;
            wheelPid[BL].pow = MIN_SPEED; 
        } else {
            *dir = Counterclockwise;  
            wheelPid[FL].pow = -MIN_SPEED;
            wheelPid[FR].pow = MIN_SPEED; 
            wheelPid[BR].pow = MIN_SPEED;
            wheelPid[BL].pow = -MIN_SPEED; 
        } 
    }
}

void driveWheelPidControl(int new_stdin){

   int* encoderPipe;
   int* linePipe;
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
   char encoderReset = 1;
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
   createLineSensorChild(&linePipe);



   stdInPipe = stdInPipe;

   struct pollfd stdin_poll = {
     .fd = stdInPipe, .events = POLLIN |  POLLPRI };

   int wheelCmd[4][2];
   unsigned char curLineSensor = 0; 

   int ignoreEncoder = 0;
   resetWheels(wheelCmd, wheels); 
   writeToWheels(wheelCmd); 
//this is beacues the IMU takes a coucple of seconds to start working
   for(int i =0;i < 60; i++){
      scanf("%g %g %g\n",&(curImu.Rx),&(curImu.Ry),&(curImu.Rz));
   }
   resetImu(imuPipe);
   for(int i =0;i < 10; i++){
      scanf("%g %g %g\n",&(curImu.Rx),&(curImu.Ry),&(curImu.Rz));
      printf("%g %g %g\n",(curImu.Rx),(curImu.Ry),(curImu.Rz));
   }
   fprintf(stderr, "IMU READY!!\n");

   resetWheels(wheelCmd, wheels); 
   while(1){
      gettimeofday(before,NULL);
      
      //sets current encoder count of wheels
      updateEncoderStatus(encoderPipe, curEnco, wheels);
      updateImuStatus(&curImu);
      updateLineSensor(linePipe,&curLineSensor);


      if(handleInput(&stdin_poll,wheels, msg, wheelCmd,encoderPipe,&direction,&command,&startUpPhase)) {
          // a new command was issued: so now it is "active" 
          encoderReset = 0; 
         // resetWheels(wheelCmd, wheels); 
      }

      //fprintf(stderr,"angle is:%g\n",angleToValue(curImu.Rx));
      //if encoder reset = 1 then we have already reset the encoders and are not moving again. this is to repeat encoder resetting actions
      if(encoderReset == 0) {
        //change: this no longer controlls when it ends, it only control pid before then ... 
        distancePowerMult = distancePIDControl(wheels,direction,&startUpPhase, ignoreEncoder, distancePowerMult);
        straightBias(wheels, direction, distancePowerMult, ignoreEncoder);

        printf("distancePowerMult:%g\n",distancePowerMult);
        
        //takes distancePowerMUlt and translates that to pow for each wheels control depending on direction adding bias 
        
        align(&direction, wheels, &command, &curImu); 
        //anglePIDControl(wheels,wheelCmd,direction,&curImu);
        //the line here is whatever the thing is that controlls when its done. Depending on the command being listened to this might be different things. 
        //for example, this might be a line, a limit switch, a distance ... 
        int taskComplete = isTaskComplete(wheels, &command, curLineSensor, &curImu,&ignoreEncoder);


        //this is what translates from WheelPid to wheelCmd 
        limitPowerWheels(wheels,wheelCmd,direction,taskComplete,ignoreEncoder);

        if(taskComplete) { 
            if(encoderReset == 0){
             resetWheels(wheelCmd, wheels); 
             resetEncoder(encoderPipe[1]);
                curImu.curError = 0;
                distancePowerMult = 0;
		ignoreEncoder = 0;
            }
             encoderReset = 1;
        }
        
      }
      writeToWheels(wheelCmd); 

      /*if(!(wheelCmd[FL][0] == 2000 && wheelCmd[FR][0] == 2000)) {
         printf("timer_u: , %g , timer_m: , %g , Finial wheel Power: , FL: , %d , FR: , %d , BR: , %d , BL , %d\n", \
               timer_u, timer_m, wheelCmd[FL][0],wheelCmd[FR][0], wheelCmd[BR][0], wheelCmd[BL][0]);
      }*/


      gettimeofday(after,NULL);
      timer_u =  (after->tv_usec - before->tv_usec) +\
            1000000*(after->tv_sec - before->tv_sec);
      timer_m  = timer_u/1000;
      timer_m = timer_m + 0;

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
      driveWheelPidControl(pipeToIt[0]);
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


