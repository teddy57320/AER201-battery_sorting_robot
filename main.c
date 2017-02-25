#include <xc.h>
#include <stdio.h>
#include "configBits.h"
#include "constants.h"
#include "lcd.h"
#include "I2C.h"
#include "eeprom_routines.h"
#include "motors.h"

const char keys[] = "123A456B789C*0#D";

void switchMenu(unsigned char left, unsigned char right, unsigned char key);
//toggles the interface "left" and "right" to show different logs

void readADC(unsigned char channel);
//select A2D channel to read

void stopOperation(void);
//stop battery sorting

unsigned char time[7];		//used to retrieve real time/date
unsigned char opTimer, solOnTimer;
unsigned int doneTimer;	//counters
unsigned char num9V, numC, numAA, numBats, numDrain;    //number of batteries of each kind sorted
unsigned char min, sec;                              
unsigned char countC, countAA, count9V, countDrain;     //temp values to count batteries
unsigned char plat1Left = 1;
unsigned char plat1Right, plat2Left, plat2Right; 
unsigned char step1, step2;


void main(void){ 

    // TRISA = 0xFF;
    // TRISC = 0x00;
    // TRISD = 0x00;   //All output mode
    // TRISB = 0xFF;   //All input mode
    // LATA = 0x00;
    // LATB = 0x00; 
    // LATC = 0x00;

    // ADCON0 = 0x00;  //Disable ADC
    // ADCON1 = 0xFF;  //Set PORTB to be digital instead of analog default  
    	
	TRISA = 0b11000011;		//RA0 and RA1 used for analog inputs
    TRISB = 0b11110010;     //inputs: RB4:7 for keypad, RB1 for LCD
	TRISC = 0x00;			//all output
	TRISD = 0x00;			//all output 
    TRISE = 0x00;           //all output

	LATA = 0x00;
    LATB = 0x00;
    LATC = 0x00;
    LATD = 0x00;
    LATE = 0x00;

    ADCON0 = 0x00;          //disable ADC
	ADCON1 = 0x0E;    //set RA0 and RA1 to analog, everything else to digital
    ADFM = 1;               //right justified
 
    GIE = 1;	//globally enable interrupt
    PEIE = 1;   //enable peripheral interrupt
    INT1IE = 1;	//enable external interrupt.-
    INT1IF = 0;	//turn off external interrupt flag
    // RBIE = 0;	//enable PORTB on change interrupt 
    TMR0IE = 1; //enable timer0 overflow interrupt
    TMR0IF = 0;	//turn off timer0 overflow interrupt flag
    TMR1IE = 1;
    TMR1IF = 0;

/***********************************************************
    TMR0 SETUP*/
    T0CON = 0b00000111; 
/*implement the following in register T0CON:   
   TMR0ON = 0; //turn off timer
   T08BIT = 0; //use 16-bit timer
   T0CS = 0;   //use internal clock
   //TOSE not changed
   PSA = 0;    //enable prescalar
   T0PS2:T0PS0 = 111 //prescale 1:256 */
    TMR0 = 55770;
// time for timer overflow
// = number of ticks to overflow * seconds per tick
// = (2^16-1-TimeresetValue)*(1/(_XTAL_FREQ/4/prescale))
// = (65535-55770)*4*256/10000000 = 1 second
//**********************************************************

/***********************************************************
    TMR1 SETUP*/
    T1CON = 0b10000000; 
/*implement the following in register T0CON:   
   RD16 = 1;
   T1RUN = 0;
   T1CKPS1:0 = 00;
   T1OSCEN = 0;
   T1SYNC = 0;
   TMR1CS = 0;
   TMR1ON = 0; */
    TMR1 = 48035;
// time for timer overflow
// = number of ticks to overflow * seconds per tick
// = (2^16-1-TimeresetValue)*(1/(_XTAL_FREQ/4/prescale))
// = (65535-48035)*4/10000000 = 7 milliseconds
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
        while (screenMode == OPERATING){	//machine is running
            __lcd_home();
            printf("RUNNING...      ");     
            __lcd_newline();
            printf("PRESS # TO STOP ");
            //readADC(0);         //read UVD IR sensor 
            // turnPlatsLeft(200);
            // turnPlat1Left(200);
            // turnPlatsRight(200);
            // turnPlat2Right(200);

            // if (((ADRESH<<8)+ADRESL)<200){
            //     __delay_ms(10);
            //     readADC(0);

            // }
            if (plat1Left){
                 plat1c1a(1);
                 plat1c1b(0);
                 step1 = 1;
                 T1CONbits.TMR1ON = 1;       //doesnt reset TMR1
                 while(plat1Left){}
            }
  
/*************************************************
			// BATTERY-SORTING OPERATION           */
/*         
            funnelSol(0);		//turn on funnel solenoid
            r.........
            gearUnit1(1);		//turn on stepper motor

            while(1){
 *              
 *          }  
        	


*/  
//************************************************
            // if ((countC + countAA + count9V + countDrain) >= 15)
            //     stopOperation();
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
        	printf("%02d:%02d               ", min, sec);
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
        while (screenMode == STOP){
            di();
            __lcd_home();
            printf("EMERGENCY STOP  ");
            __lcd_newline();
            printf("                ");
            __delay_ms(2000); 
            screenMode = STANDBY;
            ei();
        }
    }
    return;
}

void switchMenu(unsigned char left, unsigned char right, unsigned char key){
    if (key == right){   	//if "right" button is pressed, toggle "right"
        if (screenMode == STANDBY)
            screenMode = RTC_DISPLAY;  	//loop to RTC display
        else
            screenMode--;
    }
    else if (key == left){ 	//if "left" button is pressed, toggle "left"
        if (screenMode == RTC_DISPLAY)
            screenMode = STANDBY;	//loop back to standby
        else
            screenMode++;
    }
}   

void readADC(char channel){
// Select A2D channel to read
	ADCON0 = ((channel <<2));
	ADCON0bits.ADON = 1;
	ADCON0bits.GO = 1;
	while(ADCON0bits.GO_NOT_DONE){}    
}

void stopOperation(void){
    T0CONbits.TMR0ON = 0;   //turn off timer
    T1CONbits.TMR1ON = 0;
    TMR0 = 55770;
    TMR1 = 48035;
    // num9V = count9V;     //update number of batteries
    // numC = countC;
    // numAA = countAA;
    // numDrain = countDrain;
    // count9V = 0;
    // countC = 0;
    // countAA = 0;
    // countDrain = 0;
    min = opTimer / 60; //store run time
    sec = opTimer % 60;
    opTimer = 0;
    solOnTimer = 0;
    doneTimer = 0;
    plat1c1a(0);
    plat1c1b(0);
    plat1c2b(0);
    plat1c2a(0);
    plat2c1a(0);
    plat2c1b(0);
    plat2c2b(0);
    plat2c2a(0);
    gearDir(0);
    UVDsol(0);
    gearStep(0);
    funnelSol(0);
}

void interrupt high_priority highISR(void) {
    if (INT1IF){
        unsigned char keypress = (PORTB & 0xF0) >> 4;
        if (keys[keypress] == '*'){	
        	//press * to start operation or resume to standby after finish
        	if(screenMode == STANDBY){
        		screenMode = OPERATING;
                T0CONbits.TMR0ON = 1; //turn on TMR0 
                T1CONbits.TMR1ON = 0; //TMR1 off when not driving steppers
            }
        	else if (screenMode == FINISH)
        		screenMode = STANDBY;
        }
        else if (screenMode == OPERATING){
            if (keys[keypress] == '#'){     //emergency stop
                screenMode = STOP;
                stopOperation();
            }
        }
        else if (screenMode != FINISH) //edge case when user presses 4 or 6 at finish screen
        	switchMenu('4', '6', keys[keypress]);
        	// press 4 to toggle "left", 6 to toggle "right"
        INT1IF = 0;     //Clear flag bit
    }
    if (screenMode == OPERATING && TMR0IF){   //timer overflows every second
        TMR0IF = 0;
        TMR0 = 55770;
        opTimer++;        
        if (opTimer >= 180){    //stop operation after 3 minutes
            screenMode = FINISH;
            stopOperation();    
        }        
        // //doneTimer++;
        // if (doneTimer >= WAIT_TIME){	
        //     screenMode = FINISH;
        //     stopOperation();
        //}
        if (LATCbits.LC0){  //after solenoid is on for one second, turn it off
            funnelSol(0);
        }
        else {
            solOnTimer++;
            if (solOnTimer >= 4){   //turn on solenoid once every 4 seconds
                solOnTimer = 0;
                funnelSol(1);
            }
        }
    }
    if (screenMode == OPERATING && TMR1IF){   //timer overflows every 7 milliseconds
        TMR1IF = 0;
        TMR1 = 48035;
        if (plat1Left){
            if (step1 == 1){
                plat1c2a(1);     //step1
                plat1c2b(0);
            }
            if (step1 == 2){
                plat1c1a(0);     //step2
                plat1c1b(1);
            }
            if (step1 == 3){
                plat1c2a(0);     //step3
                plat1c2b(1);
            }
            if (step1 == 4){
                plat1c1a(1);     //step4
                plat1c1b(0);
            }
            plat1Left++;
            if (plat1Left>=200){
                plat1Left = 0;
                T1CONbits.TMR1ON = 0;
                step1 = 0;
            }
            else if (step1>=4)
                step1 = 1;
            else
                step1++;    
        }
        // if (plat1Right){
        //     if (step1 == 4){
        //         plat1c2a(0);    //step4
        //         plat1c2b(1);
        //     }
        //     if (step1 == 3){
        //         plat1c1a(0);    //step3
        //         plat1c1b(1);
        //     }
        //     if (step1 == 2){
        //         plat1c2a(1);    //step2
        //         plat1c2b(0);
        //     }
        //     if (step1 == 1){
        //         plat1c1a(1);    //step1
        //         plat1c1b(0);
        //     }
        //     if (plat1Right>=200){
        //         plat1Right = 0;
        //         T1CONbits.TMR1ON = 0;
        //         step1 = 0;
        //     }
        //     if (step1<=1)
        //         step1 = 4;
        //     else
        //         step1--;
        // }
        // if (plat2Left){

        // }
        // if (plat2Right){

        // }

        // doneTimer++;          
        // if (doneTimer >= 2000){     
        //     screenMode = FINISH;
        //     stopOperation();
        // }
    }	
}

void interrupt low_priority lowISR(void){
	/*
	if (RBIF && (screenMode == OPERATING){	//when change is detected one of the UVD IR sensors	
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

