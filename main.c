#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <poll.h>

#define MYREAD(_FD,_BUF,_COUNT){\
   if(read(_FD,_BUF,_COUNT)==-1){\
      fprintf(stderr,"%s: %d",__FILE__,__LINE__);\
      perror(NULL);\
      exit(EXIT_FAILURE);\
   }\
}
void myclose(int fd){
   if(close(fd)!= 0){
      fprintf(stderr,"%s: %d",__FILE__,__LINE__);
      perror(NULL);
      exit(EXIT_FAILURE);
   }
}
void mypipe(int p[]){
   if(pipe(p)!=0){
      fprintf(stderr,"%s: %d",__FILE__,__LINE__);
      perror(NULL);
      exit(EXIT_FAILURE);
   }
}
void mywrite(int fd,const void *buff,size_t count){
   if(write(fd,buff,count)==0){
      fprintf(stderr,"%s: %d",__FILE__,__LINE__);
      perror(NULL);
      exit(EXIT_FAILURE);
   }
}
void clear(){
   int c = fgetc(stdin);
   while(c!=EOF){
      c = fgetc(stdin);
   }
}
int main(){
   pid_t pid;
   //int status;
   int pipeToMe[2];
   int pipeToIt[2];
   int pipeFromSTDIN;
//   double angle[3];
   double acc[3];
   double dt_usec;
   double dt_sec;
   double rawAcc[3];
   double grav[3];
   

   struct timeval *before,*after;
   before = malloc(sizeof(struct timeval));
   after = malloc(sizeof(struct timeval));
   double pos[] = {0,0,0};
   double vel[] = {0,0,0};

   mypipe(pipeToMe);
   pipeFromSTDIN = pipeToMe[0];
   myclose(pipeToMe[1]);
   mypipe(pipeToMe);
   mypipe(pipeToIt);

   struct pollfd stdin_poll = {
     .fd = pipeFromSTDIN, .events = POLLIN |  POLLPRI };

   if((pid = fork()) < 0){
      perror(NULL);
      fprintf(stderr,"fork");
      exit(EXIT_FAILURE);
   }
   if(pid == 0){
      myclose(pipeToMe[0]);
      myclose(pipeToIt[1]);
      myclose(pipeFromSTDIN);
      dup2(pipeToMe[1],STDOUT_FILENO);
      dup2(pipeToIt[0],STDIN_FILENO);
      execlp("/usr/bin/minimu9-ahrs","minimu9-ahrs","--output","euler",(char*) NULL);
      fprintf(stderr,"execl failure");
      perror(NULL);
      exit(EXIT_FAILURE);
   }
   myclose(pipeToMe[1]);
   myclose(pipeToIt[0]);
   myclose(pipeFromSTDIN);
   dup(STDIN_FILENO);
   dup2(pipeToMe[0],STDIN_FILENO);

   gettimeofday(before,NULL);
   char print = 1;
   char buff[30];
   int index = 0;
   while(1){
/*        scanf("%lf %lf %lf %lf %lf %lf\n",&angle[0],&angle[1],\
              &angle[2],&acc[0],&acc[1],&acc[2]);*/

        scanf("%lf %lf %lf %lf %lf %lf\n",&rawAcc[0],&rawAcc[1],\
              &rawAcc[2],&grav[0],&grav[1],&grav[2]);
        gettimeofday(after,NULL);
        dt_usec =  (after->tv_usec - before->tv_usec) +\
              1000000*(after->tv_sec - before->tv_sec);
        dt_sec  = dt_usec/1000000;
        dt_sec = 0.02;

        vel[0] += acc[0]*dt_sec;
        vel[1] += acc[1]*dt_sec;
        vel[2] += acc[2]*dt_sec;

        pos[0] += vel[0]*dt_sec*1000;
        pos[1] += vel[1]*dt_sec*1000;
        pos[2] += vel[2]*dt_sec*1000;

        if(print){
            printf("Raw Acc: , %g , %g , %g , Grav: , %g , %g , %g \n",\
                  rawAcc[0],rawAcc[1],rawAcc[2],grav[0],grav[1],grav[2]);
            //printf("Acc: %g , %g , %g \n",acc[0],acc[1],acc[2]);
            /*printf("\fDT(usec):%g\nAngle:\n\tX:%6.2f Y:%6.2f Z:%6.2f "\
                  "\nACCELERATION:\n\tX:%6.3f Y:%6.3f Z:%6.3f\n",dt_usec,angle[0],angle[1],\
                  angle[2],acc[0],acc[1],acc[2]);
            printf("VELOCITY:\n\tX:%6.2f Y:%6.2f Z:%6.2f \n"\
                  "Position:\n\tX:%6.3f Y:%6.3f Z:%6.3f\n",vel[0],vel[1],\
                  vel[2],1000*pos[0],1000*pos[1],1000*pos[2]);*/
        }

        *before = *after;
                if(poll(&stdin_poll,1,0)==1){
           while(poll(&stdin_poll,1,0)==1){
              MYREAD(pipeFromSTDIN,buff+index,1);
              index++;
           }
           index--;
           buff[index] = 0;
           if(strcmp(buff,"reset")==0){
               mywrite(pipeToIt[1],"\n",sizeof(char)*3);
      //         printf("\f\n\nResting!!!\n\n");
               vel[0] = 0;
               vel[1] = 0;
               vel[2] = 0;
               pos[0] = 0;
               pos[1] = 0;
               pos[2] = 0;
           }
           else if(strcmp(buff,"quit")==0){
              break;
           }
           print = 1;
           index = 0;
        }
   }
   myclose(pipeToMe[0]); 
   myclose(pipeToIt[1]); 
/*   if(kill(pid,1)<0){
      perror(NULL);
      exit(EXIT_FAILURE);
   }*/
   return 0;

}
