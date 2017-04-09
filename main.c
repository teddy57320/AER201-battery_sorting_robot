#include <xc.h>
#include <stdio.h>
#include "configBits.h"
#include "constants.h"
#include "lcd.h"
#include "I2C.h"
#include "eeprom_routines.h"

const char keys[] = "123A456B789C*0#D";

void keypressed(unsigned char left, unsigned char right, unsigned char key);
//handles all cases where key is pressed

void readADC(unsigned char channel);
//select analog channel to read

void stopOperation(void);
//stop battery sorting

void testBatteries(void);
//logic circuit for voltage-testing in UVD

void wait_2ms(unsigned int x);   //delay a certain number of seconds

unsigned char screenMode = STANDBY; //start at standby screen
unsigned char realTime[7];		                            //used to retrieve real time/date
unsigned char lastRunRTC[7];                            //store real time/date of last run

unsigned char opTimer;                                  //counters for counting run time
unsigned char doneTimer, solOnTimer;                    //done sorting flag if no battery is detected at after 10 seconds, turning on/off every second
unsigned char num9V, numC, numAA, numBats, numDrain;    //number of batteries of each kind sorted
unsigned char min, sec;                                 //store latest run time                              
unsigned char countC, countAA, count9V, countDrain;     //temporary values to count batteries during sorting
unsigned int plat2Left, plat1Right, plat2Right, plat1Left; //flags for platform motors
//plat1Left --> drained, counterclockwise
//plat1Right --> charged, clockwise
//plat2Left --> charged, counterclockwise
//plat2Right --> drained, clockwise
unsigned char startGear;  //state of the stepper, flag for initial spinning sequence of stepper motor
unsigned char step1, step2;     //tracking steps of the UVD platforms as per http://mechatronics.mech.northwestern.edu/design_ref/actuators/stepper_drive1.html
unsigned int turn1BackRight, turn1BackLeft, turn2BackRight, turn2BackLeft;     //flags for turning platforms back to their original positions
unsigned char sorting, doubleAA;          //flag for when the program is sorting
unsigned int count_2ms; //used as a general flag for timing

