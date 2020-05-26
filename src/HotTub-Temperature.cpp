#include "HotTub.h"

#define TEMPS_BETWEEN_FLASHES 4

int nextFlashCountdown = 0;
int lastFlashingTemperatureReading;

uint lastCommand;

void HotTub::handleReceivedTemperature(unsigned int command)
{
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

    if (command == lastCommand)
        return;

    lastCommand = command;

    //if a temp button is pressed
    if (command == CMD_BTN_TEMP_DN || command == CMD_BTN_TEMP_UP)
    {
        //ignore all temperature readings until a flash is detected, or timeout
        tempButtonPressed();
        return;
    }

    if (tubMode == TM_TEMP_BUTTON_DETECTED)
    {
        nextFlashCountdown -= nextFlashCountdown > 0 ? 1 : 0;

        if (nextFlashCountdown == 0)
            tubMode = TM_NORMAL;

        return;
    }

    if (tubMode == TM_FLASHING || tubMode == TM_FLASH_DETECTED)
    {
        lastFlashingTemperatureReading = newTemp;
        return;
    }

    //its a temperature command
    lastFlashingTemperatureReading = 0;
    updateCurrentTemperature(newTemp);

#ifdef DEBUG_TUB_VERBOSE
    Serial.print("HOTTUB->Decoding temperature command, ");
    Serial.print("received ");
    Serial.println(temperature);
#endif
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

    Serial.print("HOTTUB->Setting current temperature to ");
    Serial.println(temperature);

    currentState->temperature = temperature;
    stateChanged();
}

void HotTub::updateTargetTemperature(int temperature)
{
    if (temperature <= 0)
        return;
    if (currentState->targetTemperature == temperature)
        return;

    Serial.print("HOTTUB->Setting target temperature to ");
    Serial.println(temperature);

    currentState->targetTemperature = temperature;
    stateChanged();
}

int HotTub::setTargetTemperature(int temp)
{
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

int HotTub::getLimitTemperature()
{
    return limitTemperature;
}

int HotTub::setLimitTemperature(int temp)
{
    int status = maxTemperatureValid(temp);

    if (status != 0)
        return 0;

#ifdef DEBUG_TUB
    Serial.print("HOTTUB->Setting limit temperature to ");
    Serial.println(temp);
#endif

    limitTemperature = temp;

    if (targetState->targetTemperature > temp)
    {
        targetState->targetTemperature = temp;
    }

    stateChanged();

    return status;
}

void HotTub::setTemperatureLock(bool enable)
{
    temperatureLockEnabled = enable;
    stateChanged();
}

int HotTub::maxTemperatureValid(int maxTemp)
{
    return tempValid(maxTemp, MIN_TEMP, MAX_TEMP);
}

int HotTub::targetTemperatureValid(int targetTemp)
{
    return tempValid(targetTemp, MIN_TEMP, limitTemperature);
}

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