#include "HotTubApi.h"

/*
 * Helper functions
 */

String HotTubApi::toOnOff(bool value)
{
    return value ? "on" : "off";
}

void HotTubApi::valueNotProvided(String argumentName)
{
    String output = "No " + argumentName + " provided";
    errorMessage(output);
#ifdef DEBUG_API
    Serial.println(output);
#endif
}

bool HotTubApi::checkArgument(String argumentName)
{
    if (!server->hasArg(argumentName))
    {
        valueNotProvided(argumentName);
        return true;
    }

    return false;
}

bool HotTubApi::valueIsTrue(String argumentName)
{
    String value = server->arg(argumentName);
    value.trim();
    value.toLowerCase();

    return strcmp(value.c_str(), "true") == 0;
}
