#include "HotTub.h"

unsigned long flashStartTime;

void HotTub::handleReceivedStatus(unsigned int command)
{
    int decodedState = decodeStatus(command);

#ifdef DEBUG_TUB_VERBOSE
    debugf("HOTTUB-> Received status command '%s' current state is '%s'", stateToString(decodedState), stateToString(currentState->pumpState));
#endif

    if (decodedState == PUMP_UNKNOWN)
        return;

    //if we dont yet have a target state, just pick up the current state as the target.
    if (targetState->pumpState == PUMP_UNKNOWN)
    {
        if (decodedState == PUMP_HEATER_STANDBY)
            decodedState = PUMP_HEATING;

        setTargetState(decodedState);
    }

    if (currentState->pumpState != decodedState)
    {
        debugf("HOTTUB->Received new status command '%s', old state was '%s'", stateToString(decodedState), stateToString(currentState->pumpState));

        currentState->pumpState = decodedState;
        currentState->errorCode = 0;
        stateChanged("Current State changed");
    }
}

void HotTub::handleReceivedError(unsigned int command)
{
    //debug("HOTTUB->Decoding error...");

    int errorCode = decodeError(command);

    if (errorCode == 0)
        return;

    //debugf("HOTTUB->Decoded error, error: '%i'", errorCode);

    if (currentState->pumpState == PUMP_ERROR && currentState->errorCode == errorCode)
        return;

    //#ifdef DEBUG_TUB
    //debugf("HOTTUB->Error code %d received - %s", currentState->errorCode, errorToString(currentState->errorCode));
    //#endif

    currentState->pumpState = PUMP_ERROR;
    currentState->errorCode = errorCode;
    stateChanged("Error received");
}

int HotTub::getErrorCode()
{
    return currentState->errorCode;
}

//todo - change this to return a copy of the array as we don't want external code changing its state!
CurrentState *HotTub::getCurrentState()
{
    return currentState;
}

TargetState *HotTub::getTargetState()
{
    return targetState;
}

void HotTub::setTargetState(int newState)
{
    if (targetState->pumpState != newState)
    {
        //#ifdef DEBUG_TUB
        debugf("HOTTUB->Changing target state from '%s' to '%s'", stateToString(targetState->pumpState), stateToString(newState));
        //#endif

        targetState->pumpState = newState;
        stateChanged("Target State Changed");
    }
}

void HotTub::setAutoRestart(bool enable)
{
    autoRestartEnabled = enable;
    stateChanged("Auto restart changed");
}

bool HotTub::getAutoRestart()
{
    return autoRestartEnabled;
}

char *HotTub::getStateJson()
{
    const size_t capacity = 2 * JSON_OBJECT_SIZE(7) + JSON_OBJECT_SIZE(6);
    DynamicJsonDocument doc(capacity);
    char *json = new char[300];

    CurrentState *currentState = getCurrentState();
    JsonObject currentJson = doc.createNestedObject("currentState");
    currentJson["pumpState"] = currentState->pumpState;
    currentJson["temperature"] = currentState->temperature;
    currentJson["targetTemperature"] = currentState->targetTemperature;
    currentJson["tubMode"] = tubMode;

    TargetState *targetState = getTargetState();
    JsonObject targetJson = doc.createNestedObject("targetState");
    targetJson["pumpState"] = targetState->pumpState;
    targetJson["targetTemperature"] = targetState->targetTemperature;

    doc["autoRestart"] = autoRestartEnabled;
    doc["autoRestartCount"] = currentState->autoRestartCount;
    doc["limitTemperature"] = getLimitTemperature();
    doc["temperatureLock"] = temperatureLockEnabled;
    doc["errorCode"] = currentState->errorCode;

    serializeJson(doc, json, 300);
    return json;
}