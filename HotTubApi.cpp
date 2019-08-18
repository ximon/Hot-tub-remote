#include "HotTubApi.h"
#include <ESP8266WebServer.h>


HotTubApi::HotTubApi(ESP8266WebServer* webServer, HotTub* hotTubInstance)
 : server(webServer),
  hotTub(hotTubInstance)
{
}

void HotTubApi::setup() {  
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
 * Helper functions
 */
 
String HotTubApi::toOnOff(bool value) {
  return value ? "on" : "off";
}

void HotTubApi::valueNotProvided(String argumentName) {
  String output = "No " + argumentName + " provided";
  errorMessage(output);
  Serial.println(output);  
}

bool HotTubApi::checkArgument(String argumentName) {
  if (!server->hasArg(argumentName)) {
    valueNotProvided(argumentName);
    return true;
  }

  return false;
}
    
bool HotTubApi::valueIsTrue(String argumentName) {
  String value = server->arg(argumentName);
  value.trim();
  value.toLowerCase();
  
  return value == "on" || value == "1" || value == "true" || value == "yes";
}


/*
 * Request Handlers
 */

void HotTubApi::handle_status() {
  Serial.println("API->handle_status()");
  
  const size_t capacity = 2*JSON_OBJECT_SIZE(7) + JSON_OBJECT_SIZE(6);
  DynamicJsonDocument doc(capacity);
  String json;

  CurrentState* currentState = hotTub->getCurrentState();
  JsonObject currentJson = doc.createNestedObject("currentState");
  currentJson["pumpState"] = currentState->pumpState;
  currentJson["temperature"] = currentState->temperature;
  currentJson["targetTemperature"] = currentState->targetTemperature;

  TargetState* targetState = hotTub->getTargetState();
  JsonObject targetJson = doc.createNestedObject("targetState");
  targetJson["pumpState"] = targetState->pumpState;
  targetJson["targetTemperature"] = targetState->targetTemperature;
  
  doc["autoRestart"] = hotTub->autoRestartEnabled;
  doc["limitTemperature"] = hotTub->getLimitTemperature();
  doc["temperatureLock"] = hotTub->temperatureLockEnabled;
  doc["errorCode"] = currentState->errorCode;

  serializeJson(doc, json);
  okJson(json);
}

void HotTubApi::handle_pump() {
  Serial.println("API->handle_pump()");
  if (checkArgument("state"))
    return;

  bool enable = valueIsTrue("state");
  String msg = "Sending pump command";

  hotTub->setTargetState(enable ? PUMP_FILTERING : PUMP_OFF);
  
  okMessage(msg);
  Serial.println(msg);
}

void HotTubApi::handle_heater() {
  Serial.println("API->handle_heater()");  
  if (checkArgument("state"))
    return;

  bool enable = valueIsTrue("state");
  String msg = "Sending heater command";
  
  hotTub->setTargetState(enable ? PUMP_HEATING : PUMP_FILTERING);
  
  okMessage(msg);
  Serial.println(msg);
}

void HotTubApi::handle_blower() {
  Serial.println("API->handle_blower()");
  if (checkArgument("state"))
    return;

  bool enable = valueIsTrue("state");
  String msg = "Sending blower command";
  
  hotTub->setTargetState(enable ? PUMP_BUBBLES : PUMP_HEATING);//todo - configurable state after blower turned off??
    
  okMessage(msg);
  Serial.println(msg);
}
  
void HotTubApi::handle_temperature() {
  Serial.println("API->handle_temperature()");
  if (checkArgument("value")) 
    return;
      
  int temp = server->arg("value").toInt();
  
  if (!temperatureValid(temp, hotTub->getLimitTemperature()))
    return;
  
  hotTub->setTargetTemperature(temp);
  setTempMessage(temp, "target temperature");
}

void HotTubApi::handle_maxTemperature() {
  Serial.println("API->handle_maxTemperature()");
  if (checkArgument("value"))
    return;
  
  int maxTemp = server->arg("value").toInt();
 
  if (!temperatureValid(maxTemp, MAX_TEMP))
    return;
  
  hotTub->setLimitTemperature(maxTemp);
  setTempMessage(maxTemp, "limit temperature");
}

void HotTubApi::setTempMessage(int temp, String tempType) {
  String msg = "Setting " + tempType + " to " + String(temp);
  okMessage(msg);
  Serial.println(msg);
}

bool HotTubApi::temperatureValid(int desiredTemp, int maxTemp) {
  bool aboveMax = desiredTemp > maxTemp;
  bool belowMin = desiredTemp < MIN_TEMP;

  if (aboveMax || belowMin) {
    String message = (String)(aboveMax ? "Max" : "Min") + " temperature is " + (String)(aboveMax ? String(maxTemp) : String(MIN_TEMP));
    errorMessage(message);
    return false;
  }

  return true;
}

void HotTubApi::handle_rawCommand() {
  Serial.println("API->handle_rawCommand()");
  if (checkArgument("command"))
    return;

  String commandStr =  server->arg("command");
  commandStr.toLowerCase();
  word command = strtol(commandStr.c_str(), 0, 16);

  if (command = 0) {
    errorMessage("Commands must be in the format 0x1234");
    return;
  } else if (command >= 0x4000) {
    errorMessage("Commands must be less than 0x4000");
    return;
  }
  
  String msg;
  if (hotTub->queueCommand(command)){
    msg = "Sending command " + commandStr;
  } else {
    msg = "Too many commands in queue!";
  }
  okMessage(msg);
}

void HotTubApi::handle_autoRestart() {
  Serial.println("API->handle_autoRestart()");
  if (checkArgument("state")) 
    return;

  bool enable = valueIsTrue("state");
  hotTub->autoRestartEnabled = enable;
  
  String msg = "Auto restart is " + toOnOff(enable);
  okMessage(msg);
  Serial.println(msg);
}

void HotTubApi::handle_temperatureLock() {
  Serial.println("API->handle_temperatureLock()");
  if (checkArgument("state")) 
    return;

  bool enable = valueIsTrue("state");
  hotTub->temperatureLockEnabled = enable;

  String msg = "Temperature lock is " + toOnOff(enable);
  okMessage(msg);
  Serial.println(msg);
}

/*
 * Http Helper functions
 */

void HotTubApi::addCorsHeaders() {
  server->sendHeader("Access-Control-Allow-Origin", "*");
}

//passes "Done" to be wrapped in a json object
void HotTubApi::okMessage() {
  okJson(toJson("Done"));
}

//pass in a message to be wrapped in a json object
void HotTubApi::okMessage(String message) {
  okJson(toJson(message));
}

//outputs the json
void HotTubApi::okJson(String json) {
  addCorsHeaders();
  server->send(200, "application/json", json);
}

//outputs the plain text
void HotTubApi::okText(String message) {
  addCorsHeaders();
  server->send(200, "text/plain", message);
}

//sends the error message wrapped in a json object
void HotTubApi::errorMessage(String message) {
  addCorsHeaders();
  server->send(400, "application/json", toJson(message));
}

//wraps the provided message in a json object
String HotTubApi::toJson(String message) {
  const size_t capacity = JSON_OBJECT_SIZE(5); //todo - 5? what size should this be? variable?
  StaticJsonDocument<capacity> doc;
  String json;

  doc["message"] = message;
  serializeJson(doc, json);
  return json;
}
