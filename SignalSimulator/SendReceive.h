#include "Pins.h"
#include "Interrupts.h"
#include "Commands.h"

//these are the widths of the pulses
#define START_PULSE 4440                //width of the start pulse
#define START_PULSE_TOLERANCE 100
#define START_PULSE_MIN START_PULSE - START_PULSE_TOLERANCE
#define START_PULSE_MAX START_PULSE + START_PULSE_TOLERANCE
#define DATA_START_DELAY 200            //amount of time after start pulse has gone low to wait before sampling the bits
#define BIT_PULSE 440                   //width of one bit
#define THIRD_BIT_PULSE 440 / 3
#define BUTTON_PULSE 147                //width of the low pulse when a button is pressed
#define BIT_COUNT 13                    //number of bits to read in
#define DATA_MASK 1 << (BIT_COUNT - 1)
#define MILLIS_WAIT_AFTER_COMMAND 10    //milliseconds to wait after a command has been received before sending a command
#define MILLIS_BETWEEN_COMMANDS 500    //millisconds to wait after sending a command before sending another command

unsigned long lastSentCommand = millis();
unsigned long lastReceivedCommand = millis();

void sendCommand(word command) {
  noInterrupts(); //Sending data will cause data to appear on the DATA_IN pin, ignore it whilse we're sending
  os_intr_lock();

  GPOC = LED_BIT;
      
  Serial.print("Sending 0x");
  Serial.println(command, HEX);

  unsigned start = micros();

  GPOS = DBG_BIT;
  while(micros() - start < 100) {;}
  GPOC = DBG_BIT;

  GPOS = DATA_OUT_BIT; //Start of the start pulse

  start = micros();
  while(micros() - start <= START_PULSE - 50) {;} //confirmed

  GPOC = DATA_OUT_BIT; //End of the start pulse, start of the button pulse
  
  start = micros();
  while (micros() - start <= BUTTON_PULSE + 32) {;} //confirmed
  //End of the button pulse, start of the data pulses

  GPOS = DBG_BIT;
  
  //Write the bits
  for (word bitPos = DATA_MASK; bitPos > 0; bitPos >>= 1)
  {
    if (command & bitPos)
    {
      GPOS = DATA_OUT_BIT;
      start = micros();
      while(micros() - start <= BIT_PULSE) {;} //confirmed
    }
    else
    {
      GPOC = DATA_OUT_BIT; 
      start = micros();
      while(micros() - start < BIT_PULSE) {;} //confirmed
    }    
  }

  GPOC = DATA_OUT_BIT;
  GPOC = DBG_BIT;

  os_intr_unlock();
  interrupts();
  GPOS = LED_BIT;

  lastSentCommand = millis();
}

word receiveCommand(){
  noInterrupts();
  os_intr_lock();
  
  GPOS = DBG_BIT; //Debug high while checking start pulse length
  
  //start timing first pulse
  unsigned startTime = micros();
  unsigned long endTime;
  unsigned long pulseDuration;

  //wait for the first long pulse to go low
  while (digitalRead(DATA_IN) && pulseDuration < START_PULSE_MAX)
  { 
    endTime = micros(); //update endTime to check if we have detected a too long pulse
    pulseDuration = endTime - startTime;
  }
  GPOC = DBG_BIT; //Debug low after checking start pulse length

  #ifndef IGNORE_INVALID_START_PULSE
  if (pulseDuration < START_PULSE_MIN || pulseDuration > START_PULSE_MAX) {
    Serial.print("Timed out detecting start pulse @ ");
    Serial.println(pulseDuration);
    
    os_intr_unlock();
    interrupts();

    lastReceivedCommand = millis();
    return 0;
  }
  #endif

  //here we have confirmed that the start pulse was ~4440us and the signal is now low (Start Confirm on diagram)

  delayMicroseconds(DATA_START_DELAY);

  // (Button detect on diagram)

  word data;
  word notBitPos;
  //Read the bits  
  for (word bitPos = DATA_MASK; bitPos > 0; bitPos >>= 1)
  {
    delayMicroseconds(THIRD_BIT_PULSE);
    GPOS = DBG_BIT; //Debug signal high whilst checking the bit
    
    notBitPos = ~bitPos;
    if (digitalRead(DATA_IN))
      data |= bitPos;
    else // else clause added to ensure function timing is ~balanced
      data &= notBitPos;

    delayMicroseconds(THIRD_BIT_PULSE);
    GPOC = DBG_BIT; //Debug signal low after checking the bit
    delayMicroseconds(THIRD_BIT_PULSE);
  }

  digitalWrite(DBG, LOW); //Debug signal high after checking button is pressed
  //End getting the bits

  Serial.print("Received 0x");
  Serial.println(data, HEX);

  os_intr_unlock();
  interrupts();
  
  lastReceivedCommand = millis();
}
