#include <xc.h>
#include <stdio.h>
#include "configBits.h"
#include "constants.h"
#include "lcd.h"
#include "I2C.h"
#include "eeprom_routines.h"

const char keys[] = "123A456B789C*0#D";

void switchMenu(unsigned char left, unsigned char right, unsigned char key);
//toggles the interface "left" and "right" to show different logs

void readADC(char channel);
//select A2D channel to read

unsigned int screenMode = STANDBY;	//start at standby screen
unsigned char time[7];		//used to retrieve real time/date
unsigned int opTimer, solOnTimer, solOffTimer, doneTimer = 0;	//counters
unsigned char num9V, numC, numAA, numBats, numDrain = 0; 
unsigned int min, sec = 0;
unsigned int runMin, runSec = 0;		//temp values for counting run time
unsigned char countC, countAA, count9V, countDrain = 0;	//temp values to count batteries

void main(void){ 
    TRISA = 0xFF;
	TRISC = 0x00;
    TRISD = 0x00;   //All output mode
    TRISB = 0xFF;   //All input mode
    LATA = 0x00;
    LATB = 0x00; 
    LATC = 0x00;
    ADCON0 = 0x00;  //Disable ADC
    ADCON1 = 0xFF;  //Set PORTB to be digital instead of analog default  
    
/*	REAL PIN ASSIGNMENTS	
	TRISA = 0b00001100;		//RA2 and RA3 used for voltage detection --> input
	TRISB = 0b00110000;		//RB4 and RB5 for IR sensor in UVDs --> input
	TRISC = 0x00;			//all output
	TRISD = 0x00;			//all output
	LATB = 0x00; 
    LATC = 0x00;
    LAT1 = 0x00;
    ADCON0 = 0x00;  //Disable ADC
	ADCON1 = 0xFE;  //set RA0 and RA1 to analog, everything else to digital
 */
    GIE = 1;	//globally enable interrupt
    INT1IE = 1;	//enable external interrupt.-
    INT1IF = 0;	//turn off external interrupt flag
    RBIE = 0;	//enable PORTB on change interrupt 
    TMR0IE = 1; //enable timer0 overflow interrupt
    TMR0IF = 0;	//turn off timer0 overflow interrupt flag

/***********************************************************
    Timer SETUP*/
    T0CON = 0b01000111; //implement the following in register T0CON:
//    TMR0ON = 0; //turn off timer
//    T08BIT = 1; //use 8-bit timer
//    T0CS = 0;   //use internal clock
//    //TOSE not changed
//    PSA = 0;    //enable prescalar
//    T0PS2:T0PS0 = 111 //prescale 1:256
//**********************************************************

/***********************************************************
	EEPROM SETUP*/
    // EEPGD = 0;	//enable access to EEPROM
    // CFGS = 0;	//access EEPROM
//**********************************************************

    initLCD();
    //nRBPU = 0;
    I2C_Master_Init(10000); //Initialize I2C Master with 100KHz clock
    ei(); // Enable all interrupts

    while (1) {
        while (screenMode == STANDBY){	//standby mode
        	di(); //making sure printing on LCD does not get interrupted
            __lcd_home();
            printf("START:   PRESS *");
            __lcd_newline();
            printf("< 4   DATA   6 >");
            ei();
            unsigned char i;
            for(i=0;i<50;i++){
            	if (screenMode != STANDBY)	//ensure immediate scrolling
            		break;
            	__delay_ms(10);
            }
            __lcd_home();
            __lcd_newline();
            printf(" <4   DATA   6> ");
            for(i=0;i<50;i++){
            	if (screenMode != STANDBY)
            		break;
            	__delay_ms(10);
            }
        }
        while (screenMode == OPERATING){	//while machine is running
            __lcd_home();
            printf("RUNNING...      ");     
            __lcd_newline();
            printf("                ");
/*************************************************
			BATTERY-SORTING OPERATION           */
/*         
            funnelSol(0);		//turn on funnel solenoid
            gearUnit1(1);		//turn on stepper motor
            gearUnit2(1);

            while(1){
 *              __lcd_home();
 *              __lcd_newline();
                printf("SECONDS: %0004d    ", opTimer); 
 *          }  
        	


*/  
//************************************************
        }
        while (screenMode == FINISH){	//finish screen  
        	__lcd_home();
        	printf("DONE! PRESS *   ");
        	__lcd_newline();
        	printf("TO CONTINUE     ");
        }
        while (screenMode == RUN_TIME){	//shows the log of latest run time
        	di();
        	__lcd_home();
        	printf("TOTAL RUN TIME: ");
        	__lcd_newline();
        	printf("%02x:%02x               ", min, sec);
        	ei();
        }
        while (screenMode == NUM_BAT){	//shows the log of total number of processed batteries 
        	di();
        	__lcd_home();
        	printf("TOTAL # OF      ");
        	__lcd_newline();
        	printf("BATTERIES: %02d   ", numBats);
        	ei();
        }
        while (screenMode == NUM_C){	//shows number of processed C batteries from the latest run
        	di();
        	__lcd_home();
        	printf("# OF C          ");
        	__lcd_newline();
        	printf("BATTERIES: %02d   ", numC);
        	ei();
        }
        while (screenMode == NUM_9V){	//shows number of processed 9V batteries from the latest run
        	di();
        	__lcd_home();
        	printf("# OF 9V         ");
        	__lcd_newline();
        	printf("BATTERIES: %02d     ", num9V);
        	ei();
        }
        while (screenMode == NUM_AA){	//shows number of processed AA batteries from the latest run
        	di();
        	__lcd_home();
        	printf("# OF AA         ");
        	__lcd_newline();
        	printf("BATTERIES: %02d     ", numAA);
        	ei();
        }
        while (screenMode == NUM_DRAIN){
        	di();
        	__lcd_home();
        	printf("# OF DRAINED    ");
        	__lcd_newline();
        	printf("BATTERIES: %02d     ", numDrain);
        	ei();
        }
        while (screenMode == RTC_DISPLAY){	// real time/date display
            //Reset RTC memory pointer 
		    I2C_Master_Start(); //Start condition
		    I2C_Master_Write(0b11010000); //7 bit RTC address + Write
		    I2C_Master_Write(0x00); //Set memory pointer to seconds
		    I2C_Master_Stop(); //Stop condition
		    //Read Current Time
		    I2C_Master_Start();
		    I2C_Master_Write(0b11010001); //7 bit RTC address + Read
		    for(unsigned char i=0;i<0x06;i++){
		        time[i] = I2C_Master_Read(1);
		    }
		    time[6] = I2C_Master_Read(0);       //Final Read without ack
		    I2C_Master_Stop();
		    __lcd_home();
		    printf("DATE: %02x/%02x/%02x  ", time[6],time[5],time[4]);    //Print date in YY/MM/DD
		    __lcd_newline();
		    printf("TIME: %02x:%02x:%02x  ", time[2],time[1],time[0]);    //HH:MM:SS	         
        }
    }
    return;
}

