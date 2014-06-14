/* This is used in my NFC doorlock/Door Bell Node, It sleeps for 1/2 a second, sample the IR Prox
 //If an Object is near it wakes the NFC, Reads and transmits the Tag, indicates good/bad, then Sleeps again.
 //It also has a button attached to an inturupt that rings the door bell when pressed.
 *********************Writen By Rod Christian of GCDuino.com*************************
 **                                                                                **
 ** It Use's Arduino's Low Power and Sleep Modes to conserve, monitor battery life **
 **  You can Choose if you'd like it to wake via a Interrupt, a Reset or Watchdog  **
 **                                                                                **
 **   To use the internal Battery Monitor you must power the NODE directly, Not    **
 **     Through the voltage regulator, so keep the voltage between 2.8v * 5v       **
 **                                                                                **
 **          You can also remove the Vreg to reduce current consumption.           **
 **        Try to Power sensors from spare pins, so you can turn them off          **
 **    Remember each pin can only supply 40mA with a 250mA maximum for the NODE    **
 **                                                                                **
 ******************************************************************16/05/2014*******/
//TODO: 
//      Send converted data using an INT and shifting it. eg 25.78 = 2578
//      Write/Read the message number to EEPROM << NOPE get last message number from Base Station on startup!
//      Write example's for different sensors/functions
//      Automatically Get NODE Address from Base Station and write to EEPROM
//      Can we wake from receiving a message on the nRF if it's not asleep?
//      Turn This into A NODE Library!

//Included Libraries
#include <SPI.h>
#include <nRF24L01.h> // Writen by Maniacbug of http://maniacbug.wordpress.com/
#include <RF24.h>
#include <avr/sleep.h>
#include <avr/wdt.h>
#include <Wire.h>  //NFC LIBRARIES
#include <PN532_I2C.h>
#include <PN532.h>
PN532_I2C pn532i2c(Wire);
PN532 nfc(pn532i2c);


//Pin Defines
#define nRF_Rx    2   // nRF Rx
#define nRF_Tx    10  // nRF Tx
#define LED       9   // Onboard LED
#define INT1      3   //Button on INT1 connecting to Ground
#define NFC_Pwr   8   //Low Power Mode for the NFC
#define proxPwr   7   //Supplies power to the Prox Sensor (TCRT5000)
#define proxData  A0  //The Analog Pin for reading the Prox

//Variable
#define nodeAddress 2  //1 is ALWAYS Base Station
//Low Power Sleep Types in Order of Battery Ussage
#define resetableSleep     0  //Run's code, then goes to sleep waiting to be reset (eg, button,leverswitch etc) Will need to get msgNumber
#define interruptableSleep 1  //Relies on an interrupt on INT1 going low (eg. RTC) 
#define watchDogSleep      2  //Relies on the internal watchDog timer to wake the CPU
#define insomniac          3  //All of the Above

byte sleepType = insomniac;  //Set the Sleep Type

//WATCHDOG SLEEP SETTINGS
//#define intervalType 1 // 1 = Minutes, 0 = Seconds
//#define interval 1 // Interval in minutes between waking and doing tasks
//#if intervalType
//const int sleep_total = (interval*60)/8; // Approximate number 
// of sleep cycles needed before the interval defined above 
// elapses. Not that this does integer math ((interval*seconds)/8 Second Watchdog int)
//#else
//const int sleep_total = interval;
//#endif
//volatile int sleep_count = 0; // Keep track of how many sleep cycles have been completed
volatile boolean wokeOnInt = false; //Flag for what woke the CPU

RF24 radio(nRF_Rx, nRF_Tx);
const uint64_t pipes[2] = { 
  0xF0F0F0F0E1LL, 0xF0F0F0F0D2LL }; // Radio pipe addresses for the 2 nodes to communicate.

//ONLY WORKS IF THE BATTERY IS CONNECTED DIRECTLY TO THE CPU. Not using a voltage reg.
const long InternalReferenceVoltage = 1075;  // Adjust this value to your board's specific internal BandGap voltage
//Look in the Low_power Tab for this and Goto www.gcduino.com/*********** to find out more.
// 1093 = Breadboard NODE with 3v3 Reg, 1075 NODE no reg

//nRF Message Structure
typedef struct {
  int msgNumber;
  byte sender;
  byte sensor;
  int data;
  int batLevel;
} 
message;
message msgToSend; 

void setup() 
{       
  radio.begin();
  radio.setRetries(4,15);  // Set the delay between retries & # of retries
  //radio.setPayloadSize(6);  // Small payload size seems to improve reliability
  radio.setDataRate(RF24_2MBPS);  //RF24_250KBPS for 250kbs, RF24_1MBPS for 1Mbps, or RF24_2MBPS for 2Mbps
  radio.openWritingPipe(pipes[1]); //Become the transmitter (ping out)
  radio.stopListening();    

  // initialize the output's. !!ONLY DO THIS WHEN NEEDED
  //pinMode(LED, OUTPUT);   
  //pinMode(proxPwr, OUTPUT); 
  pinMode(NFC_Pwr, OUTPUT);  //Turn ON NFC to Setup
  digitalWrite(NFC_Pwr, HIGH);

  Serial.begin(115200);  //For Debugging
  Serial.println("Begin");  //SPEED NEEDED FOR NFC

  nfc.begin();
  uint32_t versiondata = nfc.getFirmwareVersion();
  if (! versiondata) {
    Serial.print("Didn't find PN53x board");
    while (1); // halt
  }

  // Got ok data, print it out!
  Serial.print("Found chip PN5"); 
  Serial.println((versiondata>>24) & 0xFF, HEX); 
  Serial.print("Firmware ver. "); 
  Serial.print((versiondata>>16) & 0xFF, DEC); 
  Serial.print('.'); 
  Serial.println((versiondata>>8) & 0xFF, DEC);
  nfc.setPassiveActivationRetries(0xFF);
  nfc.SAMConfig();
  digitalWrite(NFC_Pwr, LOW);  //Put NFC to Sleep
  pinMode(NFC_Pwr, INPUT);
  Serial.println("Sleeping...");
  delay(250);
  //Setup message Header
  msgToSend.msgNumber = 1;
  msgToSend.sender = nodeAddress;
}

void loop() //Always Transmits a message then sleeps
{  
  gotoSleep(); 
  //Serial.println(".");
  if(wokeOnInt){ //Woke because of button
    //RING DOOR BELL
    wokeOnInt = false;
    msgToSend.sensor = 1;  //Door Bell Button
    msgToSend.data = 1;    
    //transmit();
    //msgToSend.sensor = 1;
    //msgToSend.data = 1;
    //msgToSend.batLevel = battVoltage();  
    transmit();
    //Serial.println("DING DONG");
    //blinkLed(50);
  } 
  else {
    //CHECK PROX, ETC
    pinMode(proxPwr, OUTPUT);  //Turn ON the prox sensor
    digitalWrite(proxPwr, HIGH);    
    delay(10);
    analogRead(proxData); //discard reading
    delay(10);
    int data = analogRead(proxData);
    digitalWrite(proxPwr, LOW);  
    pinMode(proxPwr, INPUT);  //Turn OFF the prox sensor 
    Serial.println(data, DEC);
    if(data < 250) { 
      //wdt_reset();  
      checkNFC(); //If something is close
    }
  }   
  //delay(500); // Just for TESTING!!!
}

void checkNFC(){
  uint8_t uid[] = { 
    0, 0, 0, 0, 0, 0, 0     };  // Buffer to store the returned UID
  uint8_t uidLength;                        // Length of the UID (4 or 7 bytes depending on ISO14443A card type)

  pinMode(NFC_Pwr, OUTPUT);  //Turn ON NFC to Setup
  digitalWrite(NFC_Pwr, HIGH);
  Wire.beginTransmission(0x48 >> 1);  //Wake NFC
  delay(20);
  Wire.endTransmission();
  nfc.SAMConfig();  //Re-Configure to read Tag's

  boolean success = nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, &uid[0], &uidLength);

  if (success) {
    msgToSend.sensor = 255;
    msgToSend.data = 255;
    //msgToSend.batLevel = battVoltage();  
    transmit();
/*    Serial.println("Found a card!");
    Serial.print("UID Length: ");
    Serial.print(uidLength, DEC);
    Serial.println(" bytes");
    Serial.print("UID Value: ");
    for (uint8_t i=0; i < uidLength; i++) 
    {
      Serial.print(" 0x");
      Serial.print(uid[i], HEX); 
    } */
  }    
  else
  {
    // PN532 probably timed out waiting for a card
    Serial.println("Timed out waiting for a card");
  }
  digitalWrite(NFC_Pwr, LOW);  //Turn NFC into LOW POWER MODE (339uA)
  pinMode(NFC_Pwr, INPUT);  //Turn ON NFC to Setup

}

void getData(){
  //MOCK DATA
  msgToSend.sensor = 3;
  msgToSend.data = 1;
  //msgToSend.batLevel = battVoltage();  
  transmit();
}

void transmit()
{
  msgToSend.batLevel = battVoltage();
  //Serial.println("Sending");
  //bool ok = radio.write( &data, sizeof(data) ); //Writes the data to the radio module
  bool ok = false;
  while(!ok) { 
    ok = radio.write( &msgToSend, sizeof(msgToSend) );
    //if (ok) Serial.println("OK"); //blinkLed(20);
    //else Serial.println("NOPE"); //blinkLed(100); //Turn LED on for an Error*/
    radio.startListening();  //Reset nRF after transmit
    delay(20);
    radio.stopListening(); 
  }
  msgToSend.msgNumber ++; 
  delay(50); //delay for testing and debounce
}



void blinkLed(int time)
{
  pinMode(LED, OUTPUT);
  for(int i = 0; i < 3; i++)  
  {
    digitalWrite(LED, HIGH); //Turn LED off for all OK
    delay(time);
    digitalWrite(LED, LOW); //Turn LED on for an Error
    delay(time);
  } 
  pinMode(LED, INPUT);
} 





