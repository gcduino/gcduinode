/*   POWER USAGE
1.445mA = Asleep with hall effect enabled. TOO HIGH!!! Changed to wake every 500mS
14/6/14
0.0036mA, 500mS duration = Asleep
4.64mA, >1mS = Awake and testing.
12.68mA, 23mS duration = Transmit condition meet
Transmitting 6 times a day (3 deliveries, 3 checks) on a PERFECT 750mAh battery
equals 226 days operation time, now if only we had perfect batteries.
*/
//Included Libraries
#include <SPI.h>
#include <nRF24L01.h> // Writen by Maniacbug of http://maniacbug.wordpress.com/
#include <RF24.h>
#include <avr/sleep.h>
#include <avr/wdt.h>

//Pin Defines
#define nRF_Rx    2   // nRF Rx
#define nRF_Tx    10  // nRF Tx
#define LED       9   // Onboard LED
#define INT1      3   //Button on INT1 connecting to Ground
#define mailIn    8
#define mailOut   7

//Variable
#define nodeAddress 3  //1 is ALWAYS Base Station
boolean gotMail = false;

RF24 radio(nRF_Rx, nRF_Tx);
const uint64_t pipes[2] = { 0xF0F0F0F0E1LL, 0xF0F0F0F0D2LL }; // Radio pipe addresses for the 2 nodes to communicate.

//ONLY WORKS IF THE BATTERY IS CONNECTED DIRECTLY TO THE CPU. Not using a voltage reg.
const long InternalReferenceVoltage = 1093;  // Adjust this value to your board's specific internal BandGap voltage
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

  digitalWrite(INT1, HIGH); //Enable the internal 20k Pull up resistor on the Interrupt Pin

  //Serial.begin(115200);  //For Debugging
  //Serial.println("Begin");  
  //delay(500); //to finish serial
  
  //Setup message Header
  msgToSend.msgNumber = 1;
  msgToSend.sender = nodeAddress;
  msgToSend.sensor = 3;
}

void loop()
{
  gotoSleep();  //Sleeps for 500mS
  if(gotMail){  //If we have mail
    pinMode(mailOut, OUTPUT);
    digitalWrite(mailOut, HIGH); //Power on the back door Halleffect
    if(!digitalRead(INT1)) {  //If it sense's the magnet TRANSMIT
      //*****************Need to reverse as it should always sense the magnet
      gotMail = false;
      msgToSend.data = gotMail;
      transmit();
    }
    digitalWrite(mailOut, LOW);
    pinMode(mailOut, INPUT);
  } else {
    pinMode(mailIn, OUTPUT);
    digitalWrite(mailIn, HIGH);
    if(!digitalRead(INT1)) {
      gotMail = true;
      msgToSend.data = gotMail;
      transmit();     
    }
    digitalWrite(mailIn, LOW);
    pinMode(mailIn, INPUT);
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
  msgToSend.batLevel = battVoltage();
  bool ok = false;
  while(!ok) { 
  ok = radio.write( &msgToSend, sizeof(msgToSend) );
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






