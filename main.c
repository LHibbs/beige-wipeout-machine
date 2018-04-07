#include "imuInteract.h"
#include "driveWheelPid.h"
#include <wiringPi.h>
#include "ldr.h"

#define ENC_TO_INCH 255

void clear(){
   int c = fgetc(stdin);
   while(c!=EOF){
      c = fgetc(stdin);
   }
}

// dist in inches
void move(enum dir direction, int dist,int* driveWheelPipe) {
   double distance = dist * ENC_TO_INCH;
   char msg[10];
   msg[0] = 'm';
   //printf("wiritng to child\n\n");
   MYWRITE(driveWheelPipe[1],msg,sizeof(char));
   MYWRITE(driveWheelPipe[1],&distance,sizeof(double));
   MYWRITE(driveWheelPipe[1],&direction,sizeof(enum dir));
}


int main(){

   pid_t driveWheelPid;
   char buff[30];
   int index = 0;
   //int pipeFromSTDIN;
   //int pipeToIMU;
   int* driveWheelPipe;
   char msg[1000];
   WheelPid wheels[4];
   //dont buffer stdout so faster printing to screen uncomment if multiple process are printing to screen
   //setbuf(stdout,NULL);

   struct timespec sleepTime;
   sleepTime.tv_sec = 0;
   sleepTime.tv_nsec = 50000000;
   long mv[4];
   int status;
   //runLDRTest();

   wiringPiSetup();
   pinMode(pin(FL),OUTPUT);
   pinMode(pin(FR),OUTPUT);
   pinMode(pin(BL),OUTPUT);
   pinMode(pin(BR),OUTPUT);

   driveWheelPid = createDriveWheelChild(&driveWheelPipe);
   //int newS;
   //int wtC;
   //createAcceleromoterChild(&newS,&wtC);


   enum dir direction;
   struct pollfd stdin_poll = {
       .fd = STDIN_FILENO, .events = POLLIN |  POLLPRI };
   //  .fd = newS, .events = POLLIN |  POLLPRI };
   //printf("in main line%d\n\n\n",__LINE__);
   

   while(1){
      if(fgetc(stdin)==EOF){
       break;
      }

      direction = Backward;
      move(direction, 150 , driveWheelPipe); 
      if(fgetc(stdin)==EOF){
         break;
      }

      direction = Forward;
      move(direction, 150 ,driveWheelPipe); 

   }
   //printf("in main line%d\n\n\n",__LINE__);
   msg[0] = 'q'; 
   MYWRITE(driveWheelPipe[1], msg, sizeof(char));   
   waitpid(driveWheelPid,&status,0);
   return 0; 

   // not using anything below this right now
   while(1){
      if(poll(&stdin_poll,1,0)==1){
           while(poll(&stdin_poll,1,0)==1){
              
              MYREAD(STDIN_FILENO,buff+index,1);
             // MYREAD(pipeFromSTDIN,buff+index,1);
              index++;
           }
      }
      if(sscanf(buff,"%ld , %ld, %ld, %ld\n",&(mv[0]),&(mv[1]),&(mv[2]),&(mv[3]))==2){
         msg[0] = 'm';
         MYWRITE(driveWheelPipe[1],msg, sizeof(char));
         MYWRITE(driveWheelPipe[1],mv, sizeof(long)*4);
      }
      else{
         fprintf(stderr,"invalid String: line:%d\n",__LINE__);
      }
      msg[0] = 'p';
      MYWRITE(driveWheelPipe[1],msg, sizeof(char));
      MYREAD(driveWheelPipe[0],wheels,sizeof(WheelPid)*4);
      printf("Current Encoder valules:\n");
      printf("FL:%ld\t FR:%ld\nBL:%ld\t BR:%ld\n",wheels[FL].encoderCnt,wheels[FR].encoderCnt,wheels[BL].encoderCnt,wheels[BR].encoderCnt);
      nanosleep(&sleepTime,NULL);
   }
   return 0;

}
