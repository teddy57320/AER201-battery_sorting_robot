#include <xc.h>
#include "configBits.h"
#include <stdio.h>
#include "motors.h"
#include "constants.h"
void turnPlatsLeft(unsigned char deg){
    plat1c1a(1);
    plat1c1b(0);
    plat2c1a(1);
    plat2c1b(0);
    for (unsigned char i=0; i<deg; i++){
       if (screenMode != OPERATING)
          break;
       plat1c2a(1);     
       plat1c2b(0);
       plat2c2a(1);
       plat2c2b(0); 
       __delay_ms(7);
       plat1c1a(0);
       plat1c1b(1);
       plat2c1a(0);
       plat2c1b(1);
       __delay_ms(7);
       plat1c2a(0);
       plat1c2b(1);
       plat2c2a(0);
       plat2c2b(1);
       __delay_ms(7);
       plat1c1a(1);
       plat1c1b(0);
       plat2c1a(1);
       plat2c1b(0);
       __delay_ms(7);
    }
}
void turnPlatsRight(unsigned char deg){
    plat1c1a(1);
    plat1c1b(0);
    plat2c1a(1);
    plat2c1b(0);
    for (unsigned char i=0; i<deg; i++){
        if (screenMode != OPERATING)
            break;
        plat1c2a(0);
        plat1c2b(1);
        plat2c2a(0);
        plat2c2b(1);
        __delay_ms(7);
        plat1c1a(0);
        plat1c1b(1);
        plat2c1a(0);
        plat2c1b(1);
        __delay_ms(7);
        plat1c2a(1);
        plat1c2b(0);
        plat2c2a(1);
        plat2c2b(0);
        __delay_ms(7);
        plat1c1a(1);
        plat1c1b(0);
        plat2c1a(1);
        plat2c1b(0);
        __delay_ms(7);
    }
}
void turnPlat1Left(unsigned char deg){
    plat1c1a(1);
    plat1c1b(0);
    for (unsigned char i=0; i<deg; i++){
       if (screenMode != OPERATING)
          break;
       plat1c2a(1);     
       plat1c2b(0);
       __delay_ms(7);
       plat1c1a(0);
       plat1c1b(1);
       __delay_ms(7);
       plat1c2a(0);
       plat1c2b(1);
       __delay_ms(7);
       plat1c1a(1);
       plat1c1b(0);
       __delay_ms(7);
    }
}
void turnPlat2Left(unsigned char deg){
    plat2c1a(1);
    plat2c1b(0);
    for (unsigned char i=0; i<deg; i++){
       if (screenMode != OPERATING)
          break;
       plat2c2a(1);
       plat2c2b(0); 
       __delay_ms(7);
       plat2c1a(0);
       plat2c1b(1);
       __delay_ms(7);
       plat2c2a(0);
       plat2c2b(1);
       __delay_ms(7);
       plat2c1a(1);
       plat2c1b(0);
       __delay_ms(7);
    }
}
void turnPlat1Right(unsigned char deg){
    plat1c1a(1);
    plat1c1b(0);
    for (unsigned char i=0; i<deg; i++){
        if (screenMode != OPERATING)
            break;
        plat1c2a(0);
        plat1c2b(1);
        __delay_ms(7);
        plat1c1a(0);
        plat1c1b(1);
        __delay_ms(7);
        plat1c2a(1);
        plat1c2b(0);
        __delay_ms(7);
        plat1c1a(1);
        plat1c1b(0);
        __delay_ms(7);
    }
}
void turnPlat2Right(unsigned char deg){
    plat2c1a(1);
    plat2c1b(0);
    for (unsigned char i=0; i<deg; i++){
        if (screenMode != OPERATING)
            break;
        plat2c2a(0);
        plat2c2b(1);
        __delay_ms(7);
        plat2c1a(0);
        plat2c1b(1);
        __delay_ms(7);
        plat2c2a(1);
        plat2c2b(0);
        __delay_ms(7);
        plat2c1a(1);
        plat2c1b(0);
        __delay_ms(7);
    }
}