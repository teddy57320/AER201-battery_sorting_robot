/* 
 * File:   constants.h
 * Author: fkang
 *
 * Created on August 11, 2016, 2:41 PM
 */

#ifndef CONSTANTS_H
#define	CONSTANTS_H         //Prevent multiple inclusion 

//LCD Control Registers
#define RS          LATDbits.LATD2          
#define E           LATDbits.LATD3
#define	LCD_PORT    LATD   //On LATD[4,7] to be specific
#define LCD_DELAY   30

//Menu Interface Screens
#define FINISH		0
#define OPERATING	1
#define STANDBY		2
#define RUN_TIME	3
#define NUM_BAT		4
#define NUM_C		5
#define	NUM_9V		6
#define NUM_AA		7
#define NUM_DRAIN	8
#define RTC_DISPLAY	9
#define STOP		10

unsigned char screenMode = STANDBY; //start at standby screen

//permanent logs
// #define PERM_LOGS	10
// #define RUN1		20
// #define RUN2		30
// #define RUN3		40
// #define RUN4		50

// LCD macros
#define __delay_1s() for(char i=0;i<100;i++){__delay_ms(10);}
#define __lcd_newline() lcdInst(0b11000000);
#define __lcd_clear() lcdInst(0b10000000);
#define __lcd_home() lcdInst(0b10000000);
#define __bcd_to_num(num) (num & 0x0F) + ((num & 0xF0)>>4)*10

//Pin Macros

//s = 0 or 1
//outputs
#define funnelSol(s) LATCbits.LC0 = s;			//initial funnel solenoid (RC0)

#define gearDir(s) LATCbits.LC1= s;				//dir pin for stepper (RC1)
#define gearStep(s) LATCbits.LC2 = s;			//step pin for stepper (RC2)

#define plat1c1a(s) LATAbits.LA2 = s;			//platform 1 coil 1a (RA2)
#define plat1c1b(s) LATAbits.LA3 = s;			//platform 1 coil 1b (RA3)
#define plat1c2b(s) LATAbits.LA4 = s;			//platform 1 coil 2b (RA4)
#define plat1c2a(s) LATAbits.LA5 = s;			//platform 2 coil 2a (RA5)
#define plat2c1a(s) LATBbits.LB0 = s;			//platform 2 coil 1a (RB0)
#define plat2c1b(s) LATBbits.LB2 = s;			//platform 2 coil 1b (RB2)
#define plat2c2b(s) LATBbits.LB3 = s;			//platform 2 coil 2b (RB3)
#define plat2c2a(s) LATAbits.LA6 = s;			//platform 2 coil 2a (RA6)

#define UVDsol(s) LATAbits.LA7 = s;				//detector wall in UVD (RA7)
#define trans1(s) LATEbits.LE0 = s;				//first transistor (RE0)
#define trans2(s) LATEbits.LE1 = s;				//second transistor (RE1)				
#define trans3(s) LATCbits.LC5 = s;				//third transistor (RC5)
#define trans4(s) LATDbits.LD0 = s;				//fourth transistor (RD0)
#define trans5(s) LATDbits.LD1 = s;				//fifth transistor (RD1)

//inputs
// #define UVDsense() PORTAbits.RA0;				//IR sensor in UVD2 (RA0)
// #define UVDvolt() PORTAbits.RA1;				//voltage detector in UVD1 (RA1)

#define WAIT_TIME 10			//max waiting time before operation terminates


#endif	/* CONSTANTS_H */

