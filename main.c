#include <xc.h>
#include <stdio.h>
#include "configBits.h"
#include "constants.h"
#include "lcd.h"
#include "I2C.h"
#include "eeprom_routines.h"

const char keys[] = "123A456B789C*0#D";

void switchMenu(unsigned char left, unsigned char right, unsigned char key);
//toggles "left" and "right" to show scroll through different logs

void readADC(unsigned char channel);
//select analog channel to read

void stopOperation(void);
//stop battery sorting

void testBatteries(void);
//logic circuit for voltage-testing in UVD

int isFluctuate(unsigned char channel);
//reads voltage and determines if it is fluctuating (fluctuation means no battery is being measured)

int abs(int x){     //absolute value function
    if (x<0)
        return -x;
    return x;
}

unsigned char time[7];		                            //used to retrieve real time/date
unsigned char opTimer, solOnTimer = 2, waitPlats;       //counters for counting run time, solenoid running, and 2 second wait for platform turning, respectively
unsigned char doneTimer;                                //done sorting flag if no battery is detected at after 10 seconds
unsigned char num9V, numC, numAA, numBats, numDrain;    //number of batteries of each kind sorted
unsigned char min, sec;                                 //store latest run time                              
unsigned char countC, countAA, count9V, countDrain;     //temporary values to count batteries during sorting
unsigned int plat2Left, plat1Right, plat2Right, plat1Left; //flags for platform motors
//plat1Left --> drained, counterclockwise
//plat1Right --> charged, clockwise
//plat2Left --> charged, counterclockwise
//plat2Right --> drained, clockwise
unsigned int stepAmount;          //flags for turning gear stepper
unsigned char stepGear, startGear;
unsigned char step1, step2;     //tracking steps of the UVD platforms as per http://mechatronics.mech.northwestern.edu/design_ref/actuators/stepper_drive1.html
unsigned char turn1BackRight, turn1BackLeft, turn2BackRight, turn2BackLeft;     //flags for turning platforms back to their original positions
unsigned char sorting;

void main(void){ 

	TRISA = 0b00000011;		//RA0, RA1 used for analog inputs
    TRISB = 0b11110010;     //inputs: RB4:7 for keypad, RB1 for keypad, RB3 for LCD
	TRISC = 0x00;			//all output
	TRISD = 0x00;			//all output 
    TRISE = 0x00;           //all output

	LATA = 0x00;            //reset all pin values
    LATB = 0x00;
    LATC = 0x00;
    LATD = 0x00;
    LATE = 0x00;

    ADCON0 = 0x00;          //disable ADC
	ADCON1 = 0x0E;          //set RA0, RA1 to analog, everything else to digital
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
    TMR1 = 58035;
// time for overflow = (65535-58035)*4/10000000 = 3 milliseconds
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
            __lcd_home();
            printf("START:   PRESS *");
            __lcd_newline();
            printf("< 4   DATA   6 >");
            for(unsigned char i=0;i<50;i++){
            	if (screenMode != STANDBY)	//ensure immediate scrolling
            		break;
            	__delay_ms(10);
            }
            __lcd_home();
            __lcd_newline();
            printf(" <4   DATA   6> ");
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
           // printf("RUNNING...      ");  
           readADC(0);
           __lcd_home();
           __lcd_newline();
           printf("%4d %2d         ", ADRES, countDrain+countAA+count9V+countC);

           if (startGear){
                __delay_ms(4000);
                startGear = 0;
                gearDir(0);
                for (unsigned int i = 0; i < 420; i++){ 
                    if (screenMode != OPERATING)
                        break;
                    gearStep(!LATCbits.LC2);
                    __delay_ms(3);
                }
                gearDir(1);
                for (unsigned char i = 0; i < 40; i++){
                    if (screenMode != OPERATING)
                        break;
                    gearStep(!LATCbits.LC2);
                    __delay_ms(3);
                }
                gearDir(0);
                for (unsigned char i = 0; i < 20; i++){
                    if (screenMode != OPERATING)
                        break;                    
                    gearStep(!LATCbits.LC2);
                    __delay_ms(3);
                }
                doneTimer = 0;
           }

            // if (!isFluctuate(0)){  //read UVD IR sensor AN0
            if (ADRES<50){    //if battery is present       
                sorting = 1; 
                UVDsol(1);          //actuate wall
                testBatteries();   
                UVDsol(0);          //pull back wall
                if (plat1Left){     //flag to turn platform1 left
                    step1 = 1;
                    turn1BackRight = 1;
                    T1CONbits.TMR1ON = 1;
                }
                if (plat1Right){    //flag to turn platform1 right
                    step1 = 4;
                    turn1BackLeft = 1;
                    T1CONbits.TMR1ON = 1;
                }
                if (plat2Left){     //flag to turn platform2 left
                    step2 = 1;
                    turn2BackRight = 1;
                    T1CONbits.TMR1ON = 1;
                }
                if (plat2Right){    //flag to turn platform2 right
                    step2 = 4;
                    turn2BackLeft = 1;
                    T1CONbits.TMR1ON = 1;
                }                
                plat1c1a(1);                    //prep for platforms turning back          
                plat1c1b(0);
                plat2c1a(1);
                plat2c1b(0);

                while((plat1Left|plat2Left|plat1Right|plat2Right) && screenMode==OPERATING);  //wait for platform to turn
                waitPlats = 2;
                while(waitPlats && screenMode == OPERATING);    //delays for 2 seconds to allow batteries to fall through
                
                plat1c1a(1);                    //prep for platforms turning back          
                plat1c1b(0);
                plat2c1a(1);
                plat2c1b(0);
                plat1Left = turn1BackLeft;      //turn platforms back
                plat1Right = turn1BackRight;
                plat2Left = turn2BackLeft;
                plat2Right = turn2BackRight;     
                stepGear = DROP_BAT;            //initiate stepper routine  
                gearDir(0);                     //counterclockwise                          

                while(((stepGear!=STATIONARY) | (plat1Left|plat2Left|plat1Right|plat2Right)) && screenMode==OPERATING);  //wait for platforms to turn back

                T1CONbits.TMR1ON = 0;   //stop timer for motors
                gearStep(0);            //reset all stepper motor pins
                gearDir(0);
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
                if (((countC + countAA + count9V + countDrain) >= 15)){   //finish condition
                    screenMode = FINISH;
                    stopOperation();
                } 
                sorting = 0;
            }                       
            __delay_ms(500);                                        
            // }     
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
            __lcd_home();
            printf("EMERGENCY STOP  ");
            __lcd_newline();
            printf("                ");
            __delay_ms(2000); 
            screenMode = STANDBY;
        }
    }
    return;
}

