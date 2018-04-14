#include "launcher.h"
#define MAX_LAUNCHER_SPEED 350
#define MAX_FEEDER_SPEED 2000
#define JAMTIME 600000 //in mu seconds 1 second
#define BUTTON_CONTACT_COUNT 10
#define START_UP_TIME 10000 //in mu second 2 ms this is time from switching from front to back so it does not read as going back before it moves


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
   if(launcherCmd->pow!=MAX_LAUNCHER_SPEED){
      writeToServod(LAUNCHER_PIN,launcherCmd->pow);
   //fprintf(stderr,"pow:%d\n",launcherCmd->pow);
   }
}
void launchingChildFunct(int new_stdin){
   char msg[100];
   int ballsToLaunch = 1;

   enum FeederState feederState = Forward;
   FeederStruct feederCmd = {{0,0},0};
   LauncherStruct launcherCmd = {0,0};


   int gradualStart = 0;//set to 1 if needing to gradual startup 0 otherwise
   struct timeval *start,*after;
   struct timespec sleepTime;
   sleepTime.tv_nsec = 100000;//.1 mu seconds
   sleepTime.tv_sec = sleepTime.tv_nsec/1000000000;
   struct pollfd stdin_poll = {
     .fd = new_stdin, .events = POLLIN |  POLLPRI };
   int on_button = 0;
   start = malloc(sizeof(struct timeval));
   after = malloc(sizeof(struct timeval));

   double diffTime = 0;

   cmdMotors(&feederCmd,&launcherCmd);

   fprintf(stderr,"got in launcher almost to while loop\n");
   gettimeofday(start,NULL);
   gettimeofday(after,NULL);
   while(1){

      //fprintf(stderr,"pin:%d",digitalRead(FEEDER_SWITCH_PIN));
      
      switch(feederState){
         case Backward:
            //fprintf(stderr,"state:Backward:");
            gettimeofday(after,NULL);
            diffTime =  (after->tv_usec - start->tv_usec) +\
               1000000*(after->tv_sec - start->tv_sec);
            if((diffTime > START_UP_TIME) && (digitalRead(FEEDER_SWITCH_PIN) == FEEDER_SWITCH_ON && on_button > BUTTON_CONTACT_COUNT) ){
               on_button = 0;

               feederCmd.cmd[0] = 0;
               feederCmd.cmd[1] = 0;

               feederState = Back;
  //             fprintf(stderr,"Back!\n");
            }
            else{
               if(digitalRead(FEEDER_SWITCH_PIN)==FEEDER_SWITCH_ON){
                  on_button++;
               }
               else{

               on_button = 0;
               }
               feederCmd.cmd[0] = MAX_FEEDER_SPEED;
               feederCmd.cmd[1] = 0;
            }
         break;
         case Forward:
            gettimeofday(after,NULL);
            diffTime =  (after->tv_usec - start->tv_usec) +\
               1000000*(after->tv_sec - start->tv_sec);
            //fprintf(stderr,"state:Forward:");
            if((diffTime > JAMTIME) || (digitalRead(FEEDER_SWITCH_PIN) == FEEDER_SWITCH_ON && on_button  > BUTTON_CONTACT_COUNT )){
               if(diffTime > JAMTIME){//jamed go back
                  ballsToLaunch--;
                  fprintf(stderr,"GOT A JAM resetting!\n");
                  on_button = 0;
                  feederState = Backward;
                  gettimeofday(start,NULL);
                  feederCmd.count = 0;
                  feederCmd.cmd[0] = MAX_FEEDER_SPEED;
                  feederCmd.cmd[1] = 0;
               }
               else{

                  on_button = 0;
                  feederCmd.cmd[0] = 0;
                  feederCmd.cmd[1] = 0;
                  feederCmd.count = 0;
                  feederState = Front;
               }
//                  fprintf(stderr,"Front!\n");
            }
            else{
            if(digitalRead(FEEDER_SWITCH_PIN) == FEEDER_SWITCH_ON ){
               on_button++;
            }
            else{
               on_button = 0;
            }
               feederCmd.cmd[0] = 2000-MAX_FEEDER_SPEED;
               feederCmd.cmd[1] = 1;
            }
            break;
         case Back:
            //fprintf(stderr,"state:Front:");
            if(feederCmd.count < 10){
               feederCmd.count++;
            }
            if(feederCmd.count == 10){
               feederCmd.cmd[0] = 2000-MAX_FEEDER_SPEED;
               feederCmd.cmd[1] = 1;
   //            fprintf(stderr,"Going Forward!\n");
               gettimeofday(start,NULL);
               on_button = 0;
               feederCmd.count =0;
               feederState = Forward;
            }
         break;
         case Front:
            //fprintf(stderr,"state:Back:");
            if(feederCmd.count < 10){
               feederCmd.count++;
            }
            if(feederCmd.count == 10){
               ballsToLaunch--;
               feederCmd.count =0;
               if(ballsToLaunch > 0){
    //              fprintf(stderr,"launching:%d more!\n",ballsToLaunch);
                  feederState = Backward;
                  on_button = 0;
                  feederCmd.cmd[0] = MAX_FEEDER_SPEED;
                  feederCmd.cmd[1] = 0;
               }
               else{
                  feederCmd.cmd[0] = 0;
                  feederCmd.cmd[1] = 0;
               }
            }
         break;
      }
     
      if(gradualStart == 1){
         if(launcherCmd.count > 10){
            launcherCmd.count = 0;
            launcherCmd.pow += 10;
            fprintf(stderr,"pow:%d\n",launcherCmd.pow);
            if(launcherCmd.pow >=  MAX_LAUNCHER_SPEED){
               launcherCmd.pow = MAX_LAUNCHER_SPEED;
               gradualStart = 0;
               writeToServod(LAUNCHER_PIN,launcherCmd.pow);
            }
         }
         launcherCmd.count++;
      }

      if(poll(&stdin_poll,1,0)==1){
         MYREAD(new_stdin,msg,sizeof(char));
         switch(msg[0]){
            case 'f'://turn off launcher wheels
               launcherCmd.pow = 0;
               gradualStart = 0; 
               launcherCmd.count = 0;
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
               //assert(launcherCmd.pow > 0);
               //TODO put in assert statment
               ballsToLaunch = 1;
               feederCmd.cmd[0] = 2000-MAX_FEEDER_SPEED;
               feederCmd.cmd[1] = 1;
               on_button = 0;
               //assert(feederState == Back);
               feederState = Backward;
               gettimeofday(start,NULL);
            break;
            case 'd': //dump x number of balls
               MYREAD(new_stdin,&ballsToLaunch,sizeof(int));
               //assert(launcherCmd.pow > 0);
               feederCmd.cmd[0] = 2000-MAX_FEEDER_SPEED;
               feederCmd.cmd[1] = 1;
               on_button = 0;
               //assert(feederState == Back);
               feederState = Backward;
            break;
            case 'q':
               exit(EXIT_SUCCESS);
            default:
               fprintf(stderr,"Error in encoder function switch statment recieved:%c line:%d, file:%s\n",msg[0],__LINE__,__FILE__);
               exit(EXIT_FAILURE);
         }
      }

      cmdMotors(&feederCmd,&launcherCmd);
      nanosleep(&sleepTime,NULL);
   }
   exit(EXIT_SUCCESS);
}

/*create child that executes open source code interacting with gyoscope*/
pid_t createLaunchingChild(int** writeToChild){
 
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
      launchingChildFunct(pipeToIt[0]);
      fprintf(stderr,"encoderCHildFUnction failure %d in  %s",__LINE__,__FILE__);
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


