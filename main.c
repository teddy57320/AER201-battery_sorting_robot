#include <xc.h>
#include <stdio.h>
#include "configBits.h"
#include "constants.h"
#include "lcd.h"
#include "I2C.h"
//#include "macros.h"

const char keys[] = "123A456B789C*0#D";
unsigned int screenMode = STANDBY;
unsigned char time[7];

void switchMenu(unsigned char left, unsigned char right, unsigned char key);
//toggles the interface "left" and "right" to show different logs

void printRTC(void);

void main(void){
    
	TRISC = 0x00;
    TRISD = 0x00;   //All output mode
    TRISB = 0xFF;   //All input mode
    LATB = 0x00; 
    LATC = 0x00;
    ADCON0 = 0x00;  //Disable ADC
    ADCON1 = 0xFF;  //Set PORTB to be digital instead of analog default  
    initLCD();
    //nRBPU = 0;
    I2C_Master_Init(10000); //Initialize I2C Master with 100KHz clock
    di(); // Disable all interrupts

    while (1) {
        while (screenMode == STANDBY){	//standby mode
            
            __lcd_home();
            printf("START:   PRESS *");
            __lcd_newline();
            printf("OPTIONS:  4 OR 6");
            
            while (PORTBbits.RB1 == 0){ 
                // RB1 is the interrupt pin, so if there is no key pressed, RB1 will be 0
                // the PIC will wait and do nothing until a key press is signaled
            }
            unsigned char keypress = (PORTB & 0xF0)>>4; // Read the 4 bit character code
            while (PORTBbits.RB1 == 1){
            // Wait until the key has been released
       		}
            if (keys[keypress] == '*')	//start operation is * is pressed
                screenMode = OPERATING;
            else
            	switchMenu('4', '6', keys[keypress]);	
            	//toggle interface "left" if 4 is pressed, and "right" if 6 is pressed
        }
        while (screenMode == OPERATING){	//while machine is running
            
            __lcd_home();
            printf("RUNNING...      ");
            __lcd_newline();
            printf("                ");

            //insert main operating code of the program

            __delay_1s();
            __delay_1s();
            __delay_1s();
            screenMode = FINISH;
        }
        while (screenMode == FINISH){	//finish screen
            
        	__lcd_home();
        	printf("DONE! PRESS *   ");
        	__lcd_newline();
        	printf("TO CONTINUE     ");

        	while (PORTBbits.RB1 == 0){ 
                // RB1 is the interrupt pin, so if there is no key pressed, RB1 will be 0
                // the PIC will wait and do nothing until a key press is signaled
            }
            unsigned char keypress = (PORTB & 0xF0)>>4; // Read the 4 bit character code
            while (PORTBbits.RB1 == 1){
            // Wait until the key has been released
       		}
            if (keys[keypress] == '*')	//if * is pressed
                screenMode = STANDBY;
        }
        while (screenMode == RUN_TIME){	//shows the log of latest run time
        	unsigned int min, sec;
        	// readRunTime(min, sec);	//hypothetical code to retrieve total number of batteries from memory
            
            min = 1;
            sec = 1;

        	__lcd_home();
        	printf("TOTAL RUN TIME: ");
        	__lcd_newline();
        	printf("%02x:%02x               ", min, sec);

        	while (PORTBbits.RB1 == 0){ 
                // RB1 is the interrupt pin, so if there is no key pressed, RB1 will be 0
                // the PIC will wait and do nothing until a key press is signaled
            }
            unsigned char keypress = (PORTB & 0xF0)>>4; // Read the 4 bit character code
            while (PORTBbits.RB1 == 1){
            // Wait until the key has been released
       		}
            switchMenu('4', '6', keys[keypress]);
        }
        while (screenMode == NUM_BAT){	//shows the log of total number of processed batteries 
        	unsigned int numBats;
        	//readTotalBats(numBats)	//hypothetical code to retrieve total number of batteries from memory

            numBats = 6;

        	__lcd_home();
        	printf("TOTAL NUMBER OF ");
        	__lcd_newline();
        	printf("BATTERIES: %02x   ", numBats);

        	while (PORTBbits.RB1 == 0){ 
                // RB1 is the interrupt pin, so if there is no key pressed, RB1 will be 0
                // the PIC will wait and do nothing until a key press is signaled
            }
            unsigned char keypress = (PORTB & 0xF0)>>4; // Read the 4 bit character code
            while (PORTBbits.RB1 == 1){
            // Wait until the key has been released
       		}
            switchMenu('4', '6', keys[keypress]);
        }
        while (screenMode == NUM_C){	//shows number of processed C batteries from the latest run
        	unsigned int numC;
        	//readTotalC(numC)	//hypothetical code to retrieve total number of C batteries from memory

            numC = 3;
            
        	__lcd_home();
        	printf("NUMBER OF C     ");
        	__lcd_newline();
        	printf("BATTERIES: %02x   ", numC);

        	while (PORTBbits.RB1 == 0){ 
                // RB1 is the interrupt pin, so if there is no key pressed, RB1 will be 0
                // the PIC will wait and do nothing until a key press is signaled
            }
            unsigned char keypress = (PORTB & 0xF0)>>4; // Read the 4 bit character code
            while (PORTBbits.RB1 == 1){
            // Wait until the key has been released
       		}
            switchMenu('4', '6', keys[keypress]);
        }
        while (screenMode == NUM_9V){	//shows number of processed 9V batteries from the latest run
        	unsigned int num9V;
        	//readTotal9V(num9V)	//hypothetical code to retrieve total number of 9V batteries from memory

            num9V = 10;

        	__lcd_home();
        	printf("NUMBER OF 9V    ");
        	__lcd_newline();
        	printf("BATTERIES: %02d     ", num9V);
        	while (PORTBbits.RB1 == 0){ 
                // RB1 is the interrupt pin, so if there is no key pressed, RB1 will be 0
                // the PIC will wait and do nothing until a key press is signaled
            }
            unsigned char keypress = (PORTB & 0xF0)>>4; // Read the 4 bit character code
            while (PORTBbits.RB1 == 1){
            // Wait until the key has been released
       		}
            switchMenu('4', '6', keys[keypress]);
        }
        while (screenMode == NUM_AA){	//shows number of processed AA batteries from the latest run
        	unsigned int numAA;	
        	//readTotalAA(numAA)	//hypothetical code to retrieve total number of AA batteries from memory

            numAA = 2;
            
        	__lcd_home();
        	printf("NUMBER OF AA    ");
        	__lcd_newline();
        	printf("BATTERIES: %02x     ", numAA);

        	while (PORTBbits.RB1 == 0){ 
                // RB1 is the interrupt pin, so if there is no key pressed, RB1 will be 0
                // the PIC will wait and do nothing until a key press is signaled
            }
            unsigned char keypress = (PORTB & 0xF0)>>4; // Read the 4 bit character code
            while (PORTBbits.RB1 == 1){
            // Wait until the key has been released
       		}
            switchMenu('4', '6', keys[keypress]);
        }
        while (screenMode == RTC_DISPLAY){	//real time/date display
           
            printRTC();
            while (PORTBbits.RB1 == 0){ 
                printRTC();
            }
            unsigned char keypress = (PORTB & 0xF0)>>4; // Read the 4 bit character code
            while (PORTBbits.RB1 == 1){
                printRTC();
       		}
       		switchMenu('4', '6', keys[keypress]);
            //__delay_1s();
        }
    }
    return;
}

void switchMenu(unsigned char left, unsigned char right, unsigned char key){
    if (key == right){   //if "right" button is pressed, toggle "right"
        if (screenMode == STANDBY)
            screenMode = RTC_DISPLAY;
        else
            screenMode -= 1;
    }
    else if (key == left){ //if "left" button is pressed, toggle "left"
        if (screenMode == RTC_DISPLAY)
            screenMode = STANDBY;
        else
            screenMode += 1;
    }
}   

void printRTC(void){
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
    printf("%02x/%02x/%02x        ", time[6],time[5],time[4]);    //Print date in YY/MM/DD
    __lcd_newline();
    printf("%02x:%02x:%02x        ", time[2],time[1],time[0]);    //HH:MM:SS
}