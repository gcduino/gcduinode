void gotoSleep(){
  radio.powerDown();  //Put the Radio in Low Power Mode, Writing data will wake it.
  watchdogOn(); // Turn on the watch dog timer.
  //store ADC settings
  byte old_ADCSRA = ADCSRA;
  // disable ADC
  ADCSRA = 0;  

  set_sleep_mode (SLEEP_MODE_PWR_DOWN);  
  sleep_enable();

    // Do not interrupt before we go to sleep, or the
    // ISR will detach interrupts and we won't wake.
    noInterrupts ();

    // will be called when pin D3 (INT1) goes low 
    attachInterrupt (1, wake, LOW);  
    //The Internal Pull Up MUST be enabled
  
  // turn off brown-out enable in software
  // BODS must be set to one and BODSE must be set to zero within four clock cycles
  MCUCR = bit (BODS) | bit (BODSE);
  // The BODS bit is automatically cleared after three clock cycles
  MCUCR = bit (BODS); 

  // We are guaranteed that the sleep_cpu call will be done
  // as the processor executes the next instruction after
  // interrupts are turned on.
  interrupts ();  // one cycle
  sleep_cpu ();   // one cycle
  
  //********************Ultra Lower Power MODE now ON*********************
  //*********CPU will wake when HERE the interrupt pin goes LOW***********
  //Restore ADC settings to ON
  sleep_disable(); // cancel sleep as a precaution
  wdt_disable();   // Disable watchdog mode after waking.
  ADCSRA = old_ADCSRA;  
}

void wake (){
  // cancel sleep as a precaution
  //sleep_disable();
  // must do this as the pin will probably stay low for a while
  detachInterrupt (0);
}  // end of wake

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
  WDTCSR = B00000101;  //500mS
  //B00(1)00(001) == Change Braketed to: 
  //0000  0001  0010  0011  0100  0101  0110  0111  1000  1001
  // 16    32    64    125   250   500  1000  2000  4000  8000 (mS)
  // Enable the watchdog timer interupt.
  WDTCSR = WDTCSR | B01000000;
  MCUSR = MCUSR & B11110111;
}

ISR(WDT_vect)
{
  //sleep_count ++; // keep track of how many sleep cycles
  // have been completed.
}
