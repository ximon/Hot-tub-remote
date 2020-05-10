#include "HotTubApi.h"
#include <ESP8266WebServer.h>

//#define DEBUG_API

HotTubApi::HotTubApi(ESP8266WebServer *webServer, HotTub *hotTubInstance)
    : server(webServer),
      hotTub(hotTubInstance)
{
}

void HotTubApi::setup()
{
  server->on("/status", HTTP_GET, std::bind(&HotTubApi::handle_status, this));
  server->on("/autoRestart", HTTP_POST, std::bind(&HotTubApi::handle_autoRestart, this));
  server->on("/pump", HTTP_POST, std::bind(&HotTubApi::handle_pump, this));
  server->on("/heater", HTTP_POST, std::bind(&HotTubApi::handle_heater, this));
  server->on("/blower", HTTP_POST, std::bind(&HotTubApi::handle_blower, this));
  server->on("/command", HTTP_POST, std::bind(&HotTubApi::handle_rawCommand, this));
  server->addHandler(new RouteHandler("/temperature/lock", HTTP_POST, std::bind(&HotTubApi::handle_temperatureLock, this)));
  server->addHandler(new RouteHandler("/temperature/target", HTTP_POST, std::bind(&HotTubApi::handle_temperature, this)));
  server->addHandler(new RouteHandler("/temperature/max", HTTP_POST, std::bind(&HotTubApi::handle_maxTemperature, this)));
}

/*
 * Request Handlers
 */

void HotTubApi::handle_status()
{
#ifdef DEBUG_API
  Serial.println("API->handle_status()");
#endif
  char *json = hotTub->getStateJson();
  okJson(json);
}

void HotTubApi::handle_pump()
{
#ifdef DEBUG_API
  Serial.println("API->handle_pump()");
#endif
  if (checkArgument("state"))
    return;

  bool enable = valueIsTrue("state");
  String msg = "Sending pump command";

  hotTub->setTargetState(enable ? PUMP_FILTERING : PUMP_OFF);

  okMessage(msg);

#ifdef DEBUG_API
  Serial.println(msg);
#endif
}

void HotTubApi::handle_heater()
{
#ifdef DEBUG_API
  Serial.println("API->handle_heater()");
#endif
  if (checkArgument("state"))
    return;

  bool enable = valueIsTrue("state");
  String msg = "Sending heater command";

  hotTub->setTargetState(enable ? PUMP_HEATING : PUMP_FILTERING);

  okMessage(msg);
#ifdef DEBUG_API
  Serial.println(msg);
#endif
}

void HotTubApi::handle_blower()
{
#ifdef DEBUG_API
  Serial.println("API->handle_blower()");
#endif

  if (checkArgument("state"))
    return;

  bool enable = valueIsTrue("state");
  String msg = "Sending blower command";

  hotTub->setTargetState(enable ? PUMP_BUBBLES : PUMP_HEATING); //todo - configurable state after blower turned off??

  okMessage(msg);
#ifdef DEBUG_API
  Serial.println(msg);
#endif
}

void HotTubApi::handle_temperature()
{
#ifdef DEBUG_API
  Serial.println("API->handle_temperature()");
#endif

  if (checkArgument("value"))
    return;

  int temp = server->arg("value").toInt();
  int tempValid = hotTub->setTargetTemperature(temp);

  if (tempValid < 0)
  {
    errorMessage("Target temperature below minimum of " + String(MIN_TEMP));
    return;
  }

  if (tempValid > 0)
  {
    errorMessage("Target temperature above maximum of " + String(hotTub->getLimitTemperature()));
    return;
  }

  setTempMessage(temp, "target temperature");
}

void HotTubApi::handle_maxTemperature()
{
#ifdef DEBUG_API
  Serial.println("API->handle_maxTemperature()");
#endif

  if (checkArgument("value"))
    return;

  int maxTemp = server->arg("value").toInt();
  int tempValid = hotTub->setLimitTemperature(maxTemp);

  if (tempValid < 0)
  {
    errorMessage("Max temperature below minimum of " + String(MIN_TEMP));
    return;
  }

  if (tempValid > 0)
  {
    errorMessage("Max temperature above maximum of " + String(MAX_TEMP));
    return;
  }

  setTempMessage(maxTemp, "limit temperature");
}

void HotTubApi::setTempMessage(int temp, String tempType)
{
  String msg = "Setting " + tempType + " to " + String(temp);
  okMessage(msg);

#ifdef DEBUG_API
  Serial.println(msg);
#endif
}

void HotTubApi::handle_rawCommand()
{
#ifdef DEBUG_API
  Serial.println("API->handle_rawCommand()");
#endif

  if (checkArgument("command"))
    return;

  String commandStr = server->arg("command");

  commandStr.toLowerCase();
  unsigned int command = strtol(commandStr.c_str(), 0, 16);

#ifdef DEBUG_API
  Serial.print("API->command = 0x");
  Serial.println(command, HEX);
#endif

  if (command == 0)
  {
    errorMessage("API->Commands must be in the format 0x1234");
    return;
  }
  else if (command >= 0x4000)
  {
    errorMessage("API->Commands must be less than 0x4000");
    return;
  }

  String msg;

  if (hotTub->queueCommand(command))
  {
    msg = "API->Sending command " + commandStr;
  }
  else
  {
    msg = "API->Too many commands in queue!";
  }
  okMessage(msg);
}

void HotTubApi::handle_autoRestart()
{
#ifdef DEBUG_API
  Serial.println("API->handle_autoRestart()");
#endif

  if (checkArgument("state"))
    return;

  bool enable = valueIsTrue("state");
  hotTub->setAutoRestart(enable);

  String msg = "Auto restart is " + toOnOff(enable);
  okMessage(msg);

#ifdef DEBUG_API
  Serial.println(msg);
#endif
}

void HotTubApi::handle_temperatureLock()
{
#ifdef DEBUG_API
  Serial.println("API->handle_temperatureLock()");
#endif

  if (checkArgument("state"))
    return;

  bool enable = valueIsTrue("state");
  hotTub->setTemperatureLock(enable);

  String msg = "Temperature lock is " + toOnOff(enable);
  okMessage(msg);

#ifdef DEBUG_API
  Serial.println(msg);
#endif
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
