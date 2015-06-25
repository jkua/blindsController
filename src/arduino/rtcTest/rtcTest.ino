#include <SPI.h>
#include "rtc.h"
const int cs = SS; //chip select 
volatile int rtcAlarm = 0;
Rtc rtc;

void setup() {
    Serial.begin(9600);
    
    rtc.init(cs);

    // Read back
    while (!Serial) {}
    
    // year(0-99), month(1-12), day(1-31), hour(0-23), minute(0-59), second(0-59)
    rtc.setTimeDate(11, 12, 13, 0, 14, 15, 16); 
    
    rtc.setAlarm(1, -1, 15, 30);
    rtc.enableAlarm(1);

    byte buffer[27];
    rtc.readRegisters(0, 26, buffer);
    rtc.printRegisters(0, 26, buffer);

    // INT3 (D0) for RTC alarm 0
    attachInterrupt(2, rtcInterrupt, FALLING);
}

void loop() {
    Serial.println(rtc.readTimeDate());
    Serial.println(rtc.readTemp());
    if (rtcAlarm) {
        Serial.println("\n*** RTC alarm triggered!\n");
        rtcAlarm = 0;
    }
    delay(1000);
}

// Interrupt service routine
void rtcInterrupt() {
    rtcAlarm = 1;
    rtc.clearAlarm();
}
