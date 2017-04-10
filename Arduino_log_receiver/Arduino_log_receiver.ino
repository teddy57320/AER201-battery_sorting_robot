//Interface Arduino with PIC over I2C Connection
//Outputs keypad char to serial monitor until AAA sequence is given
//Then it will output serial input to LCD display
//Remember to enable the Arduino-PIC switches on RC3 and RC4! 

#include <Wire.h>

void setup() {
  Wire.begin(8);                // join i2c bus with address 8
  Wire.onReceive(receiveEvent); 
//  Wire.onRequest(requestEvent); 

  Serial.begin(9600);           
}

int state = 0;
char incomingByte;

void loop() {
//  if (state && Serial.available() > 0 && !incomingByte)
//    incomingByte = Serial.read();
}

//char buf[3];
//int counter=0;

void receiveEvent() {

  char x = Wire.read();    // receive byte as char
  Serial.print(x);       // print to serial output

//  buf[counter++] = x;
//  counter=counter==3?0:counter;
//
//  if(buf[0]=='A' && buf[1]=='A' && buf[2]=='A'){
//    state = 1;
//}
}

//void requestEvent() {
//  Wire.write(incomingByte); // respond with message of 1 byte
//  incomingByte=0;           // clear output buffer
//}
