#include "HotTub.h"

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
        //if its not flashing then the temperature won't change
        if (tubMode != TM_FLASHING && tubMode != TM_FLASH_DETECTED)
            return;

        if (temperatureLockEnabled)
        {
            Serial.println("HOTTUB->Temperature is locked, keeping current target temperature");
            break;
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
        break;
    }
    case CMD_BTN_PUMP:
        if (currentState->pumpState == PUMP_FILTERING || currentState->pumpState == PUMP_HEATER_STANDBY || currentState->pumpState == PUMP_HEATING)
        {
            //currentState = PUMP_OFF
            //targetState = PUMP_OFF;
            //set some delay to 5secs to prevent sending commands
            //todo - this should really just set the target state to PUMP_OFF and start the pumpOffDelay of 5 seconds
            manuallyTurnedOff = true;
        }
        break;
    }

    lastButtonPressTime = millis();
}