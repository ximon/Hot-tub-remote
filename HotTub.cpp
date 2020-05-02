#include "HotTub.h"
#include "Commands.h"
#include "Temperatures.h"
#include "SendReceive.h"

//todo - extract Serial.print lines to a debug function that can be handled externally.
#define DISABLE_TARGET_TEMPERATURE

#define DEBUG_TUB

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
  return currentState;
}

TargetState* HotTub::getTargetState() {
  return targetState;
}

void HotTub::setTargetState(int newState) {
  if(targetState->pumpState != newState) {
#ifdef DEBUG_TUB
    Serial.print("HOTTUB->Changing state from ");
    Serial.print(stateToString(targetState->pumpState));
    Serial.print(" to ");
    Serial.println(stateToString(newState));
#endif
    
    targetState->pumpState = newState;
    stateChanged();
  }
}

int HotTub::maxTemperatureValid(int maxTemp) {
  bool aboveMax = maxTemp > MAX_TEMP;
  bool belowMin = maxTemp < MIN_TEMP;

  if (aboveMax)
    return 1;

  if (belowMin)
    return -1;

  return 0;
}

int HotTub::targetTemperatureValid(int targetTemp) {
  bool aboveMax = targetTemp > limitTemperature;
  bool belowMin = targetTemp < MIN_TEMP;

  if (aboveMax)
    return 1;
    
  if (belowMin)
    return -1;
    
  return 0;
}

int HotTub::setTargetTemperature(int temp) {
  int status = targetTemperatureValid(temp);

  if (status != 0)
    return 0;

#ifdef DEBUG_TUB
  Serial.print("HOTTUB->Setting target temperature to ");
  Serial.println(temp);
#endif

  targetState->targetTemperature = temp;
  stateChanged();
  
  return status;
}

int HotTub::setLimitTemperature(int temp) {
  int status = maxTemperatureValid(temp);

  if (status != 0)
    return 0; 

#ifdef DEBUG_TUB
  Serial.print("HOTTUB->Setting limit temperature to ");
  Serial.println(temp);
#endif

  limitTemperature = temp;
  
  if (targetState->targetTemperature > temp) {
    targetState->targetTemperature = temp;
  }

  stateChanged();

  return status;
}

int HotTub::getLimitTemperature() {
  return limitTemperature;
}

void HotTub::setTemperatureLock(bool enable) {
  temperatureLockEnabled = enable;
  stateChanged();
}

void HotTub::setAutoRestart(bool enable) {
  autoRestartEnabled = enable;
  stateChanged();
}


int HotTub::getErrorCode() {
  return currentState->errorCode;
}

char* HotTub::getStateJson() {
  const size_t capacity = 2*JSON_OBJECT_SIZE(7) + JSON_OBJECT_SIZE(6);
  DynamicJsonDocument doc(capacity);
  char* json = new char[300];

  CurrentState* currentState = getCurrentState();
  JsonObject currentJson = doc.createNestedObject("currentState");
  currentJson["pumpState"] = currentState->pumpState;
  currentJson["temperature"] = currentState->temperature;
  currentJson["targetTemperature"] = currentState->targetTemperature;

  TargetState* targetState = getTargetState();
  JsonObject targetJson = doc.createNestedObject("targetState");
  targetJson["pumpState"] = targetState->pumpState;
  targetJson["targetTemperature"] = targetState->targetTemperature;
  
  doc["autoRestart"] = autoRestartEnabled;
  doc["limitTemperature"] = getLimitTemperature();
  doc["temperatureLock"] = temperatureLockEnabled;
  doc["errorCode"] = currentState->errorCode;

  serializeJson(doc, json, 300);
  return json;
}


/*
 *  Commands
 */

void HotTub::onCommandSent(unsigned int command) {
#ifdef DEBUG_TUB
  Serial.println("HOTTUB->Command Sent!");
#endif

  if (command == CMD_BTN_TEMP_UP || command == CMD_BTN_TEMP_DN) {
#ifdef DEBUG_TUB
    Serial.println("HOTTUB->Next temperature reading will be for the target temperature");
#endif
    temperatureDestination = TEMP_TARGET; //to capture the newly changed target temperature
  } 
}

