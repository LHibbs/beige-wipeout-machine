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
   printf("distance Goal:%g\n",distance);
   msg[0] = 'm';
   MYWRITE(driveWheelPipe[1],msg,sizeof(char));
   MYWRITE(driveWheelPipe[1],&distance,sizeof(double));
   MYWRITE(driveWheelPipe[1],&direction,sizeof(enum dir));
}

void moveToLine(enum dir direction, int expectedDist, int* driveWheelPipe) {
   double distance = expectedDist * ENC_TO_INCH;
   char msg[10];
   msg[0] = 'l';
   MYWRITE(driveWheelPipe[1],msg,sizeof(char));
   MYWRITE(driveWheelPipe[1],&distance,sizeof(double));
   MYWRITE(driveWheelPipe[1],&direction,sizeof(enum dir));
} 


int main(){

   pid_t driveWheelPid;
   int* driveWheelPipe;
   char msg[1000];
   //dont buffer stdout so faster printing to screen uncomment if multiple process are printing to screen
   //setbuf(stdout,NULL);

   /*
   struct timespec sleepTime;
   sleepTime.tv_sec = 0;
   sleepTime.tv_nsec = 50000000;
   */
   int status;
   //runLDRTest();

   wiringPiSetup();
   pinMode(pin(FL),OUTPUT);
   pinMode(pin(FR),OUTPUT);
   pinMode(pin(BL),OUTPUT);
   pinMode(pin(BR),OUTPUT);

   driveWheelPid = createDriveWheelChild(&driveWheelPipe);

   enum dir direction;
   /*struct pollfd stdin_poll = {
       .fd = STDIN_FILENO, .events = POLLIN |  POLLPRI };
       */
   

   while(1){
      if(fgetc(stdin)==EOF){
       break;
      }

      direction = Backward;
      move(direction, 60 , driveWheelPipe); 
      if(fgetc(stdin)==EOF){
         break;
      }

      direction = Forward;
      move(direction, 70 ,driveWheelPipe); 

   }
   msg[0] = 'q'; 
   MYWRITE(driveWheelPipe[1], msg, sizeof(char));   
   waitpid(driveWheelPid,&status,0);
   return 0; 
}
