/*  ********************************************* 
 *   Christine Dierk and Bala Thoravi Kumaravel
 *   
 *   November 6, 2017
 *   
 *   ADXL + ATTiny85 + Eink Display
 *  *********************************************/

#define CLEAR_PIN       3
#define TOP_DOT         4
#define BOTTOM_DOT      1
#define REFRESH_DELAY   1000

#include <TinyADXL345.h>         // SparkFun ADXL345 Library
#include <EEPROM.h>              // EEPROM memory library
#include  "Wire.h"               // Wire library for ATTiny

//EEPROM variables
int currentState = 0;            //will be replaced by call to EEPROM (if value is available)
int nailPointer = 0;            //will be replaced by call to EEPROM (if value is available)
byte data;                       //used when writing to EEPROM

//i2c variables (to NFC chip)
const byte NTAG_ADDR = 0x55;
const byte int_reg_addr = 0x37;   //second to last block of memory 
//
//boolean slavePresent = false;
//
//  //must read or write in chunks of 16 blocks
//  byte a;
//  byte b;
//  byte c;
//  byte d;
//  byte e;
//  byte f;
//  byte g;
//  byte h;
//  byte i;
//  byte pointer = 0x32;
//  byte numData = 0x35;
//  byte mostRecent = 0x34;
//  byte m;
//  byte n;
//  byte o;
//  byte p;
//
//  unsigned long timeNow;

/*********** COMMUNICATION SELECTION ***********/
/*    Comment Out The One You Are Not Using    */
//ADXL345 adxl = ADXL345(10);           // USE FOR SPI COMMUNICATION, ADXL345(CS_PIN);
ADXL345 adxl = ADXL345();             // USE FOR I2C COMMUNICATION

/****************** INTERRUPT ******************/
/*      Uncomment If Attaching Interrupt       */
//int interruptPin = 2;                 // Setup pin 2 to be the interrupt pin (for most Arduino Boards)


/******************** SETUP ********************/
/*          Configure ADXL345 Settings         */
void setup(){

  //i2c to NFC tag
  Wire.begin();

  //timeNow = millis();
  
  pinMode(CLEAR_PIN, OUTPUT);
  pinMode(TOP_DOT, OUTPUT);
  pinMode(BOTTOM_DOT, OUTPUT);

  currentState = EEPROM.read(0);
  nailPointer = EEPROM.read(1);

  adxl.powerOn();                     // Power on the ADXL345

  adxl.setRangeSetting(16);           // Give the range settings
                                      // Accepted values are 2g, 4g, 8g or 16g
                                      // Higher Values = Wider Measurement Range
                                      // Lower Values = Greater Sensitivity

  //adxl.setSpiBit(0);                  // Configure the device to be in 4 wire SPI mode when set to '0' or 3 wire SPI mode when set to 1
                                      // Default: Set to 1
                                      // SPI pins on the ATMega328: 11, 12 and 13 as reference in SPI Library 
   
  adxl.setActivityXYZ(1, 0, 0);       // Set to activate movement detection in the axes "adxl.setActivityXYZ(X, Y, Z);" (1 == ON, 0 == OFF)
  adxl.setActivityThreshold(75);      // 62.5mg per increment   // Set activity   // Inactivity thresholds (0-255)
 
  adxl.setInactivityXYZ(1, 0, 0);     // Set to detect inactivity in all the axes "adxl.setInactivityXYZ(X, Y, Z);" (1 == ON, 0 == OFF)
  adxl.setInactivityThreshold(75);    // 62.5mg per increment   // Set inactivity // Inactivity thresholds (0-255)
  adxl.setTimeInactivity(10);         // How many seconds of no activity is inactive?

  adxl.setTapDetectionOnXYZ(0, 0, 1); // Detect taps in the directions turned ON "adxl.setTapDetectionOnX(X, Y, Z);" (1 == ON, 0 == OFF)
 
  // Set values for what is considered a TAP and what is a DOUBLE TAP (0-255)
  adxl.setTapThreshold(175);           // 62.5 mg per increment
  adxl.setTapDuration(15);            // 625 μs per increment
  adxl.setDoubleTapLatency(90);       // 1.25 ms per increment
  adxl.setDoubleTapWindow(200);       // 1.25 ms per increment
 
  // Set values for what is considered FREE FALL (0-255)
  adxl.setFreeFallThreshold(7);       // (5 - 9) recommended - 62.5mg per increment
  adxl.setFreeFallDuration(30);       // (20 - 70) recommended - 5ms per increment
 
  // Setting all interupts to take place on INT1 pin
  //adxl.setImportantInterruptMapping(1, 1, 1, 1, 1);     // Sets "adxl.setEveryInterruptMapping(single tap, double tap, free fall, activity, inactivity);" 
                                                        // Accepts only 1 or 2 values for pins INT1 and INT2. This chooses the pin on the ADXL345 to use for Interrupts.
                                                        // This library may have a problem using INT2 pin. Default to INT1 pin.
  
  // Turn on Interrupts for each mode (1 == ON, 0 == OFF)
  adxl.InactivityINT(1);
  adxl.ActivityINT(1);
  adxl.FreeFallINT(1);
  adxl.doubleTapINT(1);
  adxl.singleTapINT(1);
  
//attachInterrupt(digitalPinToInterrupt(interruptPin), ADXL_ISR, RISING);   // Attach Interrupt

}

