#include "HotTub.h"

#define TEMPS_BETWEEN_FLASHES 4

#define DEBUG_TUB

int nextFlashCountdown = 0;
int lastFlashingTemperatureReading;

uint lastCommand;

void HotTub::handleReceivedTemperature(unsigned int command)
{
    int originalTubMode = tubMode;
    int newTemp = decodeTemperature(command);

    nextFlashCountdown -= nextFlashCountdown > 0 ? 1 : 0; //decrement flash timeout counter;

    if (tubMode == TM_FLASHING && nextFlashCountdown == 0)
        tubMode = TM_NORMAL;

    if (tubMode == TM_NORMAL && newTemp > 0)
        updateCurrentTemperature(newTemp);

    if (command == CMD_FLASH)
    {
        //reset countdown
        nextFlashCountdown = TEMPS_BETWEEN_FLASHES + 1;

        //if we're flashing
        tubMode = tubMode == TM_FLASH_DETECTED || tubMode == TM_FLASHING
                      ? TM_FLASHING
                      : TM_FLASH_DETECTED;

        if (tubMode == TM_FLASHING)
            updateTargetTemperature(lastFlashingTemperatureReading);
    }
    /*
    if (command == lastCommand)
    {
        if (tubMode != originalTubMode)
            stateChanged("Tub Mode changed");
        return;
    }
    */
    lastCommand = command;

    //if a temp button is pressed
    if ((command == CMD_BTN_TEMP_DN || command == CMD_BTN_TEMP_UP) && tubMode == TM_NORMAL)
    {
        //ignore all temperature readings until a flash is detected, or timeout
        tempButtonPressed();
        return;
    }

    if (tubMode == TM_TEMP_BUTTON_DETECTED)
    {
        nextFlashCountdown -= nextFlashCountdown > 0 ? 1 : 0;

        if (nextFlashCountdown == 0)
        {
            tubMode = TM_NORMAL;
            //stateChanged("Tub mode changed");
        }

        return;
    }

    if (tubMode == TM_FLASHING || tubMode == TM_FLASH_DETECTED)
    {
        lastFlashingTemperatureReading = newTemp;
        //stateChanged("Flashing changed");
        return;
    }

    //its a temperature command
    lastFlashingTemperatureReading = 0;
    updateCurrentTemperature(newTemp);
}

void HotTub::tempButtonPressed()
{
    tubMode = TM_TEMP_BUTTON_DETECTED;
    nextFlashCountdown = TEMPS_BETWEEN_FLASHES + 1;
}

void HotTub::updateCurrentTemperature(int temperature)
{
    if (temperature <= 0)
        return;

    if (currentState->temperature == temperature)
        return;

#ifdef DEBUG_TUB
    debugf("HOTTUB->Changing current temperature from '%i' to '%i'", currentState->temperature, temperature);
#endif

    currentState->temperature = temperature;
    stateChanged("Current temperature changed");
}

//Set the current target temperature, i.e. the one the tub is displaying
void HotTub::updateTargetTemperature(int temperature)
{
    if (temperature <= 0)
        return;

    if (currentState->targetTemperature == temperature)
        return;

    if (targetState->targetTemperature == INIT_TEMP)
    {
#ifdef DEBUG_TUB
        debugf("HOTTUB->Changing target target temperature from '%i' to '%i'", targetState->targetTemperature, temperature);
#endif
        targetState->targetTemperature = temperature;
    }

#ifdef DEBUG_TUB
    debugf("HOTTUB->Changing current target temperature from '%i' to '%i'", currentState->targetTemperature, temperature);
#endif

    currentState->targetTemperature = temperature;
    stateChanged("Current Target temperature changed");
}

//sets the target state's target temperature, the one to aim to set the tub to
int HotTub::setTargetTemperature(int temperature)
{
    if (temperature == targetState->targetTemperature)
        return 0;

    int status = targetTemperatureValid(temperature);

    if (status != 0)
        return 0;

#ifdef DEBUG_TUB
    debugf("HOTTUB->Changing target target temperature from '%i' to '%i'", targetState->targetTemperature, temperature);
#endif

    targetState->targetTemperature = temperature;
    stateChanged("Target Target temperature changed");

    return status;
}

int HotTub::getLimitTemperature()
{
    return limitTemperature;
}

int HotTub::setLimitTemperature(int temperature)
{
    if (temperature == limitTemperature)
        return 0;

    int status = maxTemperatureValid(temperature);

    if (status != 0)
        return 0;

#ifdef DEBUG_TUB
    debugf("HOTTUB->Changing limit temperature from '%i' to '%i'", limitTemperature, temperature);
#endif

    limitTemperature = temperature;

    if (targetState->targetTemperature > temperature)
        targetState->targetTemperature = temperature;

    stateChanged("Temperature limit changed");

    return status;
}

void HotTub::setTemperatureLock(bool enable)
{
    temperatureLockEnabled = enable;
    stateChanged("Temperature lock changed");
}

int HotTub::maxTemperatureValid(int maxTemp)
{
    return tempValid(maxTemp, MIN_TEMP, MAX_TEMP);
}

//returns 0 is the supplied target temperature is within the ranges
int HotTub::targetTemperatureValid(int targetTemp)
{
    return tempValid(targetTemp, MIN_TEMP, limitTemperature);
}

//returns 0 is the supplied temperature is within the ranges
int HotTub::tempValid(int temperature, int minTemp, int maxTemp)
{
    bool aboveMax = temperature > maxTemp;
    bool belowMin = temperature < minTemp;

    if (aboveMax)
        return 1;

    if (belowMin)
        return -1;

    return 0;
}