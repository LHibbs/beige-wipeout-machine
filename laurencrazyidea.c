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



/*void simpleAnglePID(WheelPid *wheels, enum dir direction , double pow, ImuDir *curImu) {

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

*/