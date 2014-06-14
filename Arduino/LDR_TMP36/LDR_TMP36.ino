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
//TODO: Test the Power Usage
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
#define tempPin   A7   //TMP36 Data
#define tempPwr   8   //TMP36 Power
#define lightPin  A6   //        A6    LDR-10k voltage divider Data
#define lightPwr  7   //pin7-LDR-^-10k-Gnd

//Variable
#define nodeAddress 3  //1 is ALWAYS Base Station
//Low Power Sleep Types in Order of Battery Ussage
#define resetableSleep 0      //Run's code, then goes to sleep waiting to be reset (eg, button,leverswitch etc) Will need to get msgNumber
#define interruptableSleep 1  //Relies on an interrupt on INT1 going low (eg. RTC) 
#define watchDogSleep 2       //Relies on the internal watchDog timer to wake the CPU

byte sleepType = watchDogSleep;  //Set the Sleep Type

//WATCHDOG SLEEP SETTINGS
#define interval 5 // Interval in minutes between waking and doing tasks
const int sleep_total = (interval*60)/8; // Approximate number 
// of sleep cycles needed before the interval defined above 
// elapses. Not that this does integer math ((interval*seconds)/8 Second Watchdog int)
volatile int sleep_count = 0; // Keep track of how many sleep cycles have been completed

RF24 radio(nRF_Rx, nRF_Tx);
const uint64_t pipes[2] = { 0xF0F0F0F0E1LL, 0xF0F0F0F0D2LL }; // Radio pipe addresses for the 2 nodes to communicate.

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
  int batLevel;  //Do we use this as Sensor 1???? and then report how many messages will be coming
} 
message;
message msgToSend; 

long stopWatch = 0;

void setup() 
{       
  radio.begin();
  radio.setRetries(4,15);  // Set the delay between retries & # of retries
  //radio.setPayloadSize(6);  // Small payload size seems to improve reliability
  radio.setDataRate(RF24_2MBPS);  //RF24_250KBPS for 250kbs, RF24_1MBPS for 1Mbps, or RF24_2MBPS for 2Mbps
  radio.openWritingPipe(pipes[1]); //Become the transmitter (ping out)
  radio.stopListening();    

  // initialize the output LED.
  pinMode(LED, OUTPUT);    
  
  if(sleepType == interruptableSleep) digitalWrite(INT1, HIGH); //Enable the internal 10k Pull up resistor on the Interrupt Pin
  if(sleepType == watchDogSleep) watchdogOn(); // Turn on the watch dog timer.

  //Serial.begin(9600);  //For Debugging
  //Serial.println("Begin");  
  
  //Setup message Header
  msgToSend.msgNumber = 1;
  msgToSend.sender = nodeAddress;
  //Just to test!!!!
    sendData(1, tempPwr, tempPin);
    delay(5);
    sendData(2, lightPwr, lightPin);
}

void loop() //Always Transmits a message then sleeps
{
  
  //Serial.println(millis()); 
  //delay(500);
  if(sleepType == watchDogSleep) {
    //Serial.println(sleep_count);
    if(sleep_count >= sleep_total) {
      //Counts the amount of times the CPU has awaken from sleep
      // and transmits the data when it's reached the interval time.
      sendData(1, tempPwr, tempPin);    
      delay(5);
      sendData(2, lightPwr, lightPin);  
      sleep_count = 0;
    }
  } else {
    sendData(1, tempPwr, tempPin);
    delay(5);
    sendData(2, lightPwr, lightPin);
  } 
  gotoSleep(); 
}

void sendData(byte sensor, byte pwr, byte data){  //Convert to a function (powerPin, SensorPin)
  msgToSend.sensor = sensor;
  pinMode(pwr, OUTPUT); 
  digitalWrite(pwr, HIGH);
  delay(10);
  analogRead(data); //discharge the ADC capacitor
  delay(10);
  msgToSend.data = analogRead(data); 
  digitalWrite(pwr, LOW); 
  pinMode(pwr, INPUT); //Consumes less power as an INPUT
  //Battery Data
  msgToSend.batLevel = battVoltage();
  
  transmit();
}

//Unsure weather to do this in the NODE or in the BaseStation
void calcTemp(){ //Times by 100 to shift decimal and keep it Int Math, remember to shift decimal 1 place back
  //Rolls over change to long then cast to int!!
  int temp = (((analogRead(tempPin) *100 *330)/10240)-500);
  //eg TMP36 reading of 217 would equal 199 or 19.9 degress.
  //int volt = ((analogRead(data) *100 *330)/1024)-5000);  //to keep 2 decimal places eg 1993 or 19.93
}

void transmit()
{
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
  delay(200); //delay for testing and debounce
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






