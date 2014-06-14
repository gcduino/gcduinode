//MEASURE THE BATTERY VOLTAGE COMAPARED TO THE 1.1v INTERNAL REF.
//Information from Nick Gammon, http://www.gammon.com.au/forum/?id=11497

int battVoltage (){ //this is refered to as Bandgap!

  //ONLY WORKS IF THE BATTERY IS CONNECTED DIRECTLY TO THE CPU.
  //Not through a voltage regulator!
  analogRead(6); //Read an Unused Analog Pin
  // REFS0 : Selects AVcc external reference
  // MUX3 MUX2 MUX1 : Selects 1.1V (VBG)  
  ADMUX = _BV (REFS0) | _BV (MUX3) | _BV (MUX2) | _BV (MUX1);
  delayMicroseconds(250); //Improves Accuracy
  ADCSRA |= _BV( ADSC );  // start conversion
  while (ADCSRA & _BV (ADSC))
  { 
  }  // wait for conversion to complete
  int results = (((InternalReferenceVoltage * 1024) / ADC) + 5) / 10; 
  return results;
} // end of battVoltage


/********************************************************************************
To find the InternalReferenceVoltage, Run this code and measure the voltage on the AREF pin
it should be close to 1.1v or 1100m

void setup ()
{
  analogReference (INTERNAL);
  analogRead (A0);  // force voltage reference to be turned on
}
void loop () { }
*/
