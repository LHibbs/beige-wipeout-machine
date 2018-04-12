#include "imuInteract.h"
#include "driveWheelPid.h"
#include <wiringPi.h>
#include "ldr.h"

#define ENC_TO_INCH 255

//binary config of light sensors for each type of line command

#define SIDELINE_FWD 5  //00000101
#define CENTER_TO_SUPPLY_FWD 2 
#define SUPPLY_TO_SUPPLY_FWD 2
#define SUPPLY_TO_CENTER_FWD 2

#define SIDELINE_BCK 48 //00110000
#define CENTER_TO_SUPPLY_BCK 8 //00001000 
#define SUPPLY_TO_SUPPLY_BCK 8
#define SUPPLY_TO_CENTER_BCK 8

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

void moveToLine(enum dir direction, int expectedDist, int* driveWheelPipe, char type) {
   double distance = expectedDist * ENC_TO_INCH;
   char msg[10];
   msg[0] = 'l';
   MYWRITE(driveWheelPipe[1],msg,sizeof(char));
   MYWRITE(driveWheelPipe[1],&distance,sizeof(double));
   MYWRITE(driveWheelPipe[1],&direction,sizeof(enum dir));
   MYWRITE(driveWheelPipe[1],&type, sizeof(char)); 
} 

void alignCommand(int * driveWheelPipe) { 
   char msg[10];
   msg[0] = 'a';
   MYWRITE(driveWheelPipe[1],msg,sizeof(char));
}


void sidelineForward(int* driveWheelPipe) { 
    //move(Forward, 30, driveWheelPipe); 
    //alignCommand(driveWheelPipe); 
    moveToLine(Left, 8, driveWheelPipe, CENTER_TO_SUPPLY_FWD); 
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


   //enum dir direction;
   /*struct pollfd stdin_poll = {
       .fd = STDIN_FILENO, .events = POLLIN |  POLLPRI };
       */
   

   while(1){
      if(fgetc(stdin)==EOF){
       break;
      }

      /*direction = Left;
      move(direction, 70 , driveWheelPipe); 
      if(fgetc(stdin)==EOF){
         break;
      }

      //direction = Forward;
      direction = Right;
      move(direction, 70 ,driveWheelPipe); 
      */


      sidelineForward(driveWheelPipe); 
   }
   msg[0] = 'q'; 
   MYWRITE(driveWheelPipe[1], msg, sizeof(char));   
   digitalWrite(pin(FL),0);
   digitalWrite(pin(FR),0);
   digitalWrite(pin(BL),0);
   digitalWrite(pin(BR),0);
   for(int i =0; i <6;i++){
	   sprintf(msg,"echo %d=0 > /dev/servoblaster",i);
	   system(msg);
   }
   waitpid(driveWheelPid,&status,0);

   return 0; 
}


/*void centerToSupplyFwd(int * driveWheelPipe) { 
    move
    align
    move

}*/

/*
void wipeout(int* driveWheelPipe) {
   move(Forward, 82, driveWheelPipe, SIDELINE_FWD); 
   move(Forward, 13 ,driveWheelPipe, CENTER_TO_SUPPLY_FWD);
   move(Forward, 26, driveWheelPipe, SUPPLY_TO_SUPPLY_FWD); 
   move(Forward, 13, driveWheelPipe, SUPPLY_TO_CENTER_FWD); 
   move(Forward, 82, driveWheelPipe, SIDELINE_BCK); 
} */