void HotTub::onCommandReceived(unsigned int command) {
#ifdef DEBUG_TUB_VERBOSE
  Serial.print("HOTTUB->Handling command 0x");
  Serial.println(command, HEX);
#endif
  
  if (command == 0) {
#ifdef DEBUG_TUB
    Serial.print("HOTTUB->Invalid command received");
#endif
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
  switch (pumpState){
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

void HotTub::handleReceivedError(unsigned int command) {
  decodeError(command);
   
  if (currentState->errorCode > 0) {

#ifdef DEBUG_TUB
    Serial.print("HOTTUB->Error code ");
    Serial.print(currentState->errorCode);
    Serial.print(" received - ");
    Serial.println(errorToString(currentState->errorCode));
#endif

    stateChanged();
  }
}

void HotTub::handleReceivedStatus(unsigned int command) {
#ifdef DEBUG_TUB_VERBOSE
  Serial.print("HOTTUB->Decoding status command, ");
#endif

  decodeStatus(command);

#ifdef DEBUG_TUB_VERBOSE
  Serial.print("state is : ");
  Serial.println(stateToString(currentState->pumpState));
#endif
}

void HotTub::handleReceivedTemperature(unsigned int command) {
  int temperature = decodeTemperature(command);
    
#ifdef DEBUG_TUB_VERBOSE
  Serial.print("HOTTUB->Decoding temperature command, ");
  Serial.print("received ");
  Serial.println(temperature);
#endif
    
  switch (temperatureDestination) {
    case TEMP_PREP_CURRENT:
      if (millis() - TEMP_CURRENT_DELAY > tempIgnoreStart) {
#ifdef DEBUG_TUB_VERBOSE
        Serial.println("HOTTUB->Next reading will be for the current temperature");
#endif
        temperatureDestination = TEMP_CURRENT;
      }
#ifdef DEBUG_TUB
      else {
        Serial.println("HOTTUB->Ignoring Current Temperature reading...");
      }
#endif
      break;
    case TEMP_CURRENT:
      if (currentState->temperature != temperature) {
        currentState->temperature = temperature;
        stateChanged();
      }
#ifdef DEBUG_TUB_VERBOSE
      Serial.print("HOTTUB->Temperature is ");
      Serial.println(currentState->temperature);
#endif
      break;
    case TEMP_PREP_TARGET:
      if (millis() - TEMP_TARGET_DELAY > tempIgnoreStart) {
#ifdef DEBUG_TUB
        Serial.println("HOTTUB->Next reading will be for the Target Temperature");
#endif
        temperatureDestination = TEMP_TARGET;
      }
#ifdef DEBUG_TUB
      else {
        Serial.println("HOTTUB->Ignoring Target Temperature reading...");
      }
#endif
      break;
    case TEMP_TARGET:
      if (currentState->targetTemperature != temperature) {
        currentState->targetTemperature = temperature;
        stateChanged();
      }
#ifdef DEBUG_TUB_VERBOSE
      Serial.print("HOTTUB->Target temperature is ");
      Serial.println(currentState->targetTemperature);
#endif
      
      tempIgnoreStart = millis();
      temperatureDestination = TEMP_PREP_CURRENT;
      break;
  }
}

void HotTub::handleReceivedButton(unsigned int command) {
#ifdef DEBUG_TUB
  Serial.print("HOTTUB->Decoding button command,");
  Serial.print(buttonToString(command));
  Serial.println(" button was pressed");
#endif

  lastButton = command;

  switch (command) {
    case CMD_BTN_TEMP_UP:
    case CMD_BTN_TEMP_DN:
      temperatureDestination = TEMP_PREP_TARGET;
      tempIgnoreStart = millis();
      
      if (!temperatureLockEnabled) {
        if (temperatureDestination == TEMP_TARGET) {
#ifdef DEBUG_TUB
          Serial.print("HOTTUB->Changing target temp from ");
          Serial.print(targetState->targetTemperature);
#endif

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
#ifdef DEBUG_TUB          
          Serial.print(" to ");
          Serial.println(targetState->targetTemperature);
#endif
        }
      }
#ifdef DEBUG_TUB
      else {
        Serial.println("HOTTUB->Temperature is locked, keeping current target temperature");
      }
#endif
      
      break;
    case CMD_BTN_PUMP:
      if (currentState->pumpState == PUMP_FILTERING || currentState->pumpState == PUMP_HEATER_STANDBY || currentState->pumpState == PUMP_HEATING) {
        manuallyTurnedOff = true;
      }
      break;
  }

  lastButtonPressTime = millis();
}

void HotTub::decodeStatus(unsigned int command) {
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
      if (temperatureDestination == TEMP_CURRENT) {
        temperatureDestination = TEMP_PREP_TARGET;
        tempIgnoreStart = millis();
      }
      break;
  }

  if (targetState->pumpState == PUMP_UNKNOWN) {
    targetState->pumpState = newState;
    stateChanged();
  }

  if (currentState->pumpState != newState) {
    currentState->pumpState = newState;
    currentState->errorCode = 0;
    stateChanged();
  }
}

void HotTub::decodeError(unsigned int command) {
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
      currentState->pumpState = PUMP_ERROR;
      currentState->errorCode = errorCode;
      stateChanged();
    }
  }
}

