#ifndef HOTTUBAPI_H
#define HOTTUBAPI_H

#include <ArduinoJson.h>
#include "RouteHandler.h"
#include "HotTub.h"
#include <ESP8266WebServer.h>
#include <Syslog.h>

class HotTubApi
{
public:
  HotTubApi(ESP8266WebServer *webServer, HotTub *hotTubInstance, Syslog *syslog);
  void setup();

private:
  ESP8266WebServer *server;
  HotTub *hotTub;
  Syslog *logger;

  // Helper functions
  String toOnOff(bool value);
  void valueNotProvided(String argumentName);
  bool checkArgument(String argumentName);
  bool valueIsTrue(String argumentName);

  // Request Handlers
  void handle_status();
  void handle_autoRestart();
  void handle_pump();
  void handle_heater();
  void handle_blower();
  void handle_temperature();
  void handle_maxTemperature();
  void setTempMessage(int temp, String tempType);
  bool temperatureValid(int desiredTemp, int maxTemp);
  void handle_rawCommand();
  void handle_panelLock();
  void handle_temperatureLock();

  // Http Helper functions
  void addCorsHeaders();
  void okMessage();
  void okMessage(String message);
  void okJson(char *json);
  void okJson(String json);
  void okText(String message);
  void errorMessage(String message);
  String toJson(String message);
};

#endif
