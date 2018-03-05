#include "imuInteract.h"

/*create child that executes open source code interacting with gyoscope*/
void createAcceleromoterChild(int* newSTDIN, int* writeToChild){

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
      dup2(pipeToMe[1],STDOUT_FILENO);
      dup2(pipeToIt[0],STDIN_FILENO);
      execlp("/usr/bin/minimu9-ahrs","minimu9-ahrs","--output","euler",(char*) NULL);
      fprintf(stderr,"execl failure");
      perror(NULL);
      exit(EXIT_FAILURE);
   }

   myclose(pipeToMe[1]);
   myclose(pipeToIt[0]);

   *newSTDIN  = dup(STDIN_FILENO);
   dup2(pipeToMe[0],STDIN_FILENO);
   
   *writeToChild = pipeToIt[1];
}