void main(void){ 

	TRISA = 0b00101111;		//RA0:3, RA5 used for voltage readings
    TRISB = 0b11110010;     //inputs: RB1, RB4:7 for keypad, RB3 for LCD
	TRISC = 0x00;			//all output
	TRISD = 0x00;			//all output 
    TRISE = 0b00000001;     //RE0 for voltage reading

	LATA = 0x00;            //reset all pin values
    LATB = 0x00;
    LATC = 0x00;
    LATD = 0x00;
    LATE = 0x00;

    ADCON0 = 0x00;          //disable ADC
	ADCON1 = 0b00001001;    //set RA0:3, RA5, RE0 to analog (AN0:5)
    ADCON2 = 0b10110001;
    //ADFM = 1;
    //ACQ[2:0] = 110;   16T_AD = 12.8us converting acquisition time
    //ADCS[2:0] = 001;  T_AD = 8*T_OSC = 0.8us > 0.7us minimum for pic18
 
    GIE = 1;        //globally enable interrupt
    PEIE = 1;       //enable peripheral interrupt
    INT1IE = 1;	    //enable external interrupt from RB1
    INT1IF = 0;	    //turn off external interrupt flag
    TMR0IE = 1;     //enable TMR0 overflow interrupt
    TMR0IF = 0;	    //turn off TMR0 overflow interrupt flag
    TMR1IE = 1;     //enable TMR1 overflow interrupt
    TMR1IF = 0;     //turn off TMR1 overflow interrupt flag

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
    /*implement the following in register T1CON:   
    RD16 = 1;
    T1RUN = 0;   //use only internal oscillator
    T1CKPS[1:0] = 00, 1:1 prescale; 
    T1OSCEN = 0;
    T1SYNC = 0;
    TMR1CS = 0;
    TMR1ON = 0; */
    TMR1 = 60535;
// time for overflow = (65535-60535)*4/10000000 = 2 milliseconds
//**********************************************************

/***********************************************************
	EEPROM SETUP*/
    // EEPGD = 0;	//enable access to EEPROM
    // CFGS = 0;	//access EEPROM
//**********************************************************
    initLCD();
    nRBPU = 0;
    I2C_Master_Init(10000); //Initialize I2C Master with 100KHz clock
    ei(); // Enable all interrupts

    while (1) {
        while (screenMode == STANDBY){	//standby mode
            __lcd_home();
            printf("PRESS * TO START");
            __lcd_newline();
            printf("< TOGGLE  LOGS >");
            for(unsigned char i=0;i<50;i++){
            	if (screenMode != STANDBY)	//ensure immediate scrolling
            		break;
            	__delay_ms(10);
            }
            __lcd_home();
            __lcd_newline();
            printf(" <TOGGLE  LOGS> ");
            for(unsigned char i=0;i<50;i++){
            	if (screenMode != STANDBY)
            		break;
            	__delay_ms(10);
            }
        }
        while (screenMode == OPERATING){	//machine is running
            // __lcd_home();
            // printf("RUNNING...      ");     
            // __lcd_newline();
            // printf("%02d                ", doneTimer);
           // __lcd_home();
           // __lcd_newline();
           // printf("PRESS # TO STOP ");
            readADC(0); //RA0
           __lcd_home();
           __lcd_newline();
           printf("%4d %2d         ", ADRES, countDrain+countAA+count9V+countC);
           if (startGear){
                wait_2ms(2000);
                startGear = 0;
                gearDir(0);
                doneTimer = 0;

                unsigned char steps = 0;
                while(steps<20 && screenMode==OPERATING){    //big stepper motor turning sequence 180deg
                    steps++;
                    gearStep(1);
                    __delay_ms(5);
                    gearStep(0);
                    __delay_ms(5);
                }
                steps = 0;
                while(steps < 178 && screenMode==OPERATING){
                    steps++;
                    gearStep(1);
                    __delay_ms(2);
                    gearStep(0);
                    __delay_ms(2);
                }
                steps = 0;
                while(steps <2 && screenMode==OPERATING){
                    steps++;
                    gearStep(1);
                    __delay_ms(5);
                    gearStep(0);
                    __delay_ms(5);
                }
                steps = 0;
            }

            //read UVD IR sensor RA0 = AN0
            if (ADRES < 22 | ADRES > 55){    //if battery is present   
                wait_2ms(250);    
                sorting = 1; 
                UVDsol(1);          //actuate wall
                wait_2ms(500);
                testBatteries();    
                UVDsol(0);          //pull back wall

                if (doubleAA)     //turn one platform first
                    plat1Right = 512;
                if (plat1Left)     //flag to turn platform1 left
                    step1 = 1;
                if (plat1Right)    //flag to turn platform1 right
                    step1 = 4;                
                if (plat2Left)     //flag to turn platform2 left
                    step2 = 1;
                if (plat2Right)    //flag to turn platform2 right
                    step2 = 4;

                turn1BackRight = plat1Left;   
                turn1BackLeft = plat1Right;   
                turn2BackRight = plat2Left;  
                turn2BackLeft = plat2Right;     

                plat1c1a(1);                    //prep platform turning sequence        
                plat1c1b(0);
                plat2c1a(1);
                plat2c1b(0);

                while((plat1Left|plat2Left|plat1Right|plat2Right) && screenMode==OPERATING);  //wait for platform to turn

                if (doubleAA){
                    plat2Left = 512;
                    step2 = 1;
                    turn2BackRight = plat2Left;
                }

                while (plat2Left && screenMode==OPERATING);         //wait for second platform to turn for AA
                wait_2ms(500);

                if (((countC + countAA + count9V + countDrain) >= 15))   //finish condition
                    stopOperation();

                plat1c1a(1);                    //prep for platforms turning back          
                plat1c1b(0);
                plat2c1a(1);
                plat2c1b(0);
                plat1Left = turn1BackLeft;      //turn platforms back
                plat1Right = turn1BackRight;
                plat2Left = turn2BackLeft;
                plat2Right = turn2BackRight;
   
                unsigned char steps = 0;
                while(steps<20 && screenMode==OPERATING){    //big stepper motor turning sequence
                    steps++;
                    gearStep(1);
                    __delay_ms(5);
                    gearStep(0);
                    __delay_ms(5);
                }
                steps = 0;
                while(steps < 178 && screenMode==OPERATING){
                    steps++;
                    gearStep(1);
                    __delay_ms(2);
                    gearStep(0);
                    __delay_ms(2);
                }
                steps = 0;
                while(steps<2 && screenMode==OPERATING){
                    steps++;
                    gearStep(1);
                    __delay_ms(5);
                    gearStep(0);
                    __delay_ms(5);
                }
                steps = 0;

                while((plat1Left|plat2Left|plat1Right|plat2Right) && screenMode==OPERATING);  //wait for platforms to turn back
                
                gearStep(0);            //reset all stepper motor pins
                gearDir(0);
                plat1Right = 0;
                plat2Right = 0;
                plat1Left = 0;
                plat2Left = 0;
                turn1BackRight = 0;
                turn1BackLeft = 0;
                turn2BackRight = 0;
                turn2BackLeft = 0;
                plat1c1a(0);
                plat1c1b(0);
                plat1c2b(0);
                plat1c2a(0);
                plat2c1a(0);
                plat2c1b(0);
                plat2c2b(0);
                plat2c2a(0);
                doubleAA = 0;
                sorting = 0;
            }                       
            wait_2ms(250);       
        }
        while (screenMode == FINISH){	//finish screen  
        	__lcd_home();
        	printf("DONE! PRESS *   ");
        	__lcd_newline();
        	printf("TO CONTINUE     ");
        }
        while (screenMode == RUN_TIME){	//shows the log of latest run time
        	__lcd_home();
        	printf("TOTAL RUN TIME: ");
        	__lcd_newline();
        	printf("%02d:%02d               ", min, sec);
        }
        while (screenMode == NUM_BAT){	//shows the log of total number of processed batteries 
        	__lcd_home();
        	printf("TOTAL # OF      ");
        	__lcd_newline();
        	printf("BATTERIES: %02d   ", numBats);
        }
        while (screenMode == NUM_C){	//shows number of processed C batteries from the latest run
        	__lcd_home();
        	printf("# OF C          ");
        	__lcd_newline();
        	printf("BATTERIES: %02d   ", numC);
        }
        while (screenMode == NUM_9V){	//shows number of processed 9V batteries from the latest run
        	__lcd_home();
        	printf("# OF 9V         ");
        	__lcd_newline();
        	printf("BATTERIES: %02d     ", num9V);
        }
        while (screenMode == NUM_AA){	//shows number of processed AA batteries from the latest run
        	__lcd_home();
        	printf("# OF AA         ");
        	__lcd_newline();
        	printf("BATTERIES: %02d     ", numAA);
        }
        while (screenMode == NUM_DRAIN){
        	__lcd_home();
        	printf("# OF DRAINED    ");
        	__lcd_newline();
        	printf("BATTERIES: %02d     ", numDrain);
        }
        while (screenMode == RTC_LAST_RUN){
            __lcd_home();
            printf("LAST RUN:       ");    
            __lcd_newline();
            printf("%02x/%02x/%02x        ", lastRunRTC[6],lastRunRTC[5],lastRunRTC[4]);    //YY/MM/DD
            for(unsigned char i=0;i<200;i++){
                if (screenMode != RTC_LAST_RUN)  //ensure immediate scrolling
                    break;
                __delay_ms(10);
            }
            __lcd_home();
            __lcd_newline();
            printf("%02x:%02x:%02x        ", lastRunRTC[2],lastRunRTC[1],lastRunRTC[0]);    //HH:MM:SS  
            for(unsigned char i=0;i<200;i++){
                if (screenMode != RTC_LAST_RUN)  //ensure immediate scrolling
                    break;
                __delay_ms(10);
            }   
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
		        realTime[i] = I2C_Master_Read(1);
		    }
		    realTime[6] = I2C_Master_Read(0);       //Final Read without ack
		    I2C_Master_Stop();
		    __lcd_home();
		    printf("DATE: %02x/%02x/%02x  ", realTime[6],realTime[5],realTime[4]);    //Print date in YY/MM/DD
		    __lcd_newline();
		    printf("TIME: %02x:%02x:%02x  ", realTime[2],realTime[1],realTime[0]);    //HH:MM:SS	         
        }
    }
    return;
}

