#include "encoder.h"
#include <time.h>
//encoder pinout
#define FR_PIN 25
#define FL_PIN 28 
#define BR_PIN 29 
#define BL_PIN 24 
//fill this in later
#define READPIN(PIN) (0)

#define CHECKWHEEL(PIN){\
   if(digitalRead(indexToPos[PIN])!=drive[PIN].lastVal){\
         drive[PIN].count += drive[PIN].dir;\
         /*this just togels the state in a more effiecnt way then reading again from the input*/\
         drive[PIN].lastVal ^= 1;\
      }\
}

/*function that is called that will never return that wil always loop around*/
void encoderChildFunct(){
   char indexToPos[]= {FR_PIN, FL_PIN, BR_PIN, BL_PIN};
   char msg[100];
   indexToPos[0] = indexToPos[0] + 0;
   struct timespec sleepTime;
   sleepTime.tv_nsec = 100000;
   sleepTime.tv_sec = sleepTime.tv_nsec/1000000000;
   int temp;
   struct pollfd stdin_poll = {
     .fd = STDIN_FILENO, .events = POLLIN |  POLLPRI };
   Encoder *drive = malloc(sizeof(Encoder)*4);
   Encoder *enMsgIn = malloc(sizeof(Encoder)*4);
   long longMsgIn[4];
   pinMode(FL_PIN,INPUT);
   pinMode(FR_PIN,INPUT);
   pinMode(BL_PIN,INPUT);
   pinMode(BR_PIN,INPUT);


   for(int i = 0; i < 4; i++){
      drive[i].count = 0;
      drive[i].lastVal = READPIN(indexToPos[i]);
      fprintf(stderr,"i:%d=%d\n",i,drive[i].lastVal);
      drive[i].dir = 1;
      drive[i].goal = 0;
   }
   fprintf(stderr,"got in encoder almost to while loop\n");
   while(1){
      //updates the counts on the encoders if we have a new reading
      CHECKWHEEL(FR);
      CHECKWHEEL(FL);
      CHECKWHEEL(BR);
      CHECKWHEEL(BL);

      if(poll(&stdin_poll,1,0)==1){
         scanf("%c",msg);
         fprintf(stderr,"got message in encoder going to scanf:'%s'\n",msg);
         switch(msg[0]){
            fprintf(stderr,"got message in encoder :%s\n",msg);
            case 'r'://reset one or all motors 
               scanf("%c",msg);
               switch(msg[0]){
                  case FR:
                  case FL:
                  case BR:
                  case BL:
                     drive[(int)msg[0]].count = 0;
                     drive[(int)msg[0]].lastVal = READPIN(indexToPos[(int)msg[0]]);
                     drive[(int)msg[0]].dir = 1;
                     drive[(int)msg[0]].goal = 0;
                  break;
                  case 'a':
                     for(int i = 0; i < 4; i++){
                        drive[i].count = 0;
                        drive[i].lastVal = READPIN(indexToPos[i]);
                        drive[i].dir = 1;
                        drive[i].goal = 0;
                     }
                  break;
                  default:
                     fprintf(stderr,"Error in encoder function switch statment line:%d, file:%s\n",__LINE__,__FILE__);
               }
            break;

            case 'd'://change direction to one or all motors
               scanf("%c",msg);
               switch(msg[0]){
                  case FR:
                  case FL:
                  case BR:
                  case BL:
                     scanf("%d",&temp);
                     drive[(int)msg[0]].dir = (char)temp;
                  break;
                  case 'a'://change all of them expect array of longs
                     MYREAD(STDIN_FILENO,&longMsgIn,(sizeof(long)*4));
                     for(int i = 0; i < 4; i++){
                        drive[i].dir = longMsgIn[i];
                     }
                  break;
                  default:
                     fprintf(stderr,"Error in encoder function switch statment line:%d, file:%s\n",__LINE__,__FILE__);
               }
            break;

            case 'g'://change goals to one or all motors
               scanf("%c",msg);
               switch(msg[0]){
                  case FR:
                  case FL:
                  case BR:
                  case BL:
                     scanf("%ld",&(drive[(int)msg[0]].goal));
                  break;
                  case 'a'://change all of them expect array of longs
                     MYREAD(STDIN_FILENO,&longMsgIn,(sizeof(long)*4));
                     for(int i = 0; i < 4; i++){
                        drive[i].goal = longMsgIn[i];
                     }
                  break;
                  default:
                     fprintf(stderr,"Error in encoder function switch statment line:%d, file:%s\n",__LINE__,__FILE__);
               }

            break;

            case 'p'://poll for the encoder states
               MYWRITE(STDOUT_FILENO,drive,(sizeof(Encoder)*4));
            break;
            default:
               fprintf(stderr,"Error in encoder function switch statment line:%d, file:%s\n",__LINE__,__FILE__);
         }
      }
      //at 4 revs per second we will get a change of state every 150usec
      nanosleep(&sleepTime,NULL);
   }
   free(longMsgIn);
   free(enMsgIn);
   exit(EXIT_SUCCESS);
}

/*create child that executes open source code interacting with gyoscope*/
void createEncoderChild(int** writeToChild){
 
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


