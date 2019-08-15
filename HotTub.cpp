#include "HotTub.h"
#include "Commands.h"
#include "Temperatures.h"
#include <Arduino.h>

//todo - extract Serial.print lines to a debug function that can be handles externally.

HotTub::HotTub(int dataInPin, int dataOutPin, int debugPin) {
  this->dataInPin = dataInPin;
  this->dataInPinBit = 1 << dataInPin;
  
  this->dataOutPin = dataOutPin;
  this->dataOutPinBit = 1 << dataOutPin;

  this->ledPin = ledPin;
  this->ledPinBit = 1 << ledPin;

  this->debugPin = debugPin;
  this->debugPinBit = 1 << debugPin;

  currentState = new HotTubState();
  targetState = new HotTubState();
}

HotTubState* HotTub::getCurrentState() {
  return currentState;
}

//todo - change this to return a copy of the array as we don't want external code changing its state!
HotTubState* HotTub::getTargetState() {
  return targetState;
}



void HotTub::maximumTemperature(int temperature, bool isCelsius) {
  limitTemperature = temperature;
  limitTemperatureIsCelsius = isCelsius;
}

int HotTub::getMaximumTemperature() {
  return limitTemperature;
}

void HotTub::temperatureLock(bool state) {
  temperatureLockEnabled = state;
}

bool HotTub::isTemperatureLockEnabled() {
  return temperatureLockEnabled;
}

void HotTub::panelLock(bool state) {
  controlPanelLockEnabled = state;
}

bool HotTub::isPanelLockEnabled() {
  return controlPanelLockEnabled;
}

void HotTub::autoRestart(bool state) {
  autoRestartEnabled = state;
}

bool HotTub::isAutoRestartEnabled() {
  return autoRestartEnabled;
}

int HotTub::getErrorCode() {
  return currentState->errorCode;
}

void HotTub::sendCommand(word command) {
  Serial.print("Sending 0x");
  Serial.println(command, HEX);

  noInterrupts(); //Sending data will cause data to appear on the DATA_IN pin, ignore it whilse we're sending
  os_intr_lock();

  GPOC = ledPinBit;

  unsigned start = micros();

  GPOS = debugPinBit;
  while(micros() - start < 100) {;}
  GPOC = debugPinBit;

  GPOS = dataOutPinBit; //Start of the start pulse

  start = micros();
  while(micros() - start <= START_PULSE - 50) {;} //confirmed

  GPOC = dataOutPinBit; //End of the start pulse, start of the button pulse
  
  start = micros();
  while (micros() - start <= BUTTON_PULSE + 32) {;} //confirmed
  //End of the button pulse, start of the data pulses

  GPOS = debugPinBit;
  
  //Write the bits
  for (word bitPos = DATA_MASK; bitPos > 0; bitPos >>= 1)
  {
    if (command & bitPos)
    {
      GPOS = dataOutPinBit;
      start = micros();
      while(micros() - start <= BIT_PULSE - 40) {;} //confirmed
    }
    else
    {
      GPOC = dataOutPinBit; 
      start = micros();
      while(micros() - start < BIT_PULSE + 40) {;} //confirmed
    }    
  }

  GPOC = dataOutPinBit;
  GPOC = debugPinBit;

  os_intr_unlock();
  interrupts();
  GPOS = ledPinBit;

  lastSentCommand = millis();
}

void HotTub::dataAvailable() {
  word data = receiveCommand();
  decodeCommand(data); 
  lastReceivedCommand = millis();
}

word HotTub::receiveCommand(){
    noInterrupts();
    os_intr_lock();
    
    GPOS = debugPinBit; //Debug high while checking start pulse length
    
    //start timing first pulse
    unsigned startTime = micros();
    unsigned long endTime;
    unsigned long pulseDuration;
  
    //wait for the first long pulse to go low
    while (digitalRead(dataInPin) && pulseDuration < START_PULSE_MAX)
    { 
      endTime = micros(); //update endTime to check if we have detected a too long pulse
      pulseDuration = endTime - startTime;
    }
    GPOC = debugPinBit; //Debug low after checking start pulse length
  
    #ifndef IGNORE_INVALID_START_PULSE
    if (pulseDuration < START_PULSE_MIN || pulseDuration > START_PULSE_MAX) {
      os_intr_unlock();
      interrupts();
  
      Serial.print("Timed out detecting start pulse @ ");
      Serial.println(pulseDuration);
  
      lastReceivedCommand = millis();
      return 0;
    }
    #endif
  
    //here we have confirmed that the start pulse was ~4440us and the signal is now low (Start Confirm on diagram)
  
    delayMicroseconds(DATA_START_DELAY); //todo - change to a loop
  
    // (Button detect on diagram)
  
    word data;
    word notBitPos;
    //Read the bits  
    for (word bitPos = DATA_MASK; bitPos > 0; bitPos >>= 1)
    {
      delayMicroseconds(THIRD_BIT_PULSE); //todo - change to a loop
      GPOS = debugPinBit; //Debug signal high whilst checking the bit
      
      notBitPos = ~bitPos;
      if (digitalRead(dataInPin))
        data |= bitPos;
      else // else clause added to ensure function timing is ~balanced
        data &= notBitPos;
  
      delayMicroseconds(THIRD_BIT_PULSE); //todo - change to a loop
      GPOC = debugPinBit; //Debug signal low after checking the bit
      delayMicroseconds(THIRD_BIT_PULSE); //todo - change to a loop
    }
    //End getting the bits
  
    os_intr_unlock();
    interrupts();
  
    lastReceivedCommand = millis();
  
    Serial.print("Received 0x");
    Serial.println(data, HEX);
    
    return data;
  }

void HotTub::decodeCommand(word command) {
  if (command == 0)
    return; //todo - do something with an invalid command?!?!
  
  if ((command >= CMD_ERROR_PKT1 && command <= CMD_END) || command == CMD_FLASH) {
    Serial.println("Decoding status command");
    decodeStatus(command);
    return;
  }

  if (command >= TEMP_104F && command <= TEMP_9C) {
    Serial.println("Decoding temperature command");
    currentState->temperature = decodeTemperature(command);
    return;
  }

  if (command >= CMD_BTN_HEAT) {
    Serial.println("Decoding button command");
    decodeButton(command);
    return;
  }
}

void HotTub::decodeButton(word command) {
  String button;
  
  switch(command) {
    case CMD_BTN_HEAT:
      button = "Heater";
      break;
    case CMD_BTN_PUMP:
      button = "Pump";
      break;
    case CMD_BTN_BLOW:
      button = "Blower";
      break;
    case CMD_BTN_TEMP_DN:
      button = "Temperature down";
      break;
    case CMD_BTN_TEMP_UP:
      button = "Temperature up";
      break;
  }

  Serial.print(button);
  Serial.println(" button was pressed");
}

void HotTub::decodeStatus(word command) {
   currentState->pumpEnabled = false;
   currentState->blowerEnabled = false;
   currentState->heaterEnabled = false;
   currentState->heaterHeating = false;

   currentState->errorCode = 0;
  
   switch(command) {
    case CMD_STATE_ALL_OFF_F:
    case CMD_STATE_ALL_OFF_C:
      break;
    case CMD_STATE_PUMP_ON_C:
    case CMD_STATE_PUMP_ON_F:
      currentState->pumpEnabled = true;
      break;
    case CMD_STATE_BLOWER_ON_C:
    case CMD_STATE_BLOWER_ON_F:
      currentState->blowerEnabled = true;
      break;
    case CMD_STATE_PUMP_ON_HEATER_ON:
      currentState->heaterEnabled = true;
      break;
    case CMD_STATE_PUMP_ON_HEATER_HEATING:
      currentState->heaterEnabled = true;
      currentState->heaterHeating = true;
      break;
    default:
      currentState->errorCode = decodeError(command);
  }
}

int HotTub::decodeError(word command) {
  String text;
  int code = 0;
  
  switch (command) {
    case CMD_ERROR_PKT1:
      text = "Error 1,3,4 1st Packet, error to follow...";
      break;
    case CMD_ERROR_PKT2:
      text = "Error 2,5,END 1st Packet, error to follow...";
      break;
    case CMD_ERROR_1:
      code = 1;
      text = "Flow sensor stuck in on state";
      break;
    case CMD_ERROR_2:
      code = 2;
      text = "Flow sensor stuck in off state";
      break;
    case CMD_ERROR_3:
      code = 3;
      text = "Water temperature below 4C";
      break;
    case CMD_ERROR_4:
      code = 4;
      text = "Water temperature above 50C";
      break;
    case CMD_ERROR_5:
      code = 5;
      text = "Unsuitable power supply";
      break;
  }

  Serial.print("Error code ");
  Serial.print(code);
  Serial.print(" received - ");
  Serial.println(text);

  return code;
}

int HotTub::decodeTemperature(word command) {
  switch (command) {
    case TEMP_40C: return 40;  
    case TEMP_39C: return 39;
    case TEMP_38C: return 38;    
    case TEMP_37C: return 37;
    case TEMP_36C: return 36;  
    case TEMP_35C: return 35;   
    case TEMP_34C: return 34;
    case TEMP_33C: return 33;
    case TEMP_32C: return 32;
    case TEMP_31C: return 31;
    case TEMP_30C: return 30;
    case TEMP_29C: return 29;
    case TEMP_28C: return 28;
    case TEMP_27C: return 27;
    case TEMP_26C: return 26;
    case TEMP_25C: return 25;
    case TEMP_24C: return 24;
    case TEMP_23C: return 23;
    case TEMP_22C: return 22;
    case TEMP_21C: return 21;
    case TEMP_20C: return 20;
    case TEMP_19C: return 19;
    case TEMP_18C: return 18;
    case TEMP_17C: return 17;
    case TEMP_16C: return 16;
    case TEMP_15C: return 15;
    case TEMP_14C: return 14;
    case TEMP_13C: return 13;
    case TEMP_12C: return 12;
    case TEMP_11C: return 11;
    case TEMP_10C: return 10;
    case TEMP_9C : return 9;

    case TEMP_104F: return 104;
    case TEMP_102F: return 102;
    case TEMP_101F: return 101;
    case TEMP_99F: return 99;
    case TEMP_97F: return 97;
    case TEMP_95F: return 95;
    case TEMP_93F: return 93;
  }
}

void HotTub::autoRestartCheck() {
  //todo - we need to check if the pump has sent the END command or the pump is off but the button hasn't been pressed
  if (autoRestartEnabled) {
    sendCommand(CMD_BTN_HEAT);
  }
}

void HotTub::targetTemperatureCheck(){
  if (currentState->temperature != targetState->temperature
    && millis() - lastSentCommand > MILLIS_BETWEEN_COMMANDS
    && millis() - lastReceivedCommand > MILLIS_WAIT_AFTER_COMMAND
    && (lastReceivedCommand > 0 || ALLOW_SEND_WITHOUT_RECEIVE))
  {
    Serial.println("Adjusting target temperature");

    if (currentState->temperature < targetState->temperature)
        sendCommand(CMD_BTN_TEMP_UP);
        
    if (currentState->temperature > targetState->temperature)
        sendCommand(CMD_BTN_TEMP_DN);
  }
}

void HotTub::setup(void (&onStateChange)()) {
  stateChanged = onStateChange;

  pinMode(ledPin, OUTPUT);    
  pinMode(dataInPin, INPUT);
  pinMode(dataOutPin, OUTPUT);
  pinMode(debugPin, OUTPUT);
}

void HotTub::loop() {
  autoRestartCheck();

  if (!DISABLE_TARGET_TEMPERATURE) {
    targetTemperatureCheck();
  }
}
