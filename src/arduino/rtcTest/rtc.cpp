#include "Arduino.h"
#include "rtc.h"

int Rtc::readRegister(byte address) {
    // Ensure this is a read
    address &= B01111111;
    digitalWrite(cs, LOW);
    SPI.transfer(address);
    byte value = SPI.transfer(0x00);
    digitalWrite(cs, HIGH);
    return value;
}
int Rtc::readRegisters(byte start, byte stop, byte* buffer) {
    // Ensure this is a read
    start &= B01111111;
    stop &= B01111111;
    digitalWrite(cs, LOW);
    SPI.transfer(start);
    for (byte i = 0; i <= stop-start; i++) {
        buffer[i] = SPI.transfer(0x00);
    }
    digitalWrite(cs, HIGH);
}
int Rtc::writeRegister(byte address, byte value) {
    // Ensure this is a write
    address |= B10000000;
    digitalWrite(cs, LOW);
    SPI.transfer(address);
    SPI.transfer(value);
    digitalWrite(cs, HIGH);
}
int Rtc::writeRegisters(byte start, byte stop, byte* buffer) {
    // Ensure this is a write
    start |= B10000000;
    stop |= B10000000;

    digitalWrite(cs, LOW);
    SPI.transfer(start);
    for (byte i = 0; i <= stop-start; i++) {
        SPI.transfer(buffer[i]);
    }
    digitalWrite(cs, HIGH);
}
int Rtc::printRegisters(byte start, byte stop, byte* buffer) {
    for (byte i = start; i <= stop; i++) {
        String outString;
        if (i < 0x10) outString = String("RTC Register 0x0");
        else outString = String("RTC Register 0x");
        outString.concat(String(i, HEX));
        outString.concat(" ");
        outString = printByte(buffer[i], outString);
        Serial.println(outString);
    }
}

String Rtc::printByte(byte input, String prefix) {
    prefix.concat("Binary: ");
    for (int i = 7; i >= 0; i--) {
        prefix.concat(input >> i & 1);
        if (i == 4) prefix.concat("  ");
        else if (i > 0) prefix.concat(" ");
    }
    if (input < 0x10) prefix.concat(", Hex: 0x0");
    else prefix.concat(", Hex: 0x");
    prefix.concat(String(input, HEX));
    prefix.concat(", Decimal: ");
    prefix.concat(input);
    return prefix;
}
//=====================================
// 24-hour mode
int Rtc::setTimeDate(int year, int month, int dayOfMonth, int dayOfWeek, int hour, int minute, int second) { 
    int TimeDate[7] = {second, minute, hour, dayOfWeek, dayOfMonth, month, year};
    byte output[7];
    for (int i = 0; i < 7; i++) {
        int b = TimeDate[i]/10;
        int a = TimeDate[i] - b*10;
        output[i] = a + (b << 4);   
    }

    writeRegisters(0x80, 0x86, output);
}
//=====================================
String Rtc::readTimeDate() {
    String temp;
    unsigned int timeDate[7]; //second, minute, hour, day of week, date, month, year
    byte readBuffer[7];

    // Read data
    readRegisters(0x00, 0x06, readBuffer);
        
    // Parse data
    for (int i = 0; i < 7; i++) {
        unsigned int curValue = readBuffer[i];
        unsigned int ones = curValue & B00001111;
        unsigned int tens = 0;
        unsigned int amPmFlag = 0;
        unsigned int mode;
        
        switch (i) {
            case 0: // seconds
            case 1: // minutes
                tens = (curValue & B01110000) >> 4;
                break;
            case 2: // hours
                mode = curValue & B01000000;
                if (mode) {  // am/pm
                    amPmFlag = curValue & B00100000;
                    tens = (curValue & B00010000) >> 4;
                }
                else {  // 24-hour
                    tens = (curValue & B00110000) >> 4;
                }
            case 3: // day of week
                break;
            case 4: // day of month
                tens = (curValue & B00110000) >> 4;
                break;
            case 5: // month
                tens = (curValue & B00010000) >> 4;
                break;
            case 6: // year
                tens = (curValue & B11110000) >> 4;
                break;
        }
        
        timeDate[i] = ones + tens*10;
        if (amPmFlag) timeDate[i] += 12;
    }

    // Format time string
    switch (timeDate[3]) {
        case 0:
            temp.concat("Sun ");
            break;
        case 1:
            temp.concat("Mon ");
            break;
        case 2:
            temp.concat("Tue ");
            break;
        case 3:
            temp.concat("Wed ");
            break;
        case 4:
            temp.concat("Thu ");
            break;
        case 5:
            temp.concat("Fri ");
            break;
        case 6:
            temp.concat("Sat ");
            break;
    } 
    temp.concat(timeDate[6] + 2000);
    temp.concat("-");
    temp.concat(timeDate[5]);
    temp.concat("-");
    temp.concat(timeDate[4]);
    temp.concat(" ");
    temp.concat(timeDate[2]);
    temp.concat(":");
    temp.concat(timeDate[1]);
    temp.concat(":");
    temp.concat(timeDate[0]);

    return(temp);
}

String Rtc::readTemp() {
    String temp;
    byte readBuffer[2];

    // Read data
    readRegisters(0x11, 0x12, readBuffer);
    int8_t integer = readBuffer[0];         
    byte fraction = (readBuffer[1] >> 6) * 25;
    
    temp.concat(integer);
    temp.concat(".");
    temp.concat(fraction);
    temp.concat(" deg C");
    
    return(temp);
}

void Rtc::setAlarm(uint8_t alarm, int8_t hour, int8_t minute, int8_t second) {
    int8_t TimeDate[4] = {second, minute, hour, -1};
    byte writeBuffer[4];
    for (int i = 0; i < 4; i++) {
        if (i == 0 && alarm == 2) continue;
        
        writeBuffer[i] = 0x80;
        // if the value is negative, don't match
        if (TimeDate[i] > 0) {
            byte b = TimeDate[i]/10;
            byte a = TimeDate[i] - b*10;
            writeBuffer[i] = a + (b << 4);
        }        
    }

    if (alarm == 1) writeRegisters(0x87, 0x8A, writeBuffer);
    else if (alarm == 2) writeRegisters(0x8B, 0x8D, writeBuffer+1);
}

void Rtc::clearAlarm() {
    // Read the existing value
    byte controlRegister = readRegister(0x0F);
  
    // Clear both alarms
    controlRegister &= B11111100;
    
    // Write the new value
    writeRegister(0x8F, controlRegister);
}

void Rtc::enableAlarm(uint8_t alarm) {
    // Read the existing value
    byte controlRegister = readRegister(0x0E);
  
    // Enable the alarm
    if (alarm == 1) controlRegister |= B00000001;
    else if (alarm == 2) controlRegister |= B00000010;
    
    // Write the new value
    writeRegister(0x8E, controlRegister);
}

void Rtc::disableAlarm(uint8_t alarm) {
    // Read the existing value
    byte controlRegister = readRegister(0x0E);
    
    // Disable the alarm
    if (alarm == 1) controlRegister &= B11111110;
    else if (alarm == 2) controlRegister &= B11111101;
    
    // Write the new value
    writeRegister(0x8E, controlRegister);
}