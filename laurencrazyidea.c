#include "driveWheelPid.h"

//index 1: 
//0 foward
//1 right 
//2 down
//3 left 

//numbers are: 
//0 is FL 
//1 is FR
//2 is BR
//3 is BL

//if its drifting clockwise, reduce power to these motors
const int clockwise[][]  = {{0,3}, {0,1}, {1,2} , {2,3}}
//if its drifting counterclockwise, reduce these 
const int counterwise[][]  = {{1,2}, {2,3}, {2,3} , {0,4}}


void simpleAnglePID(WheelPid *wheels, enum dir direction , double pow, ImuDir *curImu) {

    double error_new  = angleToValue(curImu->Rx);
    double pow;
    curImu->curError += error_new;

    pow = KP_ANGLE*error_new + KI_ANGLE*(curImu->curError);

    //negativr power means it is drifiting left (counterclockwise)
    if(pow < 0) {
        for (int i = 0; i < 4; i++) {
            if(isValInArray(i, counterwise[(int)direction], 4)) {
               wheels[i].pow -= pow; 
            } else { 
                wheels[i].pow += pow; 
            }  
        }
    } 
    if(pow > 0) {
        for (int i = 0; i < 4; i++) {
            if(isValInArray(i, clockwise[(int)direction], 4)) {
               wheels[i].pow -= pow; 
            } else { 
                wheels[i].pow += pow; 
            }  
        }
    } 
}

int isValInArray(int val, int *arr, int size){
    int i;
    for (i=0; i < size; i++) {
        if (arr[i] == val)
            return true;
    }
    return false;
}