void switchMenu(unsigned char left, unsigned char right, unsigned char key){
    if (key == right){   	//if "right" button is pressed, toggle "right"
        if (screenMode == STANDBY)
            screenMode = RTC_DISPLAY;  	//loop to RTC display
        else
            screenMode -= 1;
    }
    else if (key == left){ 	//if "left" button is pressed, toggle "left"
        if (screenMode == RTC_DISPLAY)
            screenMode = STANDBY;	//loop back to standby
        else
            screenMode += 1;
    }
}   

//void readADC(char channel){
//    // Select A2D channel to read
//    ADCON0 = ((channel <<2));
//    ADON = 1;
//    ADCON0bits.GO = 1;
//   while(ADCON0bits.GO_NOT_DONE){__delay_ms(5);}    
//}

void interrupt high_priority highISR(void) {
    if(INT1IF && (screenMode != OPERATING)){  	//keypad disconnected during operation
        unsigned char keypress = (PORTB & 0xF0) >> 4;
        if (keys[keypress] == '*'){	
        	//press * to start operation or resume to standby after finish
        	if(screenMode == STANDBY){
        		screenMode = OPERATING;
                T0CONbits.TMR0ON = 1; //turn on timer
                LATC = LATC | 0b00000010; //RC1 = 1 , free keypad pins
            }
        	else if (screenMode == FINISH)
        		screenMode = STANDBY;
        }
        else if (screenMode != FINISH) //edge case when user presses 4 or 6 at finish screen
        	switchMenu('4', '6', keys[keypress]);
        	// press 4 to toggle "left", 6 to toggle "right"
        INT1IF = 0;     //Clear flag bit
    }
    if ((screenMode == OPERATING) && TMR0IF && TMR0IE){ //stop operation after 3 minutes
        TMR0IF = 0;
        TMR0 = 0;
        opTimer++;        
        if (opTimer >= 6866){	//3 minutes
        	// time for timer overflow
            // = number of ticks to overflow * seconds per tick
        	// = (256-TimeresetValue)*(1/(_XTAL_FREQ/4/prescale))
        	// = (256-0)*(1/(10000000/4/256)) = 0.0262 seconds
        	// so to time 3 minutes it needs to run
        	// 180seconds / 0.0262seconds = 6866.455 ~ 6866 times
            opTimer = 0;
            screenMode = FINISH;
            //funnelSol(0);		//turn off funnel solenoid
            //gearUnit1(0);		//turn off gear unit
            //gearUnit2(0);
            //rotDrum(0);		//turn off rotating drum
            T0CONbits.TMR0ON = 0;   //turn off timer
            // num9V = count9V;		//update number of batteries
            // numC = countC;
            // numAA = countAA;
            // numDrain = countDrain;
            // count9V = 0;
            // countC = 0;
            // countAA = 0;
            // countDrain = 0;
            // min = runMin;		//update run time
            // sec = runSec;
            LATC = LATC && 0b11111101; // RC1 = 0 enable keypad 
        }
         /*interrupt for turning funnelSol on and off
        
        //if solenoid is off, solOffTimer++;
        //else, solOnTimer++;
        doneTimer++;
        if (solOnTimer >= 152){		//4 seconds
            solOnTimer = 0;
            funnelSol(0);
        }
        if (solOffTimer >= 38){		//1 second
        	solOffTimer = 0;
        	funnelSol(1);
        if (doneTimer >= WAIT_TIME){	
			opTimer = 0;
            screenMode = FINISH;
            //funnelSol(0);		//turn off funnel solenoid
            //gearUnit1(0);		//turn off gear unit
            //gearUnit2(0);
            //rotDrum(0);		//turn off rotating drum
            T0CONbits.TMR0ON = 0;   //turn off timer
            // num9V = count9V;		//update number of batteries
            // numC = countC;
            // numAA = countAA;
            // numDrain = countDrain;
            // count9V = 0;
            // countC = 0;
            // countAA = 0;
            // countDrain = 0;
            LATC = LATC && 0b11111101; // RC1 = 0 enable keypad 
        }*/
    }	
}

void interrupt low_priority lowISR(void){
	/*
	if (RBIF && (screenMode == OPERATING)){	//when change is detected one of the UVD IR sensors	
		RBIF = 0;
		doneTimer = 0;
		//determine which sensor it is (in UVD1 or UVD2)
		//if (UVD1){
			//if (if sense1 senses something)		//battery just fell in 
				//wait 1 second
				//actuate UVD solenoid to close in on battery
				//measure voltage
				//logical circuit to determine category
				//add to battery counter
				//set appropriate platforms to rotate
			//else......					otherwise, platform just finished rotating
				//stop rotation of platforms
				//start gearUnit
		//if (UVD2){
			//if (if sense2 senses something)		//battery just fell in 
				//wait 1 second
				//actuate UVD solenoid to close in on battery
				//measure voltage
				//logical circuit to determine category
				//add to battery counter
				//set appropriate platforms to rotate
			//else......					otherwise, platform just finished rotating
				//stop rotation of platforms
				//start gearUnit
		}
	}


    */
}

