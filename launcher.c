#include "launcher.h"
#include <assert.h>
#define MAX_LAUNCHER_SPEED 1000

#define FEEDER_PIN 4
#define FEEDER_DIR_PIN 33
#define FEEDER_SWITCH_PIN 31
#define FEEDER_SWITCH_ON 1
#define LAUNCHER_PIN 5

enum FeederState{Back,Forward,Front,Backward};

void writeToServod(int pin,int power) {
      char msg[100];
      sprintf(msg,"echo %d=%d > /dev/servoblaster",pin,power);
      system(msg);
}

typedef struct{
   int cmd[2];
   int count;
}FeederStruct;
typedef struct{
   int pow;
   int count;
}LauncherStruct;

void cmdMotors(FeederStruct* feederCmd, LauncherStruct* launcherCmd){

   writeToServod(FEEDER_PIN,feederCmd->cmd[0]);
   digitalWrite(FEEDER_DIR_PIN,feederCmd->cmd[1]);
   writeToServod(LAUNCHER_PIN,launcherCmd->pow);
}
void launcherChildFunct(){
   char msg[100];

   enum FeederState feederState = Backward;
   FeederStruct feederCmd = {{0,0},0};
   LauncherStruct launcherCmd = {0,0};


   int gradualStart = 0;//set to 1 if needing to gradual startup 0 otherwise
   struct timespec sleepTime;
   sleepTime.tv_nsec = 100000;//.1 mu seconds
   sleepTime.tv_sec = sleepTime.tv_nsec/1000000000;
   struct pollfd stdin_poll = {
     .fd = STDIN_FILENO, .events = POLLIN |  POLLPRI };


   cmdMotors(&feederCmd,&launcherCmd);

   fprintf(stderr,"got in launcher almost to while loop\n");
   while(1){

      if(feederState == Backward){
         if(digitalRead(FEEDER_SWITCH_PIN) == FEEDER_SWITCH_ON ){
            feederCmd.cmd[0] = 0;
            feederCmd.cmd[1] = 0;

            feederState = Back;
         }
      }
      if(feederState == Forward){
         if(digitalRead(FEEDER_SWITCH_PIN) == FEEDER_SWITCH_ON ){
            feederCmd.cmd[0] = 0;
            feederCmd.cmd[1] = 0;
            feederCmd.count = 0;

            feederState = Front;
         }
      }
      if(feederState == Front){
         if(feederCmd.count < 10){
            feederCmd.count++;
         }
         if(feederCmd.count == 10){
            feederCmd.cmd[0] = 0;
            feederCmd.cmd[1] = 1;

            feederState = Backward;
         }
      }
      if(gradualStart == 1){
         if(launcherCmd.count > 50){
            launcherCmd.count = 0;
            launcherCmd.pow++;
            if(launcherCmd.pow >=  MAX_LAUNCHER_SPEED){
               launcherCmd.pow = MAX_LAUNCHER_SPEED;
               gradualStart = 0;
            }
         }
         launcherCmd.count++;
      }

      if(poll(&stdin_poll,1,0)==1){
         scanf("%c",msg);
         switch(msg[0]){
            case 'f'://turn off launcher wheels
               launcherCmd.pow = 0;
            break;
            case 'r'://reset everything
               feederState = Front;
               feederCmd.cmd[0] = 0;
               feederCmd.cmd[1] = 0;
               launcherCmd.pow = 0;
            break;
            case 'u'://gradualy start launcher wheels
               gradualStart = 1; 
               launcherCmd.count = 0;
               launcherCmd.pow = 100;
            break;
            case 'l'://launch single ball
               feederCmd.cmd[0] = 2000;
               feederCmd.cmd[1] = 0;
               assert(feederState == Back);
               feederState = Forward;
            break;
            default:
               fprintf(stderr,"Error in encoder function switch statment recieved:%c line:%d, file:%s\n",msg[0],__LINE__,__FILE__);
         }
      }

      cmdMotors(&feederCmd,&launcherCmd);
      nanosleep(&sleepTime,NULL);
   }
   exit(EXIT_SUCCESS);
}

/*create child that executes open source code interacting with gyoscope*/
void createLauncherChild(int** writeToChild){
 
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
      encoderChildFunct();
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


