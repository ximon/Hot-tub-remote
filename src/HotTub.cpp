#include "HotTub.h"
#include "Commands.h"
#include "Temperatures.h"
#include "SendReceive.h"

#include "syslog.h"

//todo - extract Serial.print lines to a debug function that can be handled externally.

//#define DEBUG_TUB

HotTub::HotTub(int dataInPin, int dataOutPin, int debugPin)
    : SendReceive(dataInPin, dataOutPin, debugPin)
{
  currentState = new CurrentState();
  currentState->pumpState = PUMP_UNKNOWN;
  currentState->targetTemperature = INIT_TEMP;

  targetState = new TargetState();
  targetState->pumpState = PUMP_UNKNOWN;
  targetState->targetTemperature = INIT_TEMP;
}

void HotTub::onCommandSent(unsigned int command)
{
#ifdef DEBUG_TUB
//  Serial.println("HOTTUB->Command Sent!");
#endif

  if (command == CMD_BTN_TEMP_UP || command == CMD_BTN_TEMP_DN)
  {
#ifdef DEBUG_TUB
    debug("HOTTUB->Next temperature reading will be for the target temperature");
#endif
    tempButtonPressed();
  }
}

void HotTub::onCommandReceived(unsigned int command)
{
#ifdef DEBUG_TUB_VERBOSE
  debugf("HOTTUB->Handling command 0x%X", command);
#endif

  if (command == 0)
  {
    //#ifdef DEBUG_TUB
    debugf("HOTTUB->Invalid command received: 0x%X", command);
    //#endif
    return;
  }

  if ((command >= TEMP_104F && command <= TEMP_9C) || command == CMD_FLASH || command == CMD_BTN_TEMP_UP || command == CMD_BTN_TEMP_DN)
  {
    handleReceivedTemperature(command);

    //we need the buttons to get picked up by other checks
    if (command >= TEMP_104F && command <= TEMP_9C)
      return;
  }

  if (command >= CMD_BTN_HEAT && command <= CMD_BTN_PUMP)
  {
    handleReceivedButton(command);
    return;
  }

  if (command >= CMD_STATE_ALL_OFF_F && command <= CMD_STATE_BLOWER_ON_C)
  {
    handleReceivedStatus(command);
    return;
  }

  if ((command >= CMD_ERROR_5 && command <= CMD_ERROR_1) || command == CMD_ERROR_PKT1 || command == CMD_ERROR_PKT2)
  {
    handleReceivedError(command);
    return;
  }
}

/*
 * Target state checks
 */

//todo - this needs moving into the command receiver
void HotTub::autoRestartCheck()
{
  if (!autoRestartEnabled)
    return;

  if (currentState->pumpState == PUMP_OFF && targetState->pumpState != PUMP_OFF)
  {
    //#ifdef DEBUG_TUB
    debug("HOTTUB->Auto restarting...");
    //#endif
    currentState->autoRestartCount += 1;
    stateChanged("Auto restarted");
  }
}

void HotTub::targetTemperatureCheck()
{
  //todo - this may be redundant now, due to now detecting the display flashing.
  if (millis() - TEMP_BUTTON_DELAY < lastButtonPressTime && lastButtonPressTime > 0)
    return; //leave temperatures alone until buttons aren't being pressed

  if (currentState->targetTemperature != INIT_TEMP)
    return;

  //if the target temperature is 0 we need to send temperature button press so that the display blinks and shows us the target temperature
  if (millis() > TEMP_TARGET_INITIAL_DELAY)
  {
    debug("HOTTUB->Getting current target temperature...");

    queueCommand(CMD_BTN_TEMP_DN); //send a temp down command to get the display to flash the target temp
    lastButtonPressTime = millis();
    return;
  }

  //currentState->targetTemperature is 0 when we've just powered on
  if (currentState->targetTemperature == INIT_TEMP)
    return;

  if (currentState->targetTemperature == targetState->targetTemperature)
    return;

  if (tubMode != TM_NORMAL)
    return;

  if (currentState->targetTemperature != targetState->targetTemperature)
    adjustTemperatures();
}