/****************** MAIN CODE ******************/
/*     Accelerometer Readings and Interrupt    */
void loop(){
  
  ADXL_ISR();
  // You may also choose to avoid using interrupts and simply run the functions within ADXL_ISR(); 
  //  and place it within the loop instead.  
  // This may come in handy when it doesn't matter when the action occurs. 
//
//  //read current metadata values from NTAG over i2c
//  //only check every 500 ms
//  if (millis() - timeNow >= 500) {
//      if (!slavePresent) {                                      // determine if slave joined the I2C bus
//          Wire.beginTransmission(NTAG_ADDR);                    // begin call to slave
//          Wire.send(0x01);
//          if (Wire.endTransmission() == 0) slavePresent = true;  // if responds - mark slave as present
//      } else {       // slave found on I2C bus
//          Wire.requestFrom(NTAG_ADDR,16);                        //request 16 bytes from slave
//          a = Wire.receive();                               //received byte from slave
//          b = Wire.receive();                               //received byte from slave
//          c = Wire.receive();                               //received byte from slave
//          d = Wire.receive();                               //received byte from slave
//          e = Wire.receive();                               //received byte from slave
//          f = Wire.receive();                               //received byte from slave
//          g = Wire.receive();                               //received byte from slave
//          h = Wire.receive();                               //received byte from slave
//          i = Wire.receive();                               //received byte from slave
//          pointer = Wire.receive();                               //received byte from slave
//          numData = Wire.receive();                               //received byte from slave
//          mostRecent = Wire.receive();                               //received byte from slave
//          m = Wire.receive();                               //received byte from slave
//          n = Wire.receive();                               //received byte from slave
//          o = Wire.receive();                               //received byte from slave
//          p = Wire.receive();                               //received byte from slave
//    }
//    timeNow = millis();
//  }
}

/********************* ISR *********************/
/* Look for Interrupts and Triggered Action    */
void ADXL_ISR() {
  
  // getInterruptSource clears all triggered actions after returning value
  // Do not call again until you need to recheck for triggered actions
  byte interrupts = adxl.getInterruptSource();
  
  // Free Fall Detection
  if(adxl.triggered(interrupts, ADXL345_FREE_FALL)){
    //Serial.println("*** FREE FALL ***");
    //add code here to do when free fall is sensed
  } 
  
  // Inactivity
  if(adxl.triggered(interrupts, ADXL345_INACTIVITY)){
    //Serial.println("*** INACTIVITY ***");
     //add code here to do when inactivity is sensed
  }
  
  // Activity
  if(adxl.triggered(interrupts, ADXL345_ACTIVITY)){
    //Serial.println("*** ACTIVITY ***"); 
     //add code here to do when activity is sensed
  }
  
  // Double Tap Detection
  if(adxl.triggered(interrupts, ADXL345_DOUBLE_TAP)){
    //Serial.println("*** DOUBLE TAP ***");
     //add code here to do when a 2X tap is sensed
     //blink1();
  }
  
  // Tap Detection
  else if(adxl.triggered(interrupts, ADXL345_SINGLE_TAP)){
    //Serial.println("*** TAP ***");
     //add code here to do when a tap is sensed
     count();
  } 
}

void clearDisplay()
{
  digitalWrite(CLEAR_PIN, HIGH);
  delay(REFRESH_DELAY);
  digitalWrite(CLEAR_PIN, LOW);
}

void count(){
  currentState++;
  nailPointer++;

  if(currentState > 3){
    currentState = 0;
  }
  if(nailPointer >= 20){
    nailPointer = 0;
  }
  
  if (currentState == 0){
    digitalWrite(TOP_DOT, LOW);
    digitalWrite(BOTTOM_DOT, LOW);
    currentState = 0;
  } else if (currentState == 1){
    digitalWrite(TOP_DOT, LOW);
    digitalWrite(BOTTOM_DOT, HIGH);
  } else if (currentState == 2){
    digitalWrite(TOP_DOT, HIGH);
    digitalWrite(BOTTOM_DOT, LOW);
  } else if (currentState == 3){
    digitalWrite(TOP_DOT, HIGH);
    digitalWrite(BOTTOM_DOT, HIGH);
  }

  //write values to EEPROM
  data = (0xff & currentState);
  EEPROM.write(0, (byte) data);     //save current state

  data = (0xff & nailPointer);
  EEPROM.write(1, (byte) data);     //save nail pointer

  //write value to NTAG over i2c
  //rewrite values
  int writeableState = currentState + 48;

  //clear display to update the dots
  clearDisplay();

  // write to NTAG over i2c
  Wire.beginTransmission(NTAG_ADDR); 
  Wire.send(int_reg_addr);                  //memory block to write to
//  Wire.send(0x03);           
//  Wire.send(0x0A); 
//  Wire.send(0xD1);           
//  Wire.send(0x01);     
//  Wire.send(0x06);           
//  Wire.send(0x54);     
//  Wire.send(0x02);           
//  Wire.send(0x65);
//  Wire.send(0x6E);     
  Wire.send(nailPointer);   
  Wire.send(0x00);   
  Wire.send(0x00);   
  Wire.send(0x00);   
  Wire.send(0x00);   
  Wire.send(0x00);   
  Wire.send(0x00);   
  Wire.send(0x00);   
  Wire.send(0x00);     
  Wire.send(0x00); 
  Wire.send(0x00);           
  Wire.send(0x00);     
  Wire.send(0x00);           
  Wire.send(0x00);     
  Wire.send(0x00);           
  Wire.send(0x00);                                    
  Wire.endTransmission();  

  delay(REFRESH_DELAY);
}


