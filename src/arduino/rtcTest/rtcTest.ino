#include <SPI.h>
const int cs = SS; //chip select 

void setup() {
    Serial.begin(9600);
    
    rtcInit();
    // Read back
    while (!Serial) {}
    readRtcConfig();
    
    // year(0-99), month(1-12), day(1-31), hour(0-23), minute(0-59), second(0-59)
    setTimeDate(11, 12, 13, 0, 14, 15, 16); 
    
    setAlarm(1, -1, -1, 30);
    enableAlarm(1);

    readRtcRegisters();
}

void loop() {
    Serial.println(readTimeDate());
    Serial.println(readTemp());
    delay(1000);
}
//=====================================
int rtcInit() { 
    pinMode(cs, OUTPUT); // chip select
    digitalWrite(cs, HIGH);
    // start the SPI library:
    SPI.begin();
    SPI.setBitOrder(MSBFIRST); 
    SPI.setDataMode(SPI_MODE1); // both mode 1 & 3 should work 
    //set control register 
    digitalWrite(cs, LOW);  
    SPI.transfer(0x8E);
    SPI.transfer(0x44); // Enable oscillator, disable square wave, enable interrupt output, but disable alarms
    digitalWrite(cs, HIGH);
    delay(1);
    digitalWrite(cs, LOW);  
    SPI.transfer(0x8F); 
    SPI.transfer(0x00); // Clear the oscillator stop flag, disable the 32 kHz output, set the temp conversion rate to 64 seconds, and clear the alarm flags
    digitalWrite(cs, HIGH);
    delay(1);
    digitalWrite(cs, LOW);  
    SPI.transfer(0x93); 
    SPI.transfer(0x01); // Enable temperature conversion when on backup battery power for better accuracy
    digitalWrite(cs, HIGH);
    delay(10);
}
int readRtcConfig() {
    digitalWrite(cs, LOW); 
    SPI.transfer(0x0E);
    unsigned int readBack = SPI.transfer(0x00);
    digitalWrite(cs, HIGH);
    String temp = printByte(readBack, "RTC Config Readback 0E: ");
    Serial.println(temp);
    
    digitalWrite(cs, LOW);
    SPI.transfer(0x0F);
    readBack = SPI.transfer(0x00);
    digitalWrite(cs, HIGH);
    temp = printByte(readBack, "RTC Config Readback 0F: ");
    Serial.println(temp);
    
    digitalWrite(cs, LOW);
    SPI.transfer(0x13);
    readBack = SPI.transfer(0x00);
    digitalWrite(cs, HIGH);
    temp = printByte(readBack, "RTC Config Readback 13: ");
    Serial.println(temp);
}
int readRtcRegisters() {
    for (byte i = 0; i < 26; i++) {
        digitalWrite(cs, LOW); 
        SPI.transfer(i);
        unsigned int readBack = SPI.transfer(0x00);
        digitalWrite(cs, HIGH);
        String outString;
        if (i < 0x10) outString = String("RTC Register 0x0");
        else outString = String("RTC Register 0x");
        outString.concat(String(i, HEX));
        outString.concat(" ");
        outString = printByte(readBack, outString);
        Serial.println(outString);
    }
}
String printByte(byte input, String prefix) {
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
int setTimeDate(int year, int month, int dayOfMonth, int dayOfWeek, int hour, int minute, int second) { 
    int TimeDate[7] = {second, minute, hour, dayOfWeek, dayOfMonth, month, year};
    
    digitalWrite(cs, LOW);
    SPI.transfer(0x80);  // Multi-byte write starting from address 0x80  
    for (int i = 0; i < 7; i++) {
        int b = TimeDate[i]/10;
        int a = TimeDate[i] - b*10;
        TimeDate[i] = a + (b << 4);
           
        SPI.transfer(TimeDate[i]);        
    }
    digitalWrite(cs, HIGH);
}
//=====================================
String readTimeDate() {
    String temp;
    unsigned int timeDate[7]; //second, minute, hour, day of week, date, month, year
    unsigned int readBuffer[7];

    // Read data
    digitalWrite(cs, LOW);
    SPI.transfer(0x00);  // Multi-byte read starting from address 0  
    for (int i = 0; i < 7; i++) { 
        readBuffer[i] = SPI.transfer(0x00);         
    }
    digitalWrite(cs, HIGH);
        
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

String readTemp() {
    String temp;

    // Read data
    digitalWrite(cs, LOW);
    SPI.transfer(0x11);  // Multi-byte read starting from address 11
    int8_t integer = SPI.transfer(0x00);         
    byte fraction = (SPI.transfer(0x00) >> 6) * 25;
    digitalWrite(cs, HIGH);
    
    temp.concat(integer);
    temp.concat(".");
    temp.concat(fraction);
    temp.concat(" deg C");
    
    return(temp);
}

void setAlarm(uint8_t alarm, int8_t hour, int8_t minute, int8_t second) {
    int8_t TimeDate[4] = {second, minute, hour, -1};
    
    digitalWrite(cs, LOW);
    if (alarm == 1) SPI.transfer(0x87);
    else if (alarm == 2) SPI.transfer(0x8B);
    for (int i = 0; i < 4; i++) {
        if (i == 0 && alarm == 2) continue;
        
        byte output = 0x80;
        // if the value is negative, don't match
        if (TimeDate[i] > 0) {
            byte b = TimeDate[i]/10;
            byte a = TimeDate[i] - b*10;
            output = a + (b << 4);
        }
        
        SPI.transfer(output);         
    }
    digitalWrite(cs, HIGH);
}

void enableAlarm(uint8_t alarm) {
    // Read the existing value
    digitalWrite(cs, LOW);
    SPI.transfer(0x0E);
    byte controlRegister = SPI.transfer(0x00);
    digitalWrite(cs, HIGH);
    
    // Enable the alarm
    if (alarm == 1) controlRegister |= B00000001;
    else if (alarm == 2) controlRegister |= B00000010;
    
    // Write the new value
    digitalWrite(cs, LOW);
    SPI.transfer(0x8E);
    SPI.transfer(controlRegister);
    digitalWrite(cs, HIGH);
}

void disableAlarm(uint8_t alarm) {
    // Read the existing value
    digitalWrite(cs, LOW);
    SPI.transfer(0x0E);
    byte controlRegister = SPI.transfer(0x00);
    digitalWrite(cs, HIGH);
    
    // Disable the alarm
    if (alarm == 1) controlRegister &= B11111110;
    else if (alarm == 2) controlRegister &= B11111101;
    
    // Write the new value
    digitalWrite(cs, LOW);
    SPI.transfer(0x8E);
    SPI.transfer(controlRegister);
    digitalWrite(cs, HIGH);
}