void switchMenu(unsigned char left, unsigned char right, unsigned char key){

    if (key == '*'){ 
        //press * to start operation or resume to standby after finish
        if(screenMode == STANDBY){
            screenMode = OPERATING;
            T0CONbits.TMR0ON = 1; //turn on TMR0 
            T1CONbits.TMR1ON = 0; //TMR1 off when not driving steppers
            startGear = 1;
            __lcd_home();
            printf("RUNNING: 00:00  "); 
            funnelSol(1);
        }
        else if (screenMode == FINISH)
            screenMode = STANDBY;
    }
    else if (screenMode == OPERATING){
        if (key == '#'){     //emergency stop
            screenMode = STOP;
            countDrain--;       //weird edge case
            stopOperation();
        }
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
	ADCON0 = channel <<2;
	ADCON0bits.ADON = 1;
	ADCON0bits.GO = 1;
	while(ADCON0bits.GO_NOT_DONE);  
}

void stopOperation(void){
    T0CONbits.TMR0ON = 0;   //turn off timers
    T1CONbits.TMR1ON = 0;
    TMR0 = 55770;
    TMR1 = 58035;
    num9V = count9V;     //update number of batteries
    numC = countC;
    numAA = countAA;
    numDrain = countDrain;
    count9V = 0;
    countC = 0;
    countAA = 0;
    countDrain = 0;
    min = opTimer / 60; //store run time
    sec = opTimer % 60;
    opTimer = 0;        //rest all timers and flags
    solOnTimer = 0;
    doneTimer = 0;
    waitPlats = 0;
    stepGear = STATIONARY;
    stepAmount = 0;
    sorting = 0;
    plat1Left = 0;
    plat1Right = 0;
    plat2Left = 0;
    plat2Right = 0;
    turn2BackLeft = 0;
    turn1BackLeft = 0;
    turn2BackRight = 0;
    turn1BackRight = 0;
    trans1(0);          //reset all pins
    trans2(0);
    trans3(0);
    trans4(0);
    trans5(0);
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

void testBatteries(void){
    trans1(1);      //enable first 9V battery circuit
    // if (!isFluctuate(1)){
    if (ADRES>=706){  // (9*0.85 - 0.75)/2 = 3.45V, 3.45/5 * 1023 = 705.9
        trans1(0);
        count9V++;
        plat1Right = 1; //charged
        plat2Left = 1;
        return;
    }
    else if (ADRES>=5){      // 5 --> minimum analog reading for drained battery
        trans1(0);
        countDrain++;   //below 85% of nominal voltage --> drained
        plat1Left = 1;
        plat2Right = 1;
        return;
    }
    //return;
    //}
    trans1(0);
    trans2(1);      //enable second 9V circuit
    //if (!isFluctuate(1)){
    if (ADRES>=706){  
        trans2(0);
        count9V++;
        plat1Right = 1; //charged
        plat2Left = 1;
        return;
    }
    else if (ADRES>=5){
        trans2(0);
        countDrain++;   //drained
        plat1Left = 1;
        plat2Right = 1;
        return;
    }
    //return;
    //}
    trans2(0);
    trans3(1);      //enable C battery circuit
    //if (!isFluctuate(1)){
    if (ADRES>=54){  // (1.5*0.85 - 0.75)/2 = 0.2625V, 0.2625/5 * 1023 = 53.7
        trans3(0);
        countC++;
        plat1Right = 1;     //charged
        plat2Left = 1;
        return;
    }
    else if (ADRES>=5){
        trans3(0);          //drained
        countDrain++;
        plat1Left = 1;
        plat2Right = 1;
        return;
    }
    //return;
    //}
    trans3(0);
    trans4(1);      //enable AA circuit
    //if (!isFluctuate(1)){
    if (ADRES>=54){  
        trans4(0);
        countAA++;
        plat1Right = 1;
    }
    else if (ADRES>=5){
        trans4(0);
        countDrain++;
        plat1Left = 1;
    }
   // }
    trans4(0);
    trans5(1);      //enable second AA circuit
    //if (!isFluctuate(1)){
    if (ADRES>=54){  
        trans5(0);
        countAA++;
        plat2Left = 1;
    }
    //else if (ADRES>=5){
    else{           //battery must be a drained AA
        trans5(0);
        countDrain++;
        plat2Right = 1;
    }
    //}
    trans5(0);
    return;
}

int isFluctuate(unsigned char channel){
    readADC(channel);                                  //read voltage
    int tempVoltage = ADRES;
    for(unsigned char i = 0; i < 10; i++){             //check for fluctuation
        __delay_ms(1);
        readADC(channel);
        if (abs(ADRES-tempVoltage)>20)    //fluctuation means no battery (floating)
            return 1;
    }
    return 0;
}

void interrupt ISR(void) {
    if (INT1IF){
        unsigned char keypress = (PORTB & 0xF0) >> 4;   //detect key pressed on keypad
        switchMenu('4', '6', keys[keypress]);           //scroll logs with '4' and '6'
        INT1IF = 0;     //clear flag bit
    }
    if (screenMode == OPERATING && TMR0IF){   //timer overflows every second
        TMR0IF = 0;
        TMR0 = 55770;       //timer preset value
        opTimer++;
        min = opTimer / 60; //store run time
        sec = opTimer % 60;
        __lcd_home();
        printf("RUNNING: %02d:%02d   ", min, sec);  
        if (waitPlats)     //turn back platforms after 2 seconds
            waitPlats--;
        if (opTimer >= 180){    //stop operation after 3 minutes
            screenMode = FINISH;
            stopOperation();    
        }        
        if (LATCbits.LC0){          //after solenoid is on for one second, turn it off
            funnelSol(0);
        }
        else {
            solOnTimer++;
            if (solOnTimer >= 2){   //turn on solenoid once every 2 seconds
                solOnTimer = 0;
                funnelSol(1);
            }
        }
        if (!sorting){              //UVD does not detect a battery for WAIT_TIME seconds
            if (ADRES > 50)
                doneTimer++;
            else
                doneTimer = 0;
            if (doneTimer >= WAIT_TIME){
                screenMode = FINISH;
                stopOperation();
            }
        }
        else
            doneTimer = 0;
    }
    if (screenMode == OPERATING && TMR1IF){   //timer overflows every 3 milliseconds
        TMR1IF = 0;
        TMR1 = 58035;
        if (stepGear != STATIONARY){ 
            if (stepGear == DROP_BAT){        //wiggle battery near the UVD hole to make it fall through
                stepAmount++;
                gearStep(!LATCbits.LC2);
                if (stepAmount >= 420){      //189 deg / 0.9 deg per rot = 210 rotations = stepping 420 times
                    stepAmount = 0;
                    stepGear = WIGGLE;
                    gearDir(1);
                }
            }           
            else if (stepGear == WIGGLE){    //allow battery into the rotating gear
                stepAmount++;
                gearStep(!LATCbits.LC2);
                if (stepAmount >= 40){      //18 deg / 0.9 deg per rot = 20 rotations = stepping 40 times 
                    stepAmount = 0;
                    stepGear = FETCH_BAT;
                    gearDir(0);
                }
            }
            else if (stepGear == FETCH_BAT){ //allow time for battery to fall through
                stepAmount ++; 
                gearStep(!LATCbits.LC2);
                if (stepAmount >= 20){      //9 deg / 0.9 deg per rot = 10 rotations = stepping 20 times
                    stepAmount = 0;
                    stepGear = STATIONARY;
                    gearDir(1);
                    gearStep(0);
                }
            }
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
            plat1Left++;
            if (plat1Left>=512){ //512 --> 90 deg
                plat1Left = 0;  
            }
            else if (step1>=4)
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
            plat1Right++;
            if (plat1Right>=512){
                plat1Right = 0;
            }
            else if (step1<=1)
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
            plat2Left++;
            if (plat2Left>=512){
                plat2Left = 0;
            }
            else if (step2>=4)
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
            plat2Right++;
            if (plat2Right>=512){
                plat2Right = 0;
            }
            else if (step2<=1)
                step2 = 4;
            else
                step2--;
        }
    }	
}

