#ifndef RTC_H
#define RTC_H
#include "Arduino.h"
#include <SPI.h>

class Rtc {
public:
    void init(int cs) {
        this->cs = cs;
        pinMode(cs, OUTPUT); // chip select
        digitalWrite(cs, HIGH);
        // start the SPI library:
        SPI.begin();
        SPI.setBitOrder(MSBFIRST); 
        SPI.setDataMode(SPI_MODE1); // both mode 1 & 3 should work 
        //set control register 

        byte buffer[2];
        buffer[0] = 0x44;
        buffer[1] = 0x00;
        writeRegisters(0x8E, 0x8F, buffer);
        writeRegister(0x93, 0x1);
    }
    int readRegister(byte address);
    int readRegisters(byte start, byte stop, byte* buffer);
    int writeRegister(byte address, byte value);
    int writeRegisters(byte start, byte stop, byte* buffer);
    int printRegisters(byte start, byte stop, byte* buffer);

    String readTimeDate();
    int setTimeDate(int year, int month, int dayOfMonth, int dayOfWeek, int hour, int minute, int second);
    
    String readTemp();

    void setAlarm(uint8_t alarm, int8_t hour, int8_t minute, int8_t second);
    void clearAlarm();
    void enableAlarm(uint8_t alarm);
    void disableAlarm(uint8_t alarm);

private:
    String printByte(byte input, String prefix);
    int cs;
};

#endif