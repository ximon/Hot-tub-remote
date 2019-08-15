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
  server->addHandler(new RouteHandler("/lock/panel", HTTP_POST, std::bind(&HotTubApi::handle_panelLock, this)));
  server->addHandler(new RouteHandler("/lock/temperature", HTTP_POST, std::bind(&HotTubApi::handle_temperatureLock, this)));
  server->addHandler(new RouteHandler("/temperature/target", HTTP_POST, std::bind(&HotTubApi::handle_temperature, this)));
  server->addHandler(new RouteHandler("/temperature/max", HTTP_POST, std::bind(&HotTubApi::handle_maxTemperature, this)));
}
