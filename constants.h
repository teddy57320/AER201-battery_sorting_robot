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
#define RTC_LAST_RUN	9
#define PERM_LOGA	10
#define PERM_LOGB	11
#define PERM_LOGC	12
#define PERM_LOGD	13
// #define PC_LOG		14
#define RTC_DISPLAY	14

// LCD macros
#define __delay_1s() for(char i=0;i<100;i++){__delay_ms(10);}
#define __lcd_newline() lcdInst(0b11000000);
// #define __lcd_clear() lcdInst(0b10000000);
#define __lcd_clear() lcdInst(0b00000001); __delay_ms(10);
#define __lcd_home() lcdInst(0b10000000);
#define __bcd_to_num(num) (num & 0x0F) + ((num & 0xF0)>>4)*10

//Pin Macros

//s = 0 or 1
//outputs
#define initialSol(s) LATBbits.LB0 = s;			//initial funnel solenoid (RB0)

#define chamberDir(s) LATDbits.LD0= s;				//dir pin for stepper (RD0)
#define chamberStep(s) LATDbits.LD1 = s;			//step pin for stepper (RD1)

#define plat1c1a(s) LATCbits.LC1 = s;			//platform 1 coil 1a (RC1)	yellow/grey
#define plat1c1b(s) LATCbits.LC2 = s;			//platform 1 coil 1b (RC2)
#define plat1c2a(s) LATCbits.LC5 = s;			//platform 1 coil 2b (RC5)	orange/brown
#define plat1c2b(s) LATCbits.LC6 = s;			//platform 1 coil 2a (RC6)
#define plat2c1a(s) LATCbits.LC0 = s;			//platform 2 coil 1a (RC0)	brown/green
#define plat2c1b(s) LATEbits.LE2 = s;			//platform 2 coil 1b (RE2)
#define plat2c2b(s) LATEbits.LE1 = s;			//platform 2 coil 2b (RE1)	purple/blue
#define plat2c2a(s) LATAbits.LA4 = s;			//platform 2 coil 2a (RA4)

#define UVDsol(s) LATCbits.LC7 = s;				//UVD solenoid (RC7)


/*inputs
IR sensor: RA0	green 
C circuit: RA1 (circuit 1)	black
AA circuits: RA2 and RA3 (circuit 2 and 3)	red, white
9V circuits: RA5 and RE0 (circuit 4 and 5)	yellow, orange
*/

#define WAIT_TIME 10			//max waiting time before operation terminates


#endif	/* CONSTANTS_H */