void HotTub::adjustTemperatures()
{
  int current = currentState->targetTemperature;
  int target = targetState->targetTemperature;

  //we dont adjust the temperature if its flashing.
  //if we're not currently flashing we'll have to send an extra button to make the display flash
  int commandCount = 1 + (current < target ? target - current : current - target);
  unsigned int command = current < target ? CMD_BTN_TEMP_UP : CMD_BTN_TEMP_DN;
  const char *direction = current < target ? "increasing" : "decreasing";

  debugf("HOTTUB-> Auto %s target temperature", direction);
  queueCommands(command, commandCount);
  lastButtonPressTime = millis();
}

void HotTub::targetStateCheck()
{
  int current = currentState->pumpState;
  int target = targetState->pumpState;

  //no need to do anything if the state matches
  if (current == target)
    return;

  if (current == PUMP_HEATER_STANDBY && target == PUMP_HEATING)
    return;

  //if we've recently sent a command, try later
  if (millis() - BUTTON_SEND_DELAY < getLastCommandSentTime())
    return;

  debugf("HOTTUB->Changing from '%s' to '%s'", stateToString(current), stateToString(target));

  //if we are trying to turn everything off and the pump is in any state other than off we can just send the pump button to switch it off
  //todo - might need a way to check this later on as turning the pump off takes a few seconds after turning the heater off,
  // i think the delay is to flush cool water through the heater
  if (target == PUMP_OFF)
    queueCommand(CMD_BTN_PUMP);

  //if we only want to filter the water
  if (target == PUMP_FILTERING)
  {
    if (current == PUMP_HEATING || current == PUMP_HEATER_STANDBY)
      queueCommand(CMD_BTN_HEAT); //turn off the heat, pump goes to filtering automatically

    if (current == PUMP_OFF)
      queueCommand(CMD_BTN_PUMP); //turn on the pump

    if (current == PUMP_BUBBLES)
      queueCommand(CMD_BTN_PUMP);
  }

  //if we want to heat the water
  if (target == PUMP_HEATING)
  {
    //if we want to heat the water and its not current heating or in heater standby send heat button
    if (current != PUMP_HEATING && current != PUMP_HEATER_STANDBY)
      queueCommand(CMD_BTN_HEAT);
  }

  if (target == PUMP_BUBBLES)
  {
    queueCommand(CMD_BTN_BLOW);
  }
}

/*
 * Setup and loop
 */

void HotTub::setup(
    void (&onSetDataInterrupt)(bool state),
    void (&onStateChange)(char *reason),
    void (&onDebug)(char *message),
    void (&onDebugf)(char *fmt, ...))
{
  setDataInterrupt = onSetDataInterrupt;
  stateChanged = onStateChange;
  debug = onDebug;
  debugf = onDebugf;
  SendReceive::setup(onSetDataInterrupt, onDebug, onDebugf);
}

unsigned long lastPrintTime;
void HotTub::loop()
{
  SendReceive::loop();

  //dont send if we've recently sent, or never sent
  if (millis() - WAIT_BETWEEN_SENDING_AUTO_COMMANDS < getLastCommandQueuedTime() && getLastCommandQueuedTime() > 0)
  {
    if (millis() - DELAY_BETWEEN_PRINTS > lastPrintTime)
    {
      lastPrintTime = millis();
      //#ifdef DEBUG_TUB
      debug("HOTTUB->Waiting for auto send delay");
      //#endif
    }
    return;
  }

  //dont send anything unless we have waited the delay between commands or if the queue is full
  if (getCommandQueueCount() == MAX_OUT_COMMANDS)
  {
    if (millis() - DELAY_BETWEEN_PRINTS > lastPrintTime)
    {
      lastPrintTime = millis();
      //#ifdef DEBUG_TUB
      debug("HOTTUB->Command queue maxed out!");
      //#endif
    }
    return;
  }

  autoRestartCheck();
  targetTemperatureCheck();
  targetStateCheck();
}
