/* This is the base Code for the GCDuiNODEs on my Home nRF24L01+ sensor network.
 *********************Writen By Rod Christian of GCDuino.com*************************
 **                                                                                **
 ** It Use's Arduino's Low Power and Sleep Modes to conserve, monitor battery life **
 **  You can Choose if you'd like it to wake via a Interrupt, a Reset or Watchdog  **
 **                                                                                **
 **   To use the internal Battery Monitor you must power the NODE directly, Not    **
 **     Through the voltage regulator, so keep the voltage between 2.8v * 5v       **
 **                                                                                **
 **          You can also remove the Vreg to reduce current consumption            **
 **    It's possible to Power sensors from spare pins, so you can turn them off    **
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

//Pin Defines
#define nRF_Rx    2   // nRF Rx
#define nRF_Tx    10  // nRF Tx
#define LED       9   // Onboard LED
#define INT1      3   //Button on INT1 connecting to Ground

//Variable
#define nodeAddress 255  //1 is ALWAYS Base Station, 255 is Ping Test
//Low Power Sleep Types in Order of Battery Ussage
#define resetableSleep 0      //Run's code, then goes to sleep waiting to be reset (eg, button,leverswitch etc) Will need to get msgNumber
#define interruptableSleep 1  //Relies on an interrupt on INT1 going low (eg. RTC) 
#define watchDogSleep 2       //Relies on the internal watchDog timer to wake the CPU

byte sleepType = watchDogSleep;  //Set the Sleep Type

//WATCHDOG SLEEP SETTINGS
#define interval 1 // Interval in minutes between waking and doing tasks
const int sleep_total = (interval*60)/8; // Approximate number 
// of sleep cycles needed before the interval defined above 
// elapses. Not that this does integer math ((interval*seconds)/8 Second Watchdog int)
volatile int sleep_count = 0; // Keep track of how many sleep cycles have been completed

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

  // initialize the output LED.
  //pinMode(LED, OUTPUT);   

  if(sleepType == interruptableSleep) digitalWrite(INT1, HIGH); //Enable the internal 10k Pull up resistor on the Interrupt Pin
  if(sleepType == watchDogSleep) watchdogOn(); // Turn on the watch dog timer.

  Serial.begin(9600);  //For Debugging
  Serial.println("Begin");  

  //Setup message Header
  msgToSend.msgNumber = 1;
  msgToSend.sender = nodeAddress;
  if(nodeAddress == 255){ //Puts the node into test mode!
    radio.openReadingPipe(1,pipes[1]);  //Listen on Radio Pipe(channel) 1
    
    pingTest();
  }
}

void loop() //Always Transmits a message then sleeps
{
  if(sleepType == watchDogSleep) {
    Serial.println(sleep_count);
    if(sleep_count >= sleep_total) {
      //Counts the amount of times the CPU has awaken from sleep
      // and transmits the data when it's reached the interval time.
      getData();        
      sleep_count = 0;
    }
  } 
  else {
    getData();
  } 
  gotoSleep(); 
}

void pingTest(){
  while(1){  //CONSTANT LOOP
    long time = millis();  //TIMESTAMP
    getData(); //transmit fake data
    radio.startListening();
    bool done = false;
    do {    //Wait for return
      if (radio.available()){
        while (!done)
        {
          // Fetch the payload, and see if this was the last one.
          done = radio.read( &msgToSend, sizeof(msgToSend) );   
          delay(10);      
        }        
      }
    } 
    while (!radio.available() || !done);
    Serial.println(millis()-time); //Print the ping test time
  }
}

void getData(){
  //MOCK DATA
  msgToSend.sensor = 1;
  msgToSend.data = 357;
  msgToSend.batLevel = battVoltage();  
  transmit();
}

void transmit()
{
  //Serial.println("Sending");
  //bool ok = radio.write( &data, sizeof(data) ); //Writes the data to the radio module
  bool ok = false;
  while(!ok) { 
    ok = radio.write( &msgToSend, sizeof(msgToSend) );
    if(nodeAddress == 255){
      if (ok) Serial.print("T"); //blinkLed(20);
      else Serial.print("."); //blinkLed(100); //Turn LED on for an Error*/
    }
    radio.startListening();  //Reset nRF after transmit
    delay(10);
    radio.stopListening(); 
  }
  msgToSend.msgNumber ++; 
  // delay(200); //delay for testing and debounce
}


/*
void blinkLed(int time)
 {
 for(int i = 0; i < 2; i++)  
 {
 digitalWrite(LED, HIGH); //Turn LED off for all OK
 delay(time);
 digitalWrite(LED, LOW); //Turn LED on for an Error
 delay(time);
 } 
 }  */