void keypressed(unsigned char left, unsigned char right, unsigned char key){
    if (key == '*'){ 
        //press * to start operation or resume to standby after finish
        if(screenMode == STANDBY){
            screenMode = OPERATING;
            T0CONbits.TMR0ON = 1; //turn on TMR0 
            T1CONbits.TMR1ON = 1; //turn on TMR1
            startGear = 1;

            //store real time/date of start of run
            I2C_Master_Start(); //Start condition
            I2C_Master_Write(0b11010000); //7 bit RTC address + Write
            I2C_Master_Write(0x00); //Set memory pointer to seconds
            I2C_Master_Stop(); //Stop condition
            //Read Current Time
            I2C_Master_Start();
            I2C_Master_Write(0b11010001); //7 bit RTC address + Read
            for(unsigned char i=0;i<0x06;i++){
                lastRunRTC[i] = I2C_Master_Read(1);
            }
            lastRunRTC[6] = I2C_Master_Read(0);       //Final Read without ack
            I2C_Master_Stop();

            __lcd_home();
            printf("RUNNING: 00:00  "); 
            funnelSol(1);
        }
        else if (screenMode == FINISH)
            screenMode = STANDBY;
    }
    else if (screenMode == OPERATING){
        if (key == '#')     //emergency stop
            //countDrain--;       //weird edge case
            stopOperation();
    }
    else if (screenMode != FINISH){ //edge case when user presses 4 or 6 at finish screen
        if (key == right){   	             //if "right" button is pressed, toggle "right"
            if (screenMode == STANDBY) 
                screenMode = RTC_DISPLAY;  	 //jump to RTC display
            else
                screenMode--;
        }
        else if (key == left){ 	             //if "left" button is pressed, toggle "left"
            if (screenMode == RTC_DISPLAY)
                screenMode = STANDBY;	     //jump back to standby
            else
                screenMode++;
        }
    }
}   

