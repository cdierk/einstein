#include <TinyWireM.h>
//#include <USI_TWI_Master.h>

#define I2C_SLAVE_ADDRESS 0x4 // Address of the slave

 
int i=0;
 
void setup()
{
    TinyWireM.begin(I2C_SLAVE_ADDRESS); // join i2c network
    //TinyWireS.onReceive(receiveEvent); // not using this
    TinyWireM.onRequest(requestEvent);
 
    // Turn on LED when program starts
    pinMode(1, OUTPUT);
    digitalWrite(1, HIGH);
}
 
void loop()
{
    // This needs to be here
    TinyWireS_stop_check();
}
 
// Gets called when the ATtiny receives an i2c request
void requestEvent()
{
    TinyWireM.send(i);
    i++;
}
