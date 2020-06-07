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

/*
 * Http Helper functions
 */

void HotTubApi::addCorsHeaders()
{
    server->sendHeader("Access-Control-Allow-Origin", "*");
}

//passes "Done" to be wrapped in a json object
void HotTubApi::okMessage()
{
    okJson(toJson("Done"));
}

//pass in a message to be wrapped in a json object
void HotTubApi::okMessage(String message)
{
    okJson(toJson(message));
}

void HotTubApi::okJson(char *json)
{
    addCorsHeaders();
    server->send(200, "application/json", json);
}

//outputs the json
void HotTubApi::okJson(String json)
{
    addCorsHeaders();
    server->send(200, "application/json", json);
}

//outputs the plain text
void HotTubApi::okText(String message)
{
    addCorsHeaders();
    server->send(200, "text/plain", message);
}

//sends the error message wrapped in a json object
void HotTubApi::errorMessage(String message)
{
    addCorsHeaders();
    server->send(400, "application/json", toJson(message));
}

//wraps the provided message in a json object
String HotTubApi::toJson(String message)
{
    const size_t capacity = JSON_OBJECT_SIZE(5); //todo - 5? what size should this be? variable?
    StaticJsonDocument<capacity> doc;
    String json;

    doc["message"] = message;
    serializeJson(doc, json);
    return json;
}

void HotTubApi::setTempMessage(int temp, String tempType)
{
    String msg = "Setting " + tempType + " to " + String(temp);
    okMessage(msg);

#ifdef DEBUG_API
    Serial.println(msg);
#endif
}