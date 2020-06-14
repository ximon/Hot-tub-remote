#include "HotTub.h"
#include "Commands.h"
#include "Temperatures.h"
#include "SendReceive.h"

#include <SysLog.h>

#define DEBUG_TUB

HotTub::HotTub(int dataInPin, int dataOutPin, int debugPin, Syslog *syslog)
    : SendReceive(dataInPin, dataOutPin, debugPin, syslog),
      logger(syslog)
{
  currentState = new CurrentState();
  currentState->pumpState = PUMP_UNKNOWN;

  targetState = new TargetState();
  targetState->pumpState = PUMP_UNKNOWN;
  targetState->targetTemperature = INIT_TEMP;
}

void HotTub::onCommandSent(unsigned int command)
{
#ifdef DEBUG_TUB
  logger->logf("HOTTUB->Sent Command 0x%X", command);
#endif

  if (command == CMD_BTN_TEMP_UP || command == CMD_BTN_TEMP_DN)
  {
#ifdef DEBUG_TUB
    logger->log("HOTTUB->Next temperature reading will be for the target temperature");
#endif
    tempButtonPressed();
  }
}

void HotTub::onCommandReceived(unsigned int command)
{
#ifdef DEBUG_TUB_VERBOSE
  logger->logf("HOTTUB->Handling command 0x%X", command);
#endif

  if ((command >= TEMP_104F && command <= TEMP_9C) || command == CMD_FLASH || command == CMD_BTN_TEMP_UP || command == CMD_BTN_TEMP_DN)
  {
    handleReceivedTemperature(command);
    //no return, we need the buttons to get picked up by other checks
    if (command >= TEMP_104F && command <= TEMP_9C)
      return;
  }

  if (command >= CMD_BTN_HEAT && command <= CMD_BTN_PUMP)
  {
    handleReceivedButton(command);
    return;
  }

  if ((command >= CMD_STATE_ALL_OFF_F && command <= CMD_STATE_BLOWER_ON_C) || command == CMD_END)
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

void HotTub::targetTemperatureCheck()
{
  if (millis() - TEMP_BUTTON_DELAY < lastButtonPressTime && lastButtonPressTime > 0)
    return; //leave temperatures alone until buttons aren't being pressed

  //if the target temperature is 0 we need to send temperature button press so that the display blinks and shows us the target temperature
  if (currentState->targetTemperature == 0 && millis() > TEMP_TARGET_INITIAL_DELAY)
  {
#ifdef DEBUG_TUB
    logger->log("HOTTUB->Getting current target temperature...");
#endif
    queueCommand(CMD_BTN_TEMP_DN); //send a temp down command to get the display to flash the target temp
    lastButtonPressTime = millis();
    return;
  }

  //currentState->targetTemperature is 0 when we've just powered on
  if (currentState->targetTemperature == 0)
    return;

  if (currentState->targetTemperature == targetState->targetTemperature)
    return;

  if (tubMode != TM_NORMAL)
  {
    logger->logf("HOTTUB->Not changing temperature, TubMode is %s", tubModeToString(tubMode));
    return;
  }

  adjustTemperatures();
}

void HotTub::adjustTemperatures()
{
  if (currentState->targetTemperature < targetState->targetTemperature)
  {
#ifdef DEBUG_TUB
    logger->log("HOTTUB->Auto-Adjusting target temperature, up");
#endif
    int commandsToSend = targetState->targetTemperature - currentState->targetTemperature;

    //if we're not currently flashing we'll have to send an extra button to make the display flash
    queueCommands(CMD_BTN_TEMP_UP, commandsToSend + 1);
    lastButtonPressTime = millis();
  }

  if (currentState->targetTemperature > targetState->targetTemperature)
  {
#ifdef DEBUG_TUB
    logger->log("HOTTUB->Auto-Adjusting target temperature, down");
#endif
    int commandsToSend = currentState->targetTemperature - targetState->targetTemperature;

    //if we're not currently flashing we'll have to send an extra button to make the display flash
    queueCommands(CMD_BTN_TEMP_DN, commandsToSend + 1);

    lastButtonPressTime = millis();
  }
}

void HotTub::targetStateCheck()
{
  int stateToChangeTo = targetState->pumpState;

  //no need to do anything if the state matches
  if (currentState->pumpState == stateToChangeTo)
    return;

  //if we've recently sent a command, try later
  if (millis() - BUTTON_SEND_DELAY < getLastCommandSentTime())
    return;

  //if we are trying to turn everything off and the pump is in any state other than off we can just send the pump button to switch it off
  //todo - might need a way to check this later on as turning the pump off takes a few seconds after turning the heater off,
  // i think the delay is to flush cool water through the heater
  if (stateToChangeTo == PUMP_OFF)
  {
    queueCommand(CMD_BTN_PUMP);
    return;
  }

  //if we only want to filter the water
  if (stateToChangeTo == PUMP_FILTERING)
  {
    if (currentState->pumpState == PUMP_HEATING || currentState->pumpState == PUMP_HEATER_STANDBY)
      queueCommand(CMD_BTN_HEAT); //to turn off the heat

    if (currentState->pumpState == PUMP_OFF)
      queueCommand(CMD_BTN_PUMP); //to turn on the pump

    return;
  }

  //if we want to heat the water
  if (stateToChangeTo == PUMP_HEATING || stateToChangeTo == PUMP_HEATER_STANDBY)
  {
    //first we might need to turn off the blower
    if (currentState->pumpState == PUMP_BUBBLES)
      queueCommand(CMD_BTN_BLOW);

    if (manuallyTurnedOff)
      return;

    //if we want to heat the water and its not current heating or in heater standby send heat button
    if (currentState->pumpState != PUMP_HEATING && currentState->pumpState != PUMP_HEATER_STANDBY)
    {
      if (targetState->pumpState == PUMP_HEATING)
        queueCommand(CMD_BTN_HEAT);
    }

    return;
  }

  if (stateToChangeTo == PUMP_BUBBLES)
  {
    queueCommand(CMD_BTN_BLOW);
    return;
  }
}

/*
 * Setup and loop
 */

void HotTub::setup(void (&onStateChange)(const char *reason))
{
  stateChanged = onStateChange;
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
#ifdef DEBUG_TUB
      logger->log("HOTTUB->Waiting for auto send delay");
#endif
    }
    return;
  }

  //dont send anything unless we have waited the delay between commands or if the queue is full
  if (getCommandQueueCount() == MAX_OUT_COMMANDS)
  {
    if (millis() - DELAY_BETWEEN_PRINTS > lastPrintTime)
    {
      lastPrintTime = millis();
#ifdef DEBUG_TUB
      logger->log("HOTTUB->Command queue maxed out!");
#endif
    }
    return;
  }

  targetTemperatureCheck();
  targetStateCheck();
}
