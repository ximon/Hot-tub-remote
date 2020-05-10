#include "HotTub.h"

void HotTub::handleReceivedTemperature(unsigned int command)
{
    int temperature = decodeTemperature(command);

#ifdef DEBUG_TUB_VERBOSE
    Serial.print("HOTTUB->Decoding temperature command, ");
    Serial.print("received ");
    Serial.println(temperature);
#endif

    switch (temperatureDestination)
    {
    case TEMP_PREP_CURRENT:
        if (millis() - TEMP_CURRENT_DELAY > tempIgnoreStart || !currentState->flashing)
        {
            temperatureDestination = TEMP_CURRENT;
#ifdef DEBUG_TUB_VERBOSE
            Serial.println("HOTTUB->Next reading will be for the current temperature");
#endif
        }
        else
        {
#ifdef DEBUG_TUB_VERBOSE
            Serial.println("HOTTUB->Ignoring Current Temperature reading...");
#endif
        }
        break;

    case TEMP_CURRENT:
        if (currentState->temperature != temperature)
        {
            currentState->temperature = temperature;
            stateChanged();
        }
#ifdef DEBUG_TUB_VERBOSE
        Serial.print("HOTTUB->Temperature is ");
        Serial.println(currentState->temperature);
#endif
        break;

    case TEMP_PREP_TARGET:
        if (millis() - TEMP_TARGET_DELAY > tempIgnoreStart)
        {
#ifdef DEBUG_TUB
            Serial.println("HOTTUB->Next reading will be for the Target Temperature");
#endif
            temperatureDestination = TEMP_TARGET;
        }
#ifdef DEBUG_TUB
        else
        {
            Serial.println("HOTTUB->Ignoring Target Temperature reading...");
        }
#endif
        break;
    case TEMP_TARGET:
        if (currentState->targetTemperature != temperature)
        {
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