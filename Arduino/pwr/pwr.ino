#define CLOCK_ADDRESS 0x68

#include <Wire.h>
#include <RTClib.h>
#include <SD.h>
#include <avr/sleep.h>

#define SD_CS      8   // SD Card ChipSelect
#define Sensor     3   // Distance Sensor
#define LED        9  // Onboard LED
#define doorState  6  //Hall Effect
#define doorPwr    7  //Hall Effect Power

RTC_DS1307 RTC; //Real Time Clock
File logFile;
int lastReading = 2000; //Stop you getting a fake first reading
boolean leadingEdge = true;
long duration;
long pulseCount = 0;     // Number of pulses
double pulsePerkWh = 0.00375; // Number of pulses per kWh
// found on the meter 
// (e.g 266.6 rev/kWh)
//double kW, kWh;


void setup() {
  //Serial.begin(115200);
  pinMode(LED, OUTPUT);
  digitalWrite(doorState, HIGH);  //Enable pull up resistor
  //Initializes the RTC
  Wire.begin();
  RTC.begin();
  // Check if the RTC is running.
  if (! RTC.isrunning()) {
    digitalWrite(LED, HIGH);
    // Serial.println("RTC is NOT running");
  }
  // This section grabs the current datetime and compares it to
  // the compilation time.  If necessary, the RTC is updated.
  DateTime now = RTC.now();
  DateTime compiled = DateTime(__DATE__, __TIME__);
  if (now.unixtime() < compiled.unixtime()) {
    //Serial.println("RTC is older than compile time! Updating");
    RTC.adjust(DateTime(__DATE__, __TIME__));
  }

  //Initializes the SD Card.
  if (!SD.begin(SD_CS)) {
    digitalWrite(LED, HIGH);
    //Serial.println("Card failed, or not present");
    delay(200);
    // don't do anything more:
    return;
  }
 /* logFile = SD.open("powerlog.csv", FILE_WRITE);
  if (logFile) {
    logFile.print("TIME");
    logFile.print(',');  
    logFile.print("READING");
    logFile.print(',');    
    //logFile.print("DURATION");         
    //logFile.print(',');
    logFile.println("DOOR STATE");
    //logFile.print("POWER (kW)");
    //logFile.print(',');
    //logFile.println("ENERGY (kWh)");
    logFile.close();
  }  */
  //Serial.println("SD Card initialized.");
  //delay(200);
  pinMode(doorPwr, OUTPUT);
  digitalWrite(doorPwr, HIGH);
  watchdogOn();
}


void loop() {
  //cycle++;
  int reading = 0;
  analogRead(Sensor);
  delay(10);
  //Serial.println(analogRead(Sensor));
  for(int i = 0; i < 4; i++){
    reading+= analogRead(Sensor);
    delay(5);
  }
  reading /= 4;
  //Serial.println(reading);
/*  if(reading > 800 && leadingEdge) {
    leadingEdge = false; */
    DateTime now = RTC.now();  
    // open the file. note that only one file can be open at a time,
    // so you have to close this one before opening another.
    logFile = SD.open("powerlog.csv", FILE_WRITE);
    if (logFile) {
      logFile.print(now.hour(), DEC);
      logFile.print(':');
      logFile.print(now.minute(), DEC);
      logFile.print(':');
      logFile.print(now.second(), DEC);
      logFile.print(',');  
      logFile.print(reading);
      logFile.print(',');    
      //logFile.print(millis() - duration);
      //duration = millis();         
      //logFile.print(',');
      logFile.println(digitalRead(doorState));
      //logFile.print(pulsePerkWh / (duration / 3600.0));
     // logFile.print(',');
     // logFile.println(pulseCount * pulsePerkWh);
      logFile.close();
    }    
  //  digitalWrite(LED, HIGH);
  //  delay(20);
  //  digitalWrite(LED, LOW);
  //  lastReading = reading;    
    //cycle = 0;
   // delay(80);
 // } else if(reading < 500 && !leadingEdge) { //Wait's until the the reading is off the black
 //   leadingEdge = true;  //Set it to look for the Leading edge.
 // } */
 
 if(digitalRead(doorState)){
    digitalWrite(LED, HIGH);
    delay(20);
    digitalWrite(LED, LOW);
 }
  //delay(500);
  gotoSleep();
}

/*
void pulse(){
  pulseCount++;
  kW = pulsePerkWh / (duration / 3600.0);
  kWh = pulseCount * pulsePerkWh;
} */

void watchdogOn() {
  // Clear the reset flag, the WDRF bit (bit 3) of MCUSR.
  MCUSR = MCUSR & B11110111;
  // Set the WDCE bit (bit 4) and the WDE bit (bit 3) 
  // of WDTCSR. The WDCE bit must be set in order to 
  // change WDE or the watchdog prescalers. Setting the 
  // WDCE bit will allow updtaes to the prescalers and 
  // WDE for 4 clock cycles then it will be reset by 
  // hardware.
  WDTCSR = WDTCSR | B00011000; 
  // Set the watchdog timeout prescaler value to 1024 K 
  // which will yeild a time-out interval of about 8.0 s.
  WDTCSR = B00000101;
  //B00(1)00(001) == Change Braketed to: 
  //0000  0001  0010  0011  0100  0101  0110  0111  1000  1001
  // 16    32    64    125   250   500  1000  2000  4000  8000 (mS)
  // Enable the watchdog timer interupt.
  WDTCSR = WDTCSR | B01000000;
  MCUSR = MCUSR & B11110111;
}


void gotoSleep(){
  //radio.powerDown();  //Put the Radio in Low Power Mode, Writing data will wake it.
  //store ADC settings
  byte old_ADCSRA = ADCSRA;
  // disable ADC
  ADCSRA = 0;  

  set_sleep_mode (SLEEP_MODE_PWR_DOWN);  
  sleep_enable();

  // turn off brown-out enable in software
  // BODS must be set to one and BODSE must be set to zero within four clock cycles
  MCUCR = bit (BODS) | bit (BODSE);
  // The BODS bit is automatically cleared after three clock cycles
  MCUCR = bit (BODS); 

  // We are guaranteed that the sleep_cpu call will be done
  // as the processor executes the next instruction after
  // interrupts are turned on.
  sleep_cpu ();   // one cycle
  
  //********************Ultra Lower Power MODE now ON*********************
  //*********CPU will wake when HERE the interrupt pin goes LOW***********
  sleep_disable();  // Disable sleep mode after waking.
  //Restore ADC settings to ON
  ADCSRA = old_ADCSRA;  
}

