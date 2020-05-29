#include "HotTub.h"

#define DEBUG_TUB

void HotTub::handleReceivedButton(unsigned int command)
{
#ifdef DEBUG_TUB
    Serial.print("HOTTUB->Decoding button command,");
    Serial.print(buttonToString(command));
    Serial.println(" button was pressed");
#endif

    switch (command)
    {
    case CMD_BTN_TEMP_UP:
    case CMD_BTN_TEMP_DN:
    {
        handleTempButtonPress(command);
        break;
    }
    case CMD_BTN_PUMP:

        switch (currentState->pumpState)
        {
        case PUMP_HEATING:
            //set some delay to 5secs to prevent sending commands
        case PUMP_BUBBLES:
        case PUMP_FILTERING:
        case PUMP_HEATER_STANDBY:
            targetState->pumpState = PUMP_OFF;
            break;
        case PUMP_OFF:
            targetState->pumpState = PUMP_FILTERING;
        }

        break;
    case CMD_BTN_HEAT:

        switch (currentState->pumpState)
        {
        case PUMP_OFF:
        case PUMP_FILTERING:
        case PUMP_BUBBLES:
            targetState->pumpState = PUMP_HEATING;
            break;

        case PUMP_HEATING:
        case PUMP_HEATER_STANDBY:
            targetState->pumpState = PUMP_FILTERING;
            break;
        }

        break;
    case CMD_BTN_BLOW:
        //todo - configurable state after blower is on
        targetState->pumpState =
            currentState->pumpState == PUMP_BUBBLES
                ? PUMP_HEATING
                : PUMP_BUBBLES;
    }

    lastButtonPressTime = millis();
}

void HotTub::handleTempButtonPress(unsigned int command)
{
    //if its not flashing then the temperature won't change
    if (tubMode != TM_FLASHING && tubMode != TM_FLASH_DETECTED)
    {
        Serial.print("HOTTUB->Ignoring, TubMode was ");
        Serial.println(tubModeToString(tubMode));
        return;
    }

    if (temperatureLockEnabled)
    {
#ifdef DEBUG_TUB
        Serial.println("HOTTUB->Temperature is locked, keeping current target temperature");
#endif
        return;
    }

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

    //tubMode = TM_TEMP_MANUAL_CHANGE;
    setTargetTemperature(newTargetTemp);

#ifdef DEBUG_TUB
    Serial.print(" to ");
    Serial.println(targetState->targetTemperature);
#endif
}