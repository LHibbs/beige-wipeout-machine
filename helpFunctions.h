#ifndef HELPFUNCTIONS_H 
#define HELPFUNCTIONS_H 

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
   if(read(_FD,_BUF,_COUNT)<0){\
      fprintf(stderr,"Error in read: %s: %d",__FILE__,__LINE__);\
      perror(NULL);\
      exit(EXIT_FAILURE);\
   }\
}

#define MYWRITE(_FD,_BUF,_COUNT){\
   if(write(_FD,_BUF,_COUNT)<0){\
      fprintf(stderr,"Error in write: %s: %d",__FILE__,__LINE__);\
      perror(NULL);\
      exit(EXIT_FAILURE);\
   }\
}

void myclose(int fd);
void mypipe(int p[]);
void mywrite(int fd,const void *buff,size_t count);
#endif