void readADC(unsigned char channel){
// Select A2D channel to read
	ADCON0 = channel << 2;
	ADCON0bits.ADON = 1;
	ADCON0bits.GO = 1;
	while(ADCON0bits.GO_NOT_DONE);  
}

void stopOperation(void){
    T0CONbits.TMR0ON = 0;   //turn off timers
    T1CONbits.TMR1ON = 0;
    TMR0 = 55770;
    TMR1 = 60535;
    num9V = count9V;     //update number of batteries
    numC = countC;
    numAA = countAA;
    numDrain = countDrain;
    numBats = count9V + countC + countAA + countDrain;
    count9V = 0;
    countC = 0;
    countAA = 0;
    countDrain = 0;
    min = opTimer / 60; //store run time
    sec = opTimer % 60;
    opTimer = 0;        //rest all timers and flags
    doneTimer = 0;
    sorting = 0;
    plat1Left = 0;
    plat1Right = 0;
    plat2Left = 0;
    plat2Right = 0;
    turn2BackLeft = 0;
    turn1BackLeft = 0;
    turn2BackRight = 0;
    turn1BackRight = 0;
    count_2ms = 0;
    doubleAA = 0;
    solOnTimer = 0;
    //reset all pins
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
    screenMode = FINISH;
}

void testBatteries(void){

    // unsigned int i = opTimer % 2;
    // if(i){
    //     countDrain++;   
    //     plat1Left = 1;
    //     plat2Right = 1;
    //     return;
    // }
    // unsigned int j = opTimer * opTimer %3;
    // if(j == 0)
    //     count9V++;
    // if (j == 1)
    //     countC++;
    // if (j==2)
    //     countAA++;
    // plat1Right = 1; //charged
    // plat2Left = 1;
    // return;
    
    readADC(1);         //read C circuit RA1
    unsigned int volt1 = ADRES;
    readADC(2);         //read AA circuit RA2
    unsigned int volt2 = ADRES;
    readADC(3);         //read AA circuit RA3
    unsigned int volt3 = ADRES;
    readADC(4);         //read 9V circuit RA5
    unsigned int volt4 = ADRES;
    readADC(5);         //read 9V circuit RE0
    unsigned int volt5 = ADRES;

    __lcd_home();
    printf("%04d %04d %04d", volt1, volt2, volt3);
    __lcd_newline();
    printf("%04d %04d       ", volt4, volt5);

    if (volt1){     //charged C battery
        countC++;
        plat1Right = 512;
        plat2Left = 512;
        return;
    }
    if (!(volt1 | volt2 | volt3 | volt4 | volt5)){  //drained battery 
        countDrain++;
        plat1Left = 512;
        plat2Right = 512;
        return;
    }
    if (volt2 && volt3){            //two charged AA batteries
        countAA = countAA + 2;
        doubleAA = 1;
        return;
    }     
    if (volt4 >200 | volt5 > 200){      //charged 9V battery
        plat1Right = 512;
        plat2Left = 512;
        count9V++;
        return;
    }
    if (volt4 > 80 | volt5 > 80){       //drained 9V battery
        countDrain++;
        plat1Left = 512;
        plat2Right = 512;
        return;
    }
    if (volt2 | volt3){        //one charged AA
        countAA++;
        if (volt4 && volt5){
            plat1Right = 512;
            plat2Left = 512;
            return;
        }
        if (volt2){            //charged AA on first platform
            plat1Right = 512;   //charged
            plat2Right = 512;   //drained
        }
        else{                   //charged AA on second platform
            plat2Left = 512;    //charged
            plat1Left = 512;    //drained
        }
        return;
    }

    //otherwise, assume drained
    plat1Left = 512;
    plat2Right = 512;
    countDrain++;
    return;
}

