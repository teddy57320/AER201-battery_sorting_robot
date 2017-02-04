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
#define RTC_DISPLAY	8

// LCD macros
#define __delay_1s() for(char i=0;i<100;i++){__delay_ms(10);}
#define __lcd_newline() lcdInst(0b11000000);
#define __lcd_clear() lcdInst(0b10000000);
#define __lcd_home() lcdInst(0b10000000);
#define __bcd_to_num(num) (num & 0x0F) + ((num & 0xF0)>>4)*10

#endif	/* CONSTANTS_H */

