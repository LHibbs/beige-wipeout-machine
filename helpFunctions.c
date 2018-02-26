#include "helpFunctions.h"

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
