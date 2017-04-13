//Interface Arduino with PIC over I2C Connection
//Outputs keypad char to serial monitor until AAA sequence is given
//Then it will output serial input to LCD display
//Remember to enable the Arduino-PIC switches on RC3 and RC4! 

#include <Wire.h>
void setup() {
  Wire.begin(8);                // join i2c bus with address 8
  Wire.onReceive(receiveEvent); 
  Serial.begin(9600);    
  Serial.println("Time and date of the last sorting: 17/04/11 20:18:05");
  Serial.println("Number of AA batteries sorted: 02");
  Serial.println("Number of C batteries sorted: 07");
  Serial.println("Number of 9V batteries sorted: 00");
  Serial.println("Number of drained batteries sorted: 03");
  Serial.println("Total number of batteries sorted: 12");
  Serial.println("Seconds the sorting lasted for: 156");
}
int state = 0;
char incomingByte;

void loop() {}

void receiveEvent() {
  char x = Wire.read();    // receive byte as char
  Serial.print(x);       // print to serial output
}