int HotTub::decodeTemperature(unsigned int command) {
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
#ifdef DEBUG_TUB
    Serial.println("HOTTUB->Auto restarting...");
#endif
    setTargetState(PUMP_HEATING);
  }
}

void HotTub::targetTemperatureCheck(){
  if (millis() - TEMP_BUTTON_DELAY < lastButtonPressTime && lastButtonPressTime > 0)
    return; //leave temperatures alone until buttons aren't being pressed

  //if the target temperature is 0 we need to send temperature button press so that the display blinks and shows us the target temperature
  if (currentState->targetTemperature == 0 && millis() > TEMP_TARGET_INITIAL_DELAY) {
#ifdef DEBUG_TUB
    Serial.print("HOTTUB->Getting current target temperature...");
#endif
    queueCommand(CMD_BTN_TEMP_DN);
    lastButtonPressTime = millis();
  }

  #ifndef DISABLE_TARGET_TEMPERATURE 
  if (currentState->targetTemperature > 0
    && currentState->targetTemperature != targetState->targetTemperature)
  {
    if (currentState->targetTemperature < targetState->targetTemperature) {
#ifdef DEBUG_TUB
        Serial.println("HOTTUB->Auto-Adjusting target temperature, up");
#endif
        queueCommand(CMD_BTN_TEMP_UP);
    }
        
    if (currentState->targetTemperature > targetState->targetTemperature) {
#ifdef DEBUG_TUB
        Serial.println("HOTTUB->Auto-Adjusting target temperature, down");
#endif
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

void HotTub::setup(void (&onStateChange)()) {
  stateChanged = onStateChange;
}

long lastPrintTime; 
void HotTub::loop() {
  SendReceive::loop();
  
  //dont send if we've recently sent, or never sent
  if (millis() - WAIT_BETWEEN_SENDING_AUTO_COMMANDS < getLastCommandQueuedTime() && getLastCommandQueuedTime() > 0) {
    if (millis() - DELAY_BETWEEN_PRINTS > lastPrintTime) {
      lastPrintTime = millis();
#ifdef DEBUG_TUB
      Serial.println("HOTTUB->Waiting for auto send delay");
#endif
    }
    return;
  }

  //dont send anything unless we have waited the delay between commands or if the queue is full
  if (getCommandQueueCount() == MAX_OUT_COMMANDS) {
    if (millis() - DELAY_BETWEEN_PRINTS > lastPrintTime) {
      lastPrintTime = millis();
#ifdef DEBUG_TUB
      Serial.println("HOTTUB->Command queue maxed out!");
#endif
    }
    return;
  }

  //autoRestartCheck();
  targetTemperatureCheck();
  //targetStateCheck();
}
