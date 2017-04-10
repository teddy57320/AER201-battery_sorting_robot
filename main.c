#include <xc.h>
#include <stdio.h>
#include <stdint.h>
#include "configBits.h"
#include "constants.h"
#include "lcd.h"
#include "I2C.h"

const char keys[] = "123A456B789C*0#D";
const char timeHeader[] = "Time and date of last sorting: ";  //31
const char AAHeader[] = "Number of AA batteries sorted: ";   //31 
const char CHeader[] = "Number of C batteries sorted: ";      //30
const char nineVHeader[] = "Number of 9V batteries sorted: ";    //31
const char drainHeader[] = "Number of drained batteries sorted: ";    //36
const char totalHeader[] = "Total number of batteries sorted: ";  //34
const char runTimeHeader[] = "Seconds the sorting lasted for: ";  //32

void keypressed(unsigned char left, unsigned char right, unsigned char key);
//handles all cases where key is pressed

void readADC(unsigned char channel);
//select analog channel to read

void stopOperation(void);
//stop battery sorting

void testBatteries(void);
//logic circuit for voltage-testing in UVD

void wait_2ms(unsigned int x);   //delay a certain number of seconds

uint8_t Eeprom_ReadByte(uint16_t address);          //EEPROM storage routines
void Eeprom_WriteByte(uint16_t address, uint8_t data);
uint16_t next_address(uint16_t address);
void show_log(uint16_t log_address, unsigned char currScreen);

