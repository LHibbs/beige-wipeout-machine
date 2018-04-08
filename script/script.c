#include <stdlib.h>
#include <stdio.h>

int main(int argc, char* argv[]){
   int i = 1;
   FILE* fp_read;
   FILE* fp_write;
   char buffer[1000];
   char name[100];
   for(i = 1; i < argc; i++){
      fp_read = fopen(argv[i],"r");
      sprintf(name,"edited-%s",argv[i]);
      fp_write = fopen(name,"w");

      while(fgets(buffer, 999, fp_read)!=NULL){
         if(!(buffer[0]=='w' || buffer[0]=='e' || buffer[1] == 'L')){
            fprintf(fp_write,"%s",buffer);
         }
      }
   }

   return 0;


}
