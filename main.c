#include "imuInteract.h"
#include "encoder.h"


void clear(){
   int c = fgetc(stdin);
   while(c!=EOF){
      c = fgetc(stdin);
   }
}
int main(){
   double angle[3];
   double acc[3];
   double dt_usec;
   double dt_sec;
   //double rawAcc[3];
   //double grav[3];

   struct timeval *before,*after;
   before = malloc(sizeof(struct timeval));
   after = malloc(sizeof(struct timeval));
   double pos[] = {0,0,0};
   double vel[] = {0,0,0};
   gettimeofday(before,NULL);
   char print = 1;
   double avg[3] = {0,0,0};
   char buff[30];
   int index = 0;
   int count = 0;
   int pipeFromSTDIN;
   int pipeToIMU;
   int* encoderChildPipe;



   createAcceleromoterChild(&pipeFromSTDIN, &pipeToIMU);

   createEncoderChild(&encoderChildPipe);

   struct pollfd stdin_poll = {
     .fd = pipeFromSTDIN, .events = POLLIN |  POLLPRI };

   while(1){
        /*scanf("%lf %lf %lf %lf %lf %lf\n",&angle[0],&angle[1],\
              &angle[2],&acc[0],&acc[1],&acc[2]);*/

        scanf("%lf %lf %lf %lf %lf %lf\n",&angle[0],&angle[1],\
              &angle[2],&acc[0],&acc[1],&acc[2]);

            printf("DT(usec):%gAngle:X:%6.2f Y:%6.2f Z:%6.2f "\
                  "ACC:\tX:%6.3f Y:%6.3f Z:%6.3f\n",dt_usec,angle[0],angle[1],\
                  angle[2],acc[0],acc[1],acc[2]);

        gettimeofday(after,NULL);
        dt_usec =  (after->tv_usec - before->tv_usec) +\
              1000000*(after->tv_sec - before->tv_sec);
        dt_sec  = dt_usec/1000000;

       // dt_sec = 0.02;
       //dt_usec = 20;
        
        
        acc[0] *= 9.88;
        acc[1] *= 9.88;
        acc[2] *= 9.88;

        avg[0] += acc[0];
        avg[1] += acc[1];
        avg[2] += acc[2];

        if(count == 1){
           acc[0] = avg[0]/count;
           acc[1] = avg[1]/count;
           acc[2] = avg[2]/count;

        vel[0] += acc[0]*dt_sec;
        vel[1] += acc[1]*dt_sec;
        vel[2] += acc[2]*dt_sec;

        pos[0] += vel[0]*dt_sec*1000;
        pos[1] += vel[1]*dt_sec*1000;
        pos[2] += vel[2]*dt_sec*1000;
        avg[0] = 0;
        avg[1] = 0;
        avg[2] = 0;
        count = 0;
        }

        count++;
        if(print){
            /*printf("Raw Acc: , %g , %g , %g , Grav: , %g , %g , %g \n",\
                  rawAcc[0],rawAcc[1],rawAcc[2],grav[0],grav[1],grav[2]);*/
            //printf("Acc: %g , %g , %g \n",acc[0],acc[1],acc[2]);
       /*     printf("DT(usec):%gAngle:X:%6.2f Y:%6.2f Z:%6.2f "\
                  "ACC:\tX:%6.3f Y:%6.3f Z:%6.3f\n",dt_usec,angle[0],angle[1],\
                  angle[2],acc[0],acc[1],acc[2]);*/
            printf("\fDT(usec):%g\nAngle:\n\tX:%6.2f Y:%6.2f Z:%6.2f "\
                  "\nACCELERATION:\n\tX:%6.6f Y:%6.6f Z:%6.6f\n",dt_sec,angle[0],angle[1],\
                  angle[2],acc[0],acc[1],acc[2]);
            printf("VELOCITY:\n\tX:%6.6f Y:%6.6f Z:%6.6f \n"\
                  "Position:\n\tX:%6.3f Y:%6.3f Z:%6.3f\n",vel[0]*1000,vel[1]*1000,\
                  vel[2]*1000,pos[0],pos[1],pos[2]);
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
               mywrite(pipeToIMU,"\n",sizeof(char)*3);
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
   return 0;

}
