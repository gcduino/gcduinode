/*  This is the Hello World of GCDuiNODEs Communication, it's also great for range finding.
 *  Load this sketch onto 2 NODE's, changing the MODE on each of them.
 *  the LED will flash slow on both if they are sending/receiving messages, fast if they aren't.
 
 * Or... 
 * Leave one plugged into your PC with Serial monitor open, plug a bluetooth module & battery pack into the other.
 * Connect to it using the free android app Bluetooth spp pro (default pin of 1234) and enter it's byte steam mode.
 * You should see the char your transmitting come up repeatedly, when all you see is a string of "." your out of range.  */

boolean mode = 1; //0 = Transmit, 1 = Receive

#include <SPI.h>
#include "nRF24L01.h"  
#include "RF24.h"
// The above libraries were writen by Maniacbug of http://maniacbug.wordpress.com/

// Set up nRF24L01 radio on SPI bus plus pins 2 & 10 
RF24 radio(2, 10);

// Radio pipe addresses for the 2 nodes to communicate.
const uint64_t pipes[2] = { 0xF0F0F0F0E1LL, 0xF0F0F0F0E2LL };   

char recvChar;

#define LED 9

void setup()
{
  radio.begin();
  radio.setRetries(4,15);  // Set the delay between retries(per 250uS) & # of retries
  //radio.setPayloadSize(2);  // A smaller payload size improves reliability, set it for what you need
  radio.setDataRate(RF24_2MBPS);  // Set the Speed RF24_250KBPS for 250kbs, RF24_1MBPS for 1Mbps, or RF24_2MBPS for 2Mbps
  radio.openReadingPipe(1,pipes[1]);  //Listen on Radio Pipe(channel) 1
  radio.startListening(); 

  pinMode(LED, OUTPUT);
  digitalWrite(LED, HIGH);  //Blink to say we are alive
  delay(25);
  digitalWrite(LED, LOW);

  Serial.begin(9600); //For Debugging & bluetooth 
  if(mode) Serial.println("Receiving!");
  else Serial.println("Transmitting!");
}


void loop()
{
  if(mode) chkRadio(); 
  else transmit('N'); //Transmits the char N
}

void transmit(char data)  //WHICH MODULE TO TRANSMIT TO, DATA TO SEND
{
  radio.openWritingPipe(pipes[1]);
  radio.stopListening();
  bool ok = false;
  do{
    ok = radio.write(&data, sizeof(data)); //Writes the data to the radio module
    if(!ok){
      Serial.print(".");      
      digitalWrite(LED, HIGH);
      delay(20);       
      digitalWrite(LED, LOW);
      delay(20);
    }
    radio.startListening();  //Reset nRF before next transmit
    delay(20);
    radio.stopListening();
  } 
  while(!ok);
  Serial.println(data);
  digitalWrite(LED, HIGH);  //Long blink to say we sent a message
  delay(250);
  digitalWrite(LED, LOW); 
  delay(250);  
}

void chkRadio()
{
  if (radio.available())
  {
    bool done = false;    
    while (!done)
    {
      // Fetch the payload, and see if this was the last one.
      done = radio.read( &recvChar, sizeof(recvChar) );   
      delay(20);      
    }
    Serial.println(recvChar);
    radio.stopListening();
    radio.startListening();//RESET
    digitalWrite(LED, HIGH);  //Long blink to say we got a message
    delay(250);
    digitalWrite(LED, LOW);
    delay(250);
  }  
  else
  {
    digitalWrite(LED, HIGH);  //Short blink to say NO message
    Serial.print(".");
    delay(20);
    digitalWrite(LED, LOW);
    delay(20);
  } 
}


