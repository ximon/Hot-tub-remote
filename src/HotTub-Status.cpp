#include "HotTub.h"

unsigned long flashStartTime;

void HotTub::handleReceivedStatus(unsigned int command)
{
    int decodedState = decodeStatus(command);

#ifdef DEBUG_TUB_VERBOSE
    logger->logf("HOTTUB->Decoding status command, state is %s", stateToString(currentState->pumpState));
#endif

    if (decodedState == PUMP_UNKNOWN)
        return;

    //if we dont yet have a target state, just pick up the current state as the target.
    if (targetState->pumpState == PUMP_UNKNOWN)
    {
        targetState->pumpState = decodedState;
        stateChanged();
    }

    if (currentState->pumpState != decodedState)
    {
        currentState->pumpState = decodedState;
        currentState->errorCode = 0;
        stateChanged();
    }
}

void HotTub::handleReceivedError(unsigned int command)
{
    int errorCode = decodeError(command);

    if (errorCode == 0)
        return;

    if (currentState->pumpState == PUMP_ERROR)
        return;

    currentState->pumpState = PUMP_ERROR;
    currentState->errorCode = errorCode;
    stateChanged();

#ifdef DEBUG_TUB
    logget->logf("HOTTUB->Error code %i - %s", currentState->errorCode, errorToString(currentState->errorCode));
#endif
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
#ifdef DEBUG_TUB
        logger->logf("HOTTUB->Changing state from %s to %s", stateToString(targetState->pumpState), stateToString(newState));
#endif

        targetState->pumpState = newState;
        stateChanged();
    }
}

void HotTub::setAutoRestart(bool enable)
{
    autoRestartEnabled = enable;
    stateChanged();
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
    currentJson["flashing"] = currentState->flashing;

    TargetState *targetState = getTargetState();
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