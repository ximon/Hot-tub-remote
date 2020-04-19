#include "HotTub.h"
#include "Commands.h"
#include "Temperatures.h"
#include "SendReceive.h"
#include <Arduino.h>

//todo - extract Serial.print lines to a debug function that can be handled externally.
//#define DISABLE_TARGET_TEMPERATURE

HotTub::HotTub(int dataInPin, int dataOutPin, int debugPin)
: SendReceive(dataInPin, dataOutPin, debugPin) {
  currentState = new CurrentState();
  currentState->pumpState = PUMP_UNKNOWN;

  targetState = new TargetState();
  targetState->pumpState = PUMP_UNKNOWN;
  targetState->targetTemperature = INIT_TEMP;
}

/*
 *  State Management
 */

//todo - change this to return a copy of the array as we don't want external code changing its state!
CurrentState* HotTub::getCurrentState() {
  return currentState;;
}

TargetState* HotTub::getTargetState() {
  return targetState;
}

void HotTub::setTargetState(int newState) {
  if(targetState->pumpState != newState) {
    targetState->pumpState = newState;
    stateChanged(targetState->pumpState, newState);
  }
}

void HotTub::setTargetTemperature(int temp) {
  targetState->targetTemperature = temp;
}

void HotTub::setLimitTemperature(int temp) {
  limitTemperature = temp;
  
  if (targetState->targetTemperature > temp) {
    targetState->targetTemperature = temp;
  }
}

int HotTub::getLimitTemperature() {
  return limitTemperature;
}

int HotTub::getErrorCode() {
  return currentState->errorCode;
}




/*
 *  Commands
 */

void HotTub::onCommandSent(word command) {
  Serial.println("Command Sent!");
  if (command == CMD_BTN_TEMP_UP || command == CMD_BTN_TEMP_DN) {
    Serial.println("Next temperature reading will be for the target temperature");
    temperatureDestination = TEMP_TARGET; //to capture the newly changed target temperature
  } 
}

void HotTub::onCommandReceived(word command) {
  //Serial.print("Handling command 0x");
  //Serial.println(command, HEX);
  
  if (command == 0) {
    Serial.print("Invalid command received");
    return;
  }

  if (command >= CMD_ERROR_4 && command <= CMD_ERROR_1) {
    handleReceivedError(command);
    return;
  }

  if ((command >= CMD_ERROR_PKT1 && command <= CMD_END) || command == CMD_FLASH) {
    handleReceivedStatus(command);
    return;
  }

  if (command >= TEMP_104F && command <= TEMP_9C) {
    handleReceivedTemperature(command);
    return;
  }

  if (command >= CMD_BTN_HEAT) {
    handleReceivedButton(command);
    return;
  }
}

String HotTub::stateToString(int pumpState) {
  switch (currentState->pumpState){
    case PUMP_OFF:            return "Off";
    case PUMP_FILTERING:      return "Filtering";
    case PUMP_HEATING:        return "Heating";
    case PUMP_HEATER_STANDBY: return "Heater on Standby";
    case PUMP_BUBBLES:        return "Bubbles";
    default:                  return "UNKNOWN!!!";
  }
}

String HotTub::errorToString(int errorCode) {
  switch (errorCode) {
    case 1:  return "Flow sensor stuck in on state";
    case 2:  return "Flow sensor stuck in off state";
    case 3:  return "Water temperature below 4C";
    case 4:  return "Water temperature above 50C";
    case 5:  return "Unsuitable power supply";
    default: return "UNKNOWN!!!";
  }
}

String HotTub::buttonToString(int buttonCommand) {
   switch(buttonCommand) {
    case CMD_BTN_HEAT:    return "Heater";
    case CMD_BTN_PUMP:    return "Pump";
    case CMD_BTN_BLOW:    return "Blower";
    case CMD_BTN_TEMP_DN: return "Temperature down";
    case CMD_BTN_TEMP_UP: return "Temperature up";
    default:              return "UNKNOWN!!!";
  }
}

void HotTub::handleReceivedError(word command) {
  decodeError(command);
   
  if (currentState->errorCode > 0) {
    Serial.print("Error code ");
    Serial.print(currentState->errorCode);
    Serial.print(" received - ");
    Serial.println(errorToString(currentState->errorCode));
  }
}

void HotTub::handleReceivedStatus(word command) {
  //Serial.print("Decoding status command, ");
  decodeStatus(command);

  //Serial.print("state is : ");
  //Serial.println(stateToString(currentState->pumpState));
}

void HotTub::handleReceivedTemperature(word command) {
  Serial.print("Decoding temperature command, ");
  int temperature = decodeTemperature(command);
    
  switch (temperatureDestination) {
    case TEMP_CURRENT:
      currentState->temperature = temperature;
      Serial.print("Temperature is ");
      Serial.println(currentState->temperature);
      break;
    case TEMP_TARGET:
      tempIgnoreStart = millis();
      temperatureDestination = TEMP_IGNORE;
      
      currentState->targetTemperature = temperature;
      Serial.print("Target temperature is ");
      Serial.println(currentState->targetTemperature);
      break;
    case TEMP_IGNORE:
      if (millis() - TEMP_IGNORE_TIME > tempIgnoreStart) {
        temperatureDestination = TEMP_CURRENT;
        Serial.println("Next reading will be for the current temperature");
      } else {
        Serial.println("Ignoring temperature reading...");
      }
      break;
  }
}

void HotTub::handleReceivedButton(word command) {
  //Serial.print("Decoding button command,");

  //Serial.print(buttonToString(command));
  //Serial.println(" button was pressed");

  lastButton = command;

  switch (command) {
    case CMD_BTN_TEMP_UP:
    case CMD_BTN_TEMP_DN:
      if (!temperatureLockEnabled) {
        if (temperatureDestination == TEMP_TARGET || temperatureDestination == TEMP_IGNORE) {
          //Serial.print("Changing target temp from ");
          //Serial.print(targetState->targetTemperature);

          int newTargetTemp = command == CMD_BTN_TEMP_UP
            ? targetState->targetTemperature + 1 
            : targetState->targetTemperature - 1;

          if (newTargetTemp > limitTemperature)
            newTargetTemp = limitTemperature;

          if (newTargetTemp > MAX_TEMP)
            newTargetTemp = MAX_TEMP;

          if (newTargetTemp < MIN_TEMP)
            newTargetTemp = MIN_TEMP;

          setTargetTemperature(newTargetTemp);
          
          //Serial.print(" to ");
          //Serial.println(targetState->targetTemperature);
        }
      } else {
        //Serial.println("Temperature is locked, keeping current target temperature");
      }

      temperatureDestination = TEMP_TARGET;
      
      break;
    case CMD_BTN_PUMP:
      if (currentState->pumpState == PUMP_FILTERING || currentState->pumpState == PUMP_HEATER_STANDBY || currentState->pumpState == PUMP_HEATING) {
        manuallyTurnedOff = true;
      }
      break;
  }

  lastButtonPressTime = millis();
}

void HotTub::decodeStatus(word command) {
  int newState = currentState->pumpState;

  switch(command) {
    case CMD_STATE_ALL_OFF_F:
    case CMD_STATE_ALL_OFF_C:
        newState = PUMP_OFF;
      break;
    case CMD_STATE_PUMP_ON_C:
    case CMD_STATE_PUMP_ON_F:
      newState = PUMP_FILTERING;
      break;
    case CMD_STATE_BLOWER_ON_C:
    case CMD_STATE_BLOWER_ON_F:
      newState = PUMP_BUBBLES;
      break;
    case CMD_STATE_PUMP_ON_HEATER_ON:
      newState = PUMP_HEATER_STANDBY;
      break;
    case CMD_STATE_PUMP_ON_HEATER_HEATING:
      newState = PUMP_HEATING;
      break;
    case CMD_FLASH:
      break;
  }

  if (targetState->pumpState == PUMP_UNKNOWN)
    targetState->pumpState = newState;

  if (currentState->pumpState != newState) {
    stateChanged(currentState->pumpState, newState);
    currentState->pumpState = newState;
    currentState->errorCode = 0;
  }
}

void HotTub::decodeError(word command) {
  int errorCode = 0;
  
  switch (command) {
    case CMD_ERROR_PKT1:
    case CMD_ERROR_PKT2:
      break;
    case CMD_ERROR_1:
      errorCode = 1;
      break;
    case CMD_ERROR_2:
      errorCode = 2;
      break;
    case CMD_ERROR_3:
      errorCode = 3;
      break;
    case CMD_ERROR_4:
      errorCode = 4;
      break;
    case CMD_ERROR_5:
      errorCode = 5;
      break;
  }

  if (errorCode > 0){
    if (currentState->pumpState != PUMP_ERROR) {
      stateChanged(currentState->pumpState, PUMP_ERROR);
      currentState->pumpState = PUMP_ERROR;
      currentState->errorCode = errorCode;
    }
  }
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
    case TEMP_92F: return 92;
    case TEMP_90F: return 90;
    case TEMP_88F: return 88;
    case TEMP_86F: return 86;
    case TEMP_84F: return 84;
    case TEMP_83F: return 83;
    case TEMP_81F: return 81;
    case TEMP_79F: return 79;
    case TEMP_77F: return 77;
    case TEMP_75F: return 75;
    case TEMP_74F: return 74;
    case TEMP_72F: return 72;
    case TEMP_70F: return 70;
    case TEMP_66F: return 66;
    case TEMP_65F: return 65;
    case TEMP_63F: return 63;
    case TEMP_61F: return 61;
    case TEMP_59F: return 59;
    case TEMP_57F: return 57;
    case TEMP_56F: return 56;
    case TEMP_54F: return 54;
    case TEMP_52F: return 52;
  }
}



/*
 * Target state checks
 */

void HotTub::autoRestartCheck() {
    if (!manuallyTurnedOff && autoRestartEnabled && currentState->pumpState == PUMP_OFF && targetState->pumpState != PUMP_HEATING) {
    Serial.println("Auto restarting...");
    setTargetState(PUMP_HEATING);
  }
}

void HotTub::targetTemperatureCheck(){
  if (millis() - TEMP_IGNORE_TIME < lastButtonPressTime && lastButtonPressTime > 0)
    return; //leave temperatures alone until buttons aren't being pressed

  //if the target temperature is 0 we need to send temperature button press so that the display blinks and shows us the target temperature
  if (currentState->targetTemperature == 0 && millis() > TEMP_IGNORE_TIME) {
    Serial.print("Getting current target temperature...");
    queueCommand(CMD_BTN_TEMP_DN);
    lastButtonPressTime = millis();
  }

  #ifndef DISABLE_TARGET_TEMPERATURE 
  if (currentState->targetTemperature > 0
    && currentState->targetTemperature != targetState->targetTemperature)
  {
    if (currentState->targetTemperature < targetState->targetTemperature) {
        Serial.println("Auto-Adjusting target temperature, up");
        queueCommand(CMD_BTN_TEMP_UP);
    }
        
    if (currentState->targetTemperature > targetState->targetTemperature) {
        Serial.println("Auto-Adjusting target temperature, down");
        queueCommand(CMD_BTN_TEMP_DN);
    }
  }
  #endif
}

void HotTub::targetStateCheck(){
  int requestedState = targetState->pumpState;
  
  if (currentState->pumpState == requestedState)
    return;

  if (millis() - BUTTON_SEND_DELAY < getLastCommandSentTime())
    return;

  if (requestedState == PUMP_OFF) {
    queueCommand(CMD_BTN_PUMP);
    return;
  }

  if (requestedState == PUMP_FILTERING) {
    if (currentState->pumpState == PUMP_HEATING || currentState->pumpState == PUMP_HEATER_STANDBY)
      queueCommand(CMD_BTN_HEAT); //to turn off the heat
      
    if (currentState->pumpState == PUMP_OFF)
      queueCommand(CMD_BTN_PUMP); //to turn on the pump
      
    return;
  }

  if (requestedState == PUMP_HEATING || requestedState == PUMP_HEATER_STANDBY) {
    if (currentState->pumpState == PUMP_BUBBLES) {
      queueCommand(CMD_BTN_BLOW);
      return;
    }
    
    if (!manuallyTurnedOff && currentState->pumpState != PUMP_HEATING && currentState->pumpState != PUMP_HEATER_STANDBY)
      queueCommand(CMD_BTN_HEAT);
  }

  if (requestedState == PUMP_BUBBLES) {
    queueCommand(CMD_BTN_BLOW);
    return;
  }
}

/*
 * Setup and loop
 */

void HotTub::setup(void (&onStateChange)(int oldState, int newState)) {
  stateChanged = onStateChange;
}

long lastPrintTime; 
void HotTub::loop() {
  SendReceive::loop();
  
  //dont send if we've recently sent, or never sent
  if (millis() - WAIT_BETWEEN_SENDING_AUTO_COMMANDS < getLastCommandQueuedTime() && getLastCommandQueuedTime() > 0) {
    if (millis() - DELAY_BETWEEN_PRINTS > lastPrintTime) {
      lastPrintTime = millis();
      Serial.println("Waiting for auto send delay");
    }
    return;
  }

  //dont send anything unless we have waited the delay between commands or if the queue is full
  if (getCommandQueueCount() == MAX_OUT_COMMANDS) {
    if (millis() - DELAY_BETWEEN_PRINTS > lastPrintTime) {
      lastPrintTime = millis();
      Serial.println("Command queue maxed out!");
    }
    return;
  }

  //autoRestartCheck();
  targetTemperatureCheck();
  //targetStateCheck();
}
