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
#define STANDBY		2
#define OPERATING	1
#define FINISH		0
#define RUN_TIME	3
#define NUM_BAT		4
#define NUM_C		5
#define	NUM_9V		6
#define NUM_AA		7
#define NUM_DRAIN	8
#define RTC_DISPLAY	9
//permanent logs
#define PERM_LOGS   10
#define RUN1		20
#define RUN2		30
#define RUN3		40
#define RUN4		50

// LCD macros
#define __delay_1s() for(char i=0;i<100;i++){__delay_ms(10);}
#define __lcd_newline() lcdInst(0b11000000);
#define __lcd_clear() lcdInst(0b10000000);
#define __lcd_home() lcdInst(0b10000000);
#define __bcd_to_num(num) (num & 0x0F) + ((num & 0xF0)>>4)*10

//Pin Macros
/*
//s = 0 or 1
//outputs
#define funnelSol(s) LATC = LATC | s;			//initial funnel solenoid (RC0)
#define rotDrum(s) LATC = LATC | (s<<1);		//rotating drum (RC1)
#define gearUnit1(s) LATE = LATE | (s<<1);		//rotating gear (RE1 and RE2)
#define gearUnit2(s) LATE = LATE | (s<<2);
#define plat111(s) LATA = LATA | (s<<4);		//UVD1, platform1 #1 (RA4)
#define plat112(s) LATA = LATA | (s<<5);		//UVD1, platform1 #2 (RA5)
#define plat121(s) LATB = LATB | (s<<2);		//UVD1, platform2 #1 (RB2)
#define plat122(s) LATB = LATB | (s<<4);		//UVD1, platform2 #2 (RB4)
#define plat211(s) LATB = LATB | (s<<5);		//UVD2, platform1 #1 (RB5)
#define plat212(s) LATB = LATB | (s<<6);		//UVD2, platform1 #2 (RB6)
#define plat221(s) LATB = LATB | (s<<7);		//UVD2, platform2 #1 (RC7)
#define plat222(s) LATC = LATC | (s<<6);		//UVD2, platform2 #2 (RC6)
#define UVDwall1(s) LATA = LATA | (s<<2);		//detector lead in UVD1 (RA2)
#define UVDwall2(s) LATA = LATA | (s<<3);		//detector lead in UVD2 (RA3)
//inputs
#define UVDvolt1() PORTAbits.RA0;				//voltage detector in UVD1 (RA0)
#define UVDvolt2() PORTAbits.RA1;				//voltage detector in UVD2 (RA1)
#define UVDsense1() PORTBbits.RB4;				//IR sensor in UVD1 (RB4)
#define UVDsense2() PORTBbits.RB5;				//IR sensor in UVD2 (RB5)

#define WAIT_TIME 10			//max waiting time before operation terminates

*/
#endif	/* CONSTANTS_H */