void logPC(void);
int getHundreds(unsigned int num);
int getTens(unsigned int num);
int getOnes(unsigned int num);
char getChar(unsigned int num);

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
    CVRCON = 0x00; // Disable CCP reference voltage output
    CMCONbits.CIS = 0;
 
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
    //Eeprom_WriteByte(0, 0);
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
        while(screenMode == PERM_LOGA){
            __lcd_home();
            printf("PERMANENT LOG A:");
            __lcd_newline();
            printf("                ");
            show_log(1, PERM_LOGA);
        }
        while(screenMode == PERM_LOGB){
            __lcd_home();
            printf("PERMANENT LOG B:");
            __lcd_newline();
            printf("                ");
            show_log(97, PERM_LOGB);
        }
        while(screenMode == PERM_LOGC){
            __lcd_home();
            printf("PERMANENT LOG C:");
            __lcd_newline();
            printf("                ");
            show_log(193, PERM_LOGC);
        }
        while(screenMode == PERM_LOGD){
            __lcd_home();
            printf("PERMANENT LOG D:");
            __lcd_newline();
            printf("                ");
            show_log(289, PERM_LOGD);
        }
        while (screenMode == PC_LOG){
            __lcd_home();
            printf("PRESS * TO      ");
            __lcd_newline();
            printf("SEND DATA TO PC ");
            unsigned char keypress = (PORTB & 0xF0) >> 4;   //detect key pressed on keypad
            if (keys[keypress] == '*')
                logPC();
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

    unsigned char address_code = Eeprom_ReadByte(0);
    uint16_t address = address_code * 96 + 1;
    Eeprom_WriteByte(address, lastRunRTC[6]);   //year
    address = next_address(address);
    Eeprom_WriteByte(address, lastRunRTC[5]);   //month
    address = next_address(address);
    Eeprom_WriteByte(address, lastRunRTC[4]);   //day
    address = next_address(address);
    Eeprom_WriteByte(address, lastRunRTC[2]);   //hour
    address = next_address(address);
    Eeprom_WriteByte(address, lastRunRTC[1]);   //minute
    address = next_address(address);
    Eeprom_WriteByte(address, lastRunRTC[0]);   //second
    address = next_address(address);

    Eeprom_WriteByte(address, countAA);         //# of AAs
    address = next_address(address);
    Eeprom_WriteByte(address, countC);          //# of Cs
    address = next_address(address);
    Eeprom_WriteByte(address, count9V);         //# of 9Vs
    address = next_address(address);
    Eeprom_WriteByte(address, countDrain);      //# of drained
    address = next_address(address);
    Eeprom_WriteByte(address, numBats);         //total #
    address = next_address(address);
    Eeprom_WriteByte(address, opTimer);         //run time in seconds
           
    address_code++;
    if (address_code > 3)
        Eeprom_WriteByte(0, 0);
    else 
        Eeprom_WriteByte(0, address_code);

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

/******************************************************************************************************/
/* EEPROM storage codes */

uint8_t Eeprom_ReadByte(uint16_t address) {
    // Set address registers
    EEADRH = (uint8_t)(address >> 8);
    EEADR = (uint8_t)address;

    EECON1bits.EEPGD = 0;       // Select EEPROM Data Memory
    EECON1bits.CFGS = 0;        // Access flash/EEPROM NOT config. registers
    EECON1bits.RD = 1;          // Start a read cycle

    // A read should only take one cycle, and then the hardware will clear
    // the RD bit
    while(EECON1bits.RD == 1);

    return EEDATA;              // Return data
}


void Eeprom_WriteByte(uint16_t address, uint8_t data) {    
    // Set address registers
    EEADRH = (uint8_t)(address >> 8);
    EEADR = (uint8_t)address;

    EEDATA = data;          // Write data we want to write to SFR
    EECON1bits.EEPGD = 0;   // Select EEPROM data memory
    EECON1bits.CFGS = 0;    // Access flash/EEPROM NOT config. registers
    EECON1bits.WREN = 1;    // Enable writing of EEPROM (this is disabled again after the write completes)

    // The next three lines of code perform the required operations to
    // initiate a EEPROM write
    EECON2 = 0x55;          // Part of required sequence for write to internal EEPROM
    EECON2 = 0xAA;          // Part of required sequence for write to internal EEPROM
    EECON1bits.WR = 1;      // Part of required sequence for write to internal EEPROM

    // Loop until write operation is complete
    while(PIR2bits.EEIF == 0)
    {
        continue;   // Do nothing, are just waiting
    }

    PIR2bits.EEIF = 0;      //Clearing EEIF bit (this MUST be cleared in software after each write)
    EECON1bits.WREN = 0;    // Disable write (for safety, it is re-enabled next time a EEPROM write is performed)
}


uint16_t next_address(uint16_t address) {
    return address + 8;
}

void show_log(uint16_t log_address, unsigned char currScreen) {
    // read in log address and start fetching historical data
    
    for(unsigned char i=0;i<200;i++){
            if (screenMode != currScreen)  //ensure immediate scrolling
                break;
            __delay_ms(10);
    }
    uint16_t address = log_address;
    unsigned char year = Eeprom_ReadByte(address);      //time retrieval
    address = next_address(address);
    unsigned char month = Eeprom_ReadByte(address);
    address = next_address(address);
    unsigned char day = Eeprom_ReadByte(address);
    address = next_address(address);
    unsigned char hour = Eeprom_ReadByte(address);
    address = next_address(address);
    unsigned char minute = Eeprom_ReadByte(address);
    address = next_address(address);
    unsigned char second = Eeprom_ReadByte(address);
    address = next_address(address);

    unsigned int AA_num = Eeprom_ReadByte(address);
    address = next_address(address);
    unsigned int C_num = Eeprom_ReadByte(address);
    address = next_address(address);
    unsigned int Nine_num = Eeprom_ReadByte(address);
    address = next_address(address);
    unsigned int Drain_num = Eeprom_ReadByte(address);
    address = next_address(address);
    unsigned int total_num = Eeprom_ReadByte(address);
    address = next_address(address);
    unsigned int elapsed_time = Eeprom_ReadByte(address);

    while (screenMode == currScreen){
        __lcd_home();
        printf("%02x/%02x/%02x        ", year,month,day);    //YY/MM/DD
        __lcd_newline();
        printf("%02x:%02x:%02x        ", hour,minute, second);    //HH:MM:SS
        for(unsigned char i=0;i<200;i++){
            if (screenMode != currScreen)  //ensure immediate scrolling
                break;
            __delay_ms(10);
        }
        __lcd_home();
        printf("AA:%02d C:%02d 9V:%02d", AA_num, C_num, Nine_num);
        __lcd_newline();
        printf("X:%02d TIME:%ds    ", Drain_num, elapsed_time);

         
        for(unsigned char i=0;i<200;i++){
            if (screenMode != currScreen)  //ensure immediate scrolling
                break;
            __delay_ms(10);
        }   
    }
}

/******************************************************************************************************/

void logPC(void) {

    for(unsigned int i = 0; i < 31; i++) {
        I2C_Master_Start(); //Start condition
        I2C_Master_Write(0b00010000); //7 bit RTC address + Write
        I2C_Master_Write(timeHeader[i]); //7 bit RTC address + Write
        I2C_Master_Stop();
    }
    char started_time[19] = "  /  /     :  :  ";
    started_time[0] = getChar(getTens( __bcd_to_num(lastRunRTC[6]) ));
    started_time[1] = getChar(getOnes( __bcd_to_num(lastRunRTC[6]) ));
    started_time[3] = getChar(getTens( __bcd_to_num(lastRunRTC[5]) ));
    started_time[4] = getChar(getOnes( __bcd_to_num(lastRunRTC[5]) ));
    started_time[6] = getChar(getTens( __bcd_to_num(lastRunRTC[4]) ));
    started_time[7] = getChar(getOnes( __bcd_to_num(lastRunRTC[4]) ));
    started_time[9] = getChar(getTens( __bcd_to_num(lastRunRTC[2]) ));
    started_time[10] = getChar(getOnes( __bcd_to_num(lastRunRTC[2]) ));
    started_time[12] = getChar(getTens( __bcd_to_num(lastRunRTC[1]) ));
    started_time[13] = getChar(getOnes( __bcd_to_num(lastRunRTC[1]) ));
    started_time[15] = getChar(getTens( __bcd_to_num(lastRunRTC[0]) ));
    started_time[16] = getChar(getOnes( __bcd_to_num(lastRunRTC[0]) ));
    for(unsigned int i = 0; i < 19; i++) {
        I2C_Master_Start(); //Start condition
        I2C_Master_Write(0b00010000); //7 bit RTC address + Write
        I2C_Master_Write(started_time[i]); //7 bit RTC address + Write
        I2C_Master_Stop();
    }

    I2C_Master_Start(); //Start condition
    I2C_Master_Write(0b00010000); //7 bit RTC address + Write
    I2C_Master_Write('\n'); //7 bit RTC address + Write
    I2C_Master_Stop();
    for(unsigned int i = 0; i < 31; i++) {
        I2C_Master_Start(); //Start condition
        I2C_Master_Write(0b00010000); //7 bit RTC address + Write
        I2C_Master_Write(AAHeader[i]); //7 bit RTC address + Write
        I2C_Master_Stop();
    }
    char numberAA[2] = "  ";
    numberAA[0] = getChar(getTens(numAA));
    numberAA[1] = getChar(getOnes(numAA));
    for(unsigned int i = 0; i < 2; i++) {
        I2C_Master_Start(); //Start condition
        I2C_Master_Write(0b00010000); //7 bit RTC address + Write
        I2C_Master_Write(numberAA[i]); //7 bit RTC address + Write
        I2C_Master_Stop();
    }

    I2C_Master_Start(); //Start condition
    I2C_Master_Write(0b00010000); //7 bit RTC address + Write
    I2C_Master_Write('\n'); //7 bit RTC address + Write
    I2C_Master_Stop();
    for(unsigned int i = 0; i < 30; i++) {
        I2C_Master_Start(); //Start condition
        I2C_Master_Write(0b00010000); //7 bit RTC address + Write
        I2C_Master_Write(CHeader[i]); //7 bit RTC address + Write
        I2C_Master_Stop();
    }
    char numberC[2] = "  ";
    numberC[0] = getChar(getTens(numC));
    numberC[1] = getChar(getOnes(numC));
    for(unsigned int i = 0; i < 2; i++) {
        I2C_Master_Start(); //Start condition
        I2C_Master_Write(0b00010000); //7 bit RTC address + Write
        I2C_Master_Write(numberC[i]); //7 bit RTC address + Write
        I2C_Master_Stop();
    }

    I2C_Master_Start(); //Start condition
    I2C_Master_Write(0b00010000); //7 bit RTC address + Write
    I2C_Master_Write('\n'); //7 bit RTC address + Write
    I2C_Master_Stop();
    for(unsigned int i = 0; i < 31; i++) {
        I2C_Master_Start(); //Start condition
        I2C_Master_Write(0b00010000); //7 bit RTC address + Write
        I2C_Master_Write(nineVHeader[i]); //7 bit RTC address + Write
        I2C_Master_Stop();
    }
    char number9V[2] = "  ";
    number9V[0] = getChar(getTens(num9V));
    number9V[1] = getChar(getOnes(num9V));
    for(unsigned int i = 0; i < 2; i++) {
        I2C_Master_Start(); //Start condition
        I2C_Master_Write(0b00010000); //7 bit RTC address + Write
        I2C_Master_Write(number9V[i]); //7 bit RTC address + Write
        I2C_Master_Stop();
    }

    I2C_Master_Start(); //Start condition
    I2C_Master_Write(0b00010000); //7 bit RTC address + Write
    I2C_Master_Write('\n'); //7 bit RTC address + Write
    I2C_Master_Stop();
    for(unsigned int i = 0; i < 36; i++) {
        I2C_Master_Start(); //Start condition
        I2C_Master_Write(0b00010000); //7 bit RTC address + Write
        I2C_Master_Write(drainHeader[i]); //7 bit RTC address + Write
        I2C_Master_Stop();
    }
    char numberDrain[2] = "  ";
    numberDrain[0] = getChar(getTens(numDrain));
    numberDrain[1] = getChar(getOnes(numDrain));
    for(unsigned int i = 0; i < 2; i++) {
        I2C_Master_Start(); //Start condition
        I2C_Master_Write(0b00010000); //7 bit RTC address + Write
        I2C_Master_Write(numberDrain[i]); //7 bit RTC address + Write
        I2C_Master_Stop();
    }

    I2C_Master_Start(); //Start condition
    I2C_Master_Write(0b00010000); //7 bit RTC address + Write
    I2C_Master_Write('\n'); //7 bit RTC address + Write
    I2C_Master_Stop();
    for(unsigned int i = 0; i < 34; i++) {
        I2C_Master_Start(); //Start condition
        I2C_Master_Write(0b00010000); //7 bit RTC address + Write
        I2C_Master_Write(totalHeader[i]); //7 bit RTC address + Write
        I2C_Master_Stop();
    }
    char numberTotal[2] = "  ";
    numberTotal[0] = getChar(getTens(numBats));
    numberTotal[1] = getChar(getOnes(numBats));
    for(unsigned int i = 0; i < 2; i++) {
        I2C_Master_Start(); //Start condition
        I2C_Master_Write(0b00010000); //7 bit RTC address + Write
        I2C_Master_Write(numberTotal[i]); //7 bit RTC address + Write
        I2C_Master_Stop();
    }

    I2C_Master_Start(); //Start condition
    I2C_Master_Write(0b00010000); //7 bit RTC address + Write
    I2C_Master_Write('\n'); //7 bit RTC address + Write
    I2C_Master_Stop();
    for(unsigned int i = 0; i < 32; i++) {
        I2C_Master_Start(); //Start condition
        I2C_Master_Write(0b00010000); //7 bit RTC address + Write
        I2C_Master_Write(runTimeHeader[i]); //7 bit RTC address + Write
        I2C_Master_Stop();
    }
    char runTime[3] = "   ";
    runTime[0] = getChar(getHundreds(min*60+sec));
    runTime[1] = getChar(getTens(min*60+sec));
    runTime[2] = getChar(getOnes(min*60+sec));
    for(unsigned int i = 0; i < 3; i++) {
        I2C_Master_Start(); //Start condition
        I2C_Master_Write(0b00010000); //7 bit RTC address + Write
        I2C_Master_Write(runTime[i]); //7 bit RTC address + Write
        I2C_Master_Stop();
    }
    I2C_Master_Start(); //Start condition
    I2C_Master_Write(0b00010000); //7 bit RTC address + Write
    I2C_Master_Write('\n'); //7 bit RTC address + Write
    I2C_Master_Stop();
    I2C_Master_Start(); //Start condition
    I2C_Master_Write(0b00010000); //7 bit RTC address + Write
    I2C_Master_Write('\n'); //7 bit RTC address + Write
    I2C_Master_Stop();
}

int getHundreds(unsigned int num) {
    if(num > 99) { return (int)(num / 100); }
    return 0;
}

int getTens(unsigned int num) {
    if(num > 9) { return (int)(num / 10); }
    return 0;
}

int getOnes(unsigned int num) {
    return num % 10;
}

char getChar(unsigned int num) {
    return num + '0'; 
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