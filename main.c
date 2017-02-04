#include <xc.h>
#include <stdio.h>
#include "configBits.h"
#include "constants.h"
#include "lcd.h"
#include "I2C.h"

const char keys[] = "123A456B789C*0#D";
unsigned char screenMode = STANDBY;
unsigned char time[7];

void switchMenu(unsigned char left, unsigned char right, unsigned char key);
//toggles the interface "left" and "right" to show different logs

void main(void){ 
	TRISC = 0x00;
    TRISD = 0x00;   //All output mode
    TRISB = 0xFF;   //All input mode
    LATB = 0x00; 
    LATC = 0x00;
    ADCON0 = 0x00;  //Disable ADC
    ADCON1 = 0xFF;  //Set PORTB to be digital instead of analog default  
    INT1IE = 1;
    initLCD();
    //nRBPU = 0;
    I2C_Master_Init(10000); //Initialize I2C Master with 100KHz clock
    ei(); // Enable all interrupts

    while (1) {
        while (screenMode == STANDBY){	//standby mode
        	di();
            __lcd_home();
            printf("START:   PRESS *");
            __lcd_newline();
            printf("OPTIONS:  4 OR 6");
            ei();
            //making sure printing on LCD does not get interrupted
        }
        while (screenMode == OPERATING){	//while machine is running
        	di();
            __lcd_home();
            printf("RUNNING...      ");
            unsigned char i;
            for (i=0; i<10; i++){
                __lcd_home();
                __lcd_newline();
                printf("%02d              ", i);
                __delay_1s();
            }
            ei();
            //insert main operating code of the program
            screenMode = FINISH;
        }
        while (screenMode == FINISH){	//finish screen  
        	di();         
        	__lcd_home();
        	printf("DONE! PRESS *   ");
        	__lcd_newline();
        	printf("TO CONTINUE     ");
        	ei();
        }
        while (screenMode == RUN_TIME){	//shows the log of latest run time
        	di();
        	unsigned char min, sec;
        	// readRunTime(min, sec);	//hypothetical code to retrieve total number of batteries from memory
            
            min = 1;
            sec = 1;

        	__lcd_home();
        	printf("TOTAL RUN TIME: ");
        	__lcd_newline();
        	printf("%02x:%02x               ", min, sec);
        	ei();
        }
        while (screenMode == NUM_BAT){	//shows the log of total number of processed batteries 
        	unsigned char numBats;
        	//readTotalBats(numBats)	//hypothetical code to retrieve total number of batteries from memory

            numBats = 6;

        	__lcd_home();
        	printf("TOTAL NUMBER OF ");
        	__lcd_newline();
        	printf("BATTERIES: %02d   ", numBats);
        }
        while (screenMode == NUM_C){	//shows number of processed C batteries from the latest run
        	di();
        	unsigned char numC;
        	//readTotalC(numC)	//hypothetical code to retrieve total number of C batteries from memory

            numC = 3;
            
        	__lcd_home();
        	printf("NUMBER OF C     ");
        	__lcd_newline();
        	printf("BATTERIES: %02d   ", numC);
        	ei();
        }
        while (screenMode == NUM_9V){	//shows number of processed 9V batteries from the latest run
        	di();
        	unsigned char num9V;
        	//readTotal9V(num9V)	//hypothetical code to retrieve total number of 9V batteries from memory

            num9V = 10;

        	__lcd_home();
        	printf("NUMBER OF 9V    ");
        	__lcd_newline();
        	printf("BATTERIES: %02d     ", num9V);
        	ei();
        }
        while (screenMode == NUM_AA){	//shows number of processed AA batteries from the latest run
        	di();
        	unsigned char numAA;	
        	//readTotalAA(numAA)	//hypothetical code to retrieve total number of AA batteries from memory

            numAA = 2;
            
        	__lcd_home();
        	printf("NUMBER OF AA    ");
        	__lcd_newline();
        	printf("BATTERIES: %02x     ", numAA);
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
    if (key == right){   //if "right" button is pressed, toggle "right"
        if (screenMode == STANDBY)
            screenMode = RTC_DISPLAY;  	//loop to RTC display
        else
            screenMode -= 1;
    }
    else if (key == left){ //if "left" button is pressed, toggle "left"
        if (screenMode == RTC_DISPLAY)
            screenMode = STANDBY;	//loop back to standby
        else
            screenMode += 1;
    }
}   

void interrupt keypressed(void) {
    if(INT1IF){
        unsigned char keypress = (PORTB & 0xF0) >> 4;
        if (keys[keypress] == '*'){	
        	//press * to start operation or resume to standby after finish
        	if(screenMode == STANDBY)
        		screenMode = OPERATING;
        	else if (screenMode == FINISH)
        		screenMode = STANDBY;
        }
        else if (screenMode != FINISH) //edge case when user presses 4 or 6 at finish screen
        	switchMenu('4', '6', keys[keypress]);
        	// press 4 to toggle "left", 6 to toggle "right"
        INT1IF = 0;     //Clear flag bit
    }
}