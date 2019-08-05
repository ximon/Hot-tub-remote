#include <ArduinoJson.h>
#include <IotWebConf.h>
#include <WiFiClient.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <SPI.h>




// -- Initial name of the Thing. Used e.g. as SSID of the own Access Point.
const char thingName[] = "HotTubRemote";

// -- Initial password to connect to the Thing, when it creates an own Access Point.
const char wifiInitialApPassword[] = "HotTubRemote";

DNSServer dnsServer;
ESP8266WebServer server(80);
IotWebConf iotWebConf(thingName, &dnsServer, &server, wifiInitialApPassword);
 


#define LEDPIN 2
#define COUNTPIN 0

#define MAX_TEMP 40 //Maximum temperature supported by the hot tub
//todo - find out what the min is???
#define MIN_TEMP 15 //Minimum temperature supported by the hot tub

int autoRestart = 0;
int pumpState = 0;
int blowerState = 0;
int heaterState = 0;
int temperature = 0;

int targetTemperature = 37;




/*
 * Helper functions
 */
 
String toOnOff(bool value) {
  return value ? "on" : "off";
}

bool isArgumentMissing(String argumentName) {
  if (!hasArg(argumentName)) {
    valueMissing(argumentName);
    return true;
  }

  return false;
}

void valueMissing(String argumentName) {
  String output = "No " + argumentName + " provided";
  errorMessage(output);
  Serial.println(output);  
}

bool valueIsOn(String argumentName) {
  String value = getArg(argumentName);
  value.trim();
  value.toLowerCase();
  
  return value == "on" || value == "1" || value == "true" || value == "yes";
}

String getArg(String argumentName) {
  argumentName.toLowerCase();
  return server.arg(argumentName);
}

bool hasArg(String argumentName) {
  argumentName.toLowerCase();
  return server.hasArg(argumentName);
}




/*
 * Http Helper functions
 */

//passes "Done" to be wrapped in a json object
void okMessage() {
  okJson(toJson("Done"));
}

//pass in a message to be wrapped in a json object
void okMessage(String message) {
  okJson(toJson(message));
}

//outputs the json
void okJson(String json) {
  server.send(200, "application/json", json);
}

//outputs the plain text
void okText(String message) {
  server.send(200, "text/plain", message);
}

void okHtml(String html) {
  server.send(200, "text/html", html);
}

//sends the error message wrapped in a json object
void errorMessage(String message) {
  server.send(400, "application/json", toJson(message));
}

//wraps the provided message in a json object
String toJson(String message) {
  const size_t capacity = JSON_OBJECT_SIZE(3);
  StaticJsonDocument<capacity> doc;
  String json;

  doc["message"] = message;
  serializeJson(doc, json);
  return json;
}




/*
 * URI Handler Functions
 */

void handle_root() {
  // -- Captive portal request were already served.
  if (iotWebConf.handleCaptivePortal()) {
    return;
  }
  
  String s = "<!DOCTYPE html><html lang=\"en\"><head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1, user-scalable=no\"/>";
  s += "<title>Hot Tub Remote</title></head><body>Hot Tub Remote<br /><br />";
  s += "Go to <a href='config'>configure page</a> to change settings.<br />";
  s += "Go to <a href='states'>states</a> to view states.<br />";
  s += "</body></html>\n";

  okHtml(s);
}

void handle_states() {
  const size_t capacity = JSON_OBJECT_SIZE(7); //weirdld, this needs to be +1 since adding toOnOff?!
  StaticJsonDocument<capacity> doc;
  String json;

  doc["autoRestart"] = toOnOff(autoRestart);
  doc["pump"] = toOnOff(pumpState);
  doc["blower"] = toOnOff(blowerState);
  doc["heater"] = heaterState == 2 ? "heating" : toOnOff(heaterState);
  doc["temperature"] = temperature;
  doc["targetTemperature"] = targetTemperature;

  serializeJson(doc, json);
  okJson(json);
}

void handle_autoRestart() {
  if (isArgumentMissing("state")) return;
  
  autoRestart = valueIsOn("state");
  
  String msg = "Auto restart is " + toOnOff(autoRestart);
  okMessage(msg);
  Serial.println(msg);
}

void handle_pump() {
  if (isArgumentMissing("state")) return;
  
  pumpState = valueIsOn("state");
  String msg = "Turning pump " + toOnOff(pumpState);
  okMessage(msg);
  Serial.println(msg);
}

void handle_heater() {
  if (isArgumentMissing("state")) return;
  
  heaterState = valueIsOn("state");

  String msg = "Turning heater " + toOnOff(heaterState);
  okMessage(msg);
  Serial.println(msg);
}

void handle_blower() {
  if (isArgumentMissing("state")) return;
 
  blowerState = valueIsOn("state");
  
  String msg = "Turning blower " + toOnOff(blowerState);
  okMessage(msg);
  Serial.println(msg);
}
  
void handle_temperature() {
  if (isArgumentMissing("value")) return;
  
  int desiredTemp = getArg("value").toInt();

  bool aboveMax = desiredTemp > MAX_TEMP;
  bool belowMin = desiredTemp < MIN_TEMP;

  if (aboveMax || belowMin) {
    String message = (String)(aboveMax ? "Max" : "Min") + " temperature is " + (String)(aboveMax ? String(MAX_TEMP) : String(MIN_TEMP));
    errorMessage(message);
    return;
  }

  targetTemperature = desiredTemp;
  String msg = "Setting temperature to " + String(targetTemperature);
  okMessage(msg);
  Serial.println(msg);
}




void setup(void)
{
  pinMode(LEDPIN, OUTPUT);    
  pinMode(COUNTPIN, INPUT);
  Serial.begin(115200);  // Serial connection from ESP-01 via 3.3v console cable

  // -- Initializing the configuration.
  iotWebConf.setStatusPin(LEDPIN);
  iotWebConf.init();

  // -- Set up required URL handlers on the web server.
  server.on("/", handle_root);
  server.on("/config", []{ iotWebConf.handleConfig(); });
  server.onNotFound([](){ iotWebConf.handleNotFound(); });

  server.on("/states", handle_states);
  server.on("/autoRestart", HTTP_POST, handle_autoRestart);
  server.on("/pump", HTTP_POST, handle_pump);
  server.on("/heater", HTTP_POST, handle_heater);
  server.on("/blower", HTTP_POST, handle_blower);
  server.on("/temperature", HTTP_POST, handle_temperature);
  
  server.begin();
  Serial.println("HTTP server started");
}

void loop(void)
{  
  // -- doLoop should be called as frequently as possible.
  iotWebConf.doLoop();
  server.handleClient();
} 