void wait_2ms(unsigned int x){
    count_2ms = x;
    while (count_2ms && screenMode == OPERATING);
}

void interrupt ISR(void) {
    if (INT1IF){
        unsigned char keypress = (PORTB & 0xF0) >> 4;   //detect key pressed on keypad
        keypressed('4', '6', keys[keypress]);           //scroll logs with '4' and '6'
        INT1IF = 0;     //clear flag bit
    }
    if (screenMode == OPERATING && TMR0IF){   //timer overflows every second
        TMR0IF = 0;
        TMR0 = 55770;       //timer preset value
        opTimer++;
        min = opTimer / 60; //store run time
        sec = opTimer % 60;
        // __lcd_home();
        // printf("RUNNING: %02d:%02d   ", min, sec); 

        if (opTimer >= 180)    //stop operation after 3 minutes
            stopOperation();            
        //funnelSol(!LATBbits.LB0);   //turn big solenoid on and off every second
        if (!sorting){              //UVD does not detect a battery for WAIT_TIME seconds
            if (ADRES > 50)
                doneTimer++;
            else
                doneTimer = 0;
            if (doneTimer >= WAIT_TIME)
                stopOperation();
            
        }
        else
            doneTimer = 0;
    }
    if (screenMode == OPERATING && TMR1IF){   //timer overflows every 2 milliseconds
        TMR1IF = 0;
        TMR1 = 60535;
        if (count_2ms)                        //flag decreases every 2 ms --> used as a "timer"
            count_2ms--;
        solOnTimer++;
        if (solOnTimer >= 150){
            solOnTimer = 0;
            funnelSol(!LATBbits.LB0);         //turn solenoid on and off every 0.4 seconds
        }
        if (plat1Left){          //drained
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
            plat1Left--;
            if (step1>=4)
                step1 = 1;
            else
                step1++;    
        }
        if (plat1Right){        //charged
            if (step1 == 4){
                plat1c2a(0);    //step4
                plat1c2b(1);
            }
            if (step1 == 3){
                plat1c1a(0);    //step3
                plat1c1b(1);
            }
            if (step1 == 2){
                plat1c2a(1);    //step2
                plat1c2b(0);
            }
            if (step1 == 1){
                plat1c1a(1);    //step1
                plat1c1b(0);
            }
            plat1Right--;
            if (step1<=1)
                step1 = 4;
            else
                step1--;
        }
        if (plat2Left){          //charged
            if (step2 == 1){
                plat2c2a(1);     //step1
                plat2c2b(0);
            }
            if (step2 == 2){
                plat2c1a(0);     //step2
                plat2c1b(1);
            }
            if (step2 == 3){
                plat2c2a(0);     //step3
                plat2c2b(1);
            }
            if (step2 == 4){
                plat2c1a(1);     //step4
                plat2c1b(0);
            }
            plat2Left--;
            if (step2>=4)
                step2 = 1;
            else
                step2++;    
        }
        if (plat2Right){        //drained
            if (step2 == 4){
                plat2c2a(0);    //step4
                plat2c2b(1);
            }
            if (step2 == 3){
                plat2c1a(0);    //step3
                plat2c1b(1);
            }
            if (step2 == 2){
                plat2c2a(1);    //step2
                plat2c2b(0);
            }
            if (step2 == 1){
                plat2c1a(1);    //step1
                plat2c1b(0);
            }
            plat2Right--;
            if (step2<=1)
                step2 = 4;
            else
                step2--;
        }
    }	
}