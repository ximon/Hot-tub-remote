//todo - theres ~40ms between commands from the controller, wait ~20us after a comand has ended to send any commands
//todo - occasionally we will need to send a temp up / down command to read the desired temp set on the pump
//todo - the commands should probably be added to a queue to be sent to the pump
//todo - still need to check how the flashing works

//#define IGNORE_INVALID_START_PULSE //enable this to allow testing without a valid start pulse
#define ALLOW_SEND_WITHOUT_RECEIVE false //enable this to allow sending commands without having first received any
#define DISABLE_TARGET_TEMPERATURE

#define HOSTNAME "HotTubRemote-" // Hostname. The setup function adds the Chip ID at the end.

#include <ArduinoJson.h>
#include <IotWebConf.h>
#include <WiFiClient.h>
#include <ESP8266WiFi.h>
#include <esp8266_peri.h>
#include <ESP8266WebServer.h>
#include <ArduinoOTA.h>
#include <SPI.h>
#include "RouteHandler.h"
#include "Commands.h"
#include "SendReceive.h"
#include "Interrupts.h"
#include "Temperatures.h"

#define MIN_TEMP 9 //Minimum temperature supported by the hot tub
#define MAX_TEMP 40 //Maximum temperature supported by the hot tub


// -- Initial name of the Thing. Used e.g. as SSID of the own Access Point.
const char thingName[] = "HotTubRemote";
// -- Initial password to connect to the Thing, when it creates an own Access Point.
const char wifiInitialApPassword[] = "HotTubRemote";

#define IOTWEBCONF_CONFIG_USE_MDNS

DNSServer dnsServer;
ESP8266WebServer server(80);
IotWebConf iotWebConf(thingName, &dnsServer, &server, wifiInitialApPassword);
 

bool sendTestCommand = false;

/*
 * Pump status stuff
 */
bool autoRestart = false;


//these are the current states (decoded from the serial stream)
bool pumpEnabled = false;
bool blowerEnabled = false;
bool heaterEnabled = false;
bool heaterHeating = false;

int errorCode = 0;

int temperature = 0; //Current actual water temperature
int pumpTargetTemperature = 0; //The target temperature that is set on the pump (set by the panel or us)

int targetTemperature = 37; //desired water temperature, the loop will send up / down signals to try to set the pump to this temp
int maxTemperature = MAX_TEMP; //prevent the panel from changing the temperature above this value, the loop will send down signals to decrease the adjusted value

bool panelLock = false; //if status changes are received, revert them
bool temperatureLock = false; //if temperature changes are received, revert them










/*
 * Helper functions
 */
 
String toOnOff(bool value) {
  return value ? "on" : "off";
}


/*
 * Server helper functions
 */

String getArg(String argumentName) {
  argumentName.toLowerCase();
  return server.arg(argumentName);
}

bool hasArg(String argumentName) {
  argumentName.toLowerCase();
  return server.hasArg(argumentName);
}

bool checkArgument(String argumentName) {
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

bool valueIsTrue(String argumentName) {
  String value = getArg(argumentName);
  value.trim();
  value.toLowerCase();
  
  return value == "on" || value == "1" || value == "true" || value == "yes";
}


/*
 * Http Helper functions
 */

void addCorsHeaders() {
  server.sendHeader("Access-Control-Allow-Origin", "*");
}

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
  addCorsHeaders();
  server.send(200, "application/json", json);
}

//outputs the plain text
void okText(String message) {
  server.send(200, "text/plain", message);
}

//outputs the html
void okHtml(String html) {
  server.send(200, "text/html", html);
}

//sends the error message wrapped in a json object
void errorMessage(String message) {
  server.send(400, "application/json", toJson(message));
}

//wraps the provided message in a json object
String toJson(String message) {
  const size_t capacity = JSON_OBJECT_SIZE(5); //todo - 5? what size should this be? variable?
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

void handle_status() {
  const size_t capacity = JSON_OBJECT_SIZE(12);
  StaticJsonDocument<capacity> doc;
  String json;

  doc["autoRestart"] = autoRestart;
  doc["pump"] = pumpEnabled;
  doc["blower"] = blowerEnabled;
  doc["heaterEnabled"] = heaterEnabled;
  doc["heaterHeating"] = heaterHeating;
  doc["temperature"] = temperature;
  doc["maxTemperature"] = maxTemperature;
  doc["targetTemperature"] = targetTemperature;
  doc["temperatureLock"] = temperatureLock;
  doc["panelLock"] = panelLock;
  doc["errorCode"] = errorCode;

  serializeJson(doc, json);
  okJson(json);
}

void handle_autoRestart() {
  if (checkArgument("state")) 
    return;

  autoRestart = valueIsTrue("state");
  
  String msg = "Auto restart is " + toOnOff(autoRestart);
  okMessage(msg);
  Serial.println(msg);
}

void handle_pump() {
  if (checkArgument("state"))
    return;

  if (pumpEnabled != valueIsTrue("state"))
    sendCommand(CMD_BTN_PUMP);
  
  String msg = "Sending pump command";
  okMessage(msg);
  Serial.println(msg);
}

void handle_heater() {
  if (checkArgument("state"))
    return;
  
  if (heaterEnabled != valueIsTrue("state"))
    sendCommand(CMD_BTN_HEAT);
    
  String msg = "Sending heater command";
  okMessage(msg);
  Serial.println(msg);
}

void handle_blower() {
  if (checkArgument("state"))
    return;

  if (blowerEnabled != valueIsTrue("state"))
    sendCommand(CMD_BTN_BLOW);
    
  String msg = "Sending blower command";
  okMessage(msg);
  Serial.println(msg);
}
  
void handle_temperature() {
  if (checkArgument("value")) 
    return;
      
  int temp = getArg("value").toInt();
  
  if (!temperatureValid(temp, maxTemperature)) 
    return;
  
  targetTemperature = temp;
  setTempMessage(targetTemperature, "target temperature");
}

void handle_command() {
  if (checkArgument("command"))
    return;

  String commandStr =  getArg("command");
  word command = strtol(commandStr.c_str(), 0, 16);

  if (command = 0) {
    errorMessage("Commands must be in the format 0x1234");
    return;
  } else if (command >= 0x4000) {
    errorMessage("Commands must be less than 0x4000");
    return;
  }

  String msg = "Sending command " + commandStr;
  sendCommand(command);
  
  okMessage(msg);
}

void handle_maxTemperature() {
  if (checkArgument("value"))
    return;
  
  int temp = getArg("value").toInt();
 
  if (!temperatureValid(temp, MAX_TEMP))
    return;
  
  maxTemperature = temp;

  String msg = "max temperature";

  if (targetTemperature > maxTemperature) {
    targetTemperature = maxTemperature;
    msg += " and target temperature";
  }
  
  setTempMessage(maxTemperature, msg);
}

void setTempMessage(int temp, String tempType) {
  String msg = "Setting " + tempType + " to " + String(temp);
  okMessage(msg);
  Serial.println(msg);
}

bool temperatureValid(int desiredTemp, int maxTemp) {
  bool aboveMax = desiredTemp > maxTemp;
  bool belowMin = desiredTemp < MIN_TEMP;

  if (aboveMax || belowMin) {
    String message = (String)(aboveMax ? "Max" : "Min") + " temperature is " + (String)(aboveMax ? String(maxTemp) : String(MIN_TEMP));
    errorMessage(message);
    return false;
  }

  return true;
}

void handle_panelLock() {
  if (checkArgument("state"))
    return;

  panelLock = valueIsTrue("state");
  
  String msg = "Panel lock is " + toOnOff(panelLock);
  okMessage(msg);
  Serial.println(msg);
}

void handle_temperatureLock() {
  if (checkArgument("state")) 
    return;

  temperatureLock = valueIsTrue("state");

  String msg = "Temperature lock is " + toOnOff(temperatureLock);
  okMessage(msg);
  Serial.println(msg);
}






void handleButtonPress() {
  sendTestCommand = true;
}

void handleDataInterrupt() {
  word data = receiveCommand();
  decodeCommand(data); 
  lastReceivedCommand = millis();
}



void decodeCommand(word command) {
  if (command == 0)
    return; //todo - do something with an invalid command?!?!
  
  if ((command >= CMD_ERROR_PKT1 && command <= CMD_END) || command == CMD_FLASH) {
    Serial.println("Decoding status command");
    decodeStatus(command);
    return;
  }

  if (command >= TEMP_104F && command <= TEMP_9C) {
    Serial.println("Decoding temperature command");
    temperature = decodeTemperature(command);
    return;
  }

  if (command >= CMD_BTN_HEAT) {
    Serial.println("Decoding button command");
    decodeButton(command);
    return;
  }
}

void decodeButton(word command) {
  String button;
  
  switch(command) {
    case CMD_BTN_HEAT:
      button = "Heater";
      break;
    case CMD_BTN_PUMP:
      button = "Pump";
      break;
    case CMD_BTN_BLOW:
      button = "Blower";
      break;
    case CMD_BTN_TEMP_DN:
      button = "Temperature down";
      break;
    case CMD_BTN_TEMP_UP:
      button = "Temperature up";
      break;
  }

  Serial.print(button);
  Serial.println(" button was pressed");
}

void decodeStatus(word command) {
   pumpEnabled = false;
   blowerEnabled = false;
   heaterEnabled = false;
   heaterHeating = false;

   errorCode = 0;
  
   switch(command) {
    case CMD_STATE_ALL_OFF_F:
    case CMD_STATE_ALL_OFF_C:
      break;
    case CMD_STATE_PUMP_ON_C:
    case CMD_STATE_PUMP_ON_F:
      pumpEnabled = true;
      break;
    case CMD_STATE_BLOWER_ON_C:
    case CMD_STATE_BLOWER_ON_F:
      blowerEnabled = true;
      break;
    case CMD_STATE_PUMP_ON_HEATER_ON:
      heaterEnabled = true;
      break;
    case CMD_STATE_PUMP_ON_HEATER_HEATING:
      heaterEnabled = true;
      heaterHeating = true;
      break;
    default:
      errorCode = decodeError(command);
  }
}

int decodeError(word command) {
  String text;
  int code = 0;
  
  switch (command) {
    case CMD_ERROR_PKT1:
      text = "Error 1,3,4 1st Packet, error to follow...";
      break;
    case CMD_ERROR_PKT2:
      text = "Error 2,5,END 1st Packet, error to follow...";
      break;
    case CMD_ERROR_1:
      code = 1;
      text = "Flow sensor stuck in on state";
      break;
    case CMD_ERROR_2:
      code = 2;
      text = "Flow sensor stuck in off state";
      break;
    case CMD_ERROR_3:
      code = 3;
      text = "Water temperature below 4C";
      break;
    case CMD_ERROR_4:
      code = 4;
      text = "Water temperature above 50C";
      break;
    case CMD_ERROR_5:
      code = 5;
      text = "Unsuitable power supply";
      break;
  }

  Serial.print("Error code ");
  Serial.print(code);
  Serial.print(" received - ");
  Serial.println(text);

  return code;
}

void testCommandCheck() {
  if (sendTestCommand) {
    sendTestCommand = false;
    
    Serial.println("Button Pressed!");
    sendCommand(CMD_BTN_HEAT);
  }
}

void targetTemperatureCheck(){
  
  if (pumpTargetTemperature != targetTemperature
    && millis() - lastSentCommand > MILLIS_BETWEEN_COMMANDS
    && millis() - lastReceivedCommand > MILLIS_WAIT_AFTER_COMMAND
    && (lastReceivedCommand > 0 || ALLOW_SEND_WITHOUT_RECEIVE))
  {
    Serial.println("Adjusting target temperature");
    if (pumpTargetTemperature < targetTemperature)
        sendCommand(CMD_BTN_TEMP_UP);
        
    if (pumpTargetTemperature > targetTemperature)
        sendCommand(CMD_BTN_TEMP_DN);
  }
}

void autoRestartCheck() {
  //todo - we need to check if the pump has sent the END command
  if (autoRestart) {
    //sendCommand(CMD_BTN_HEAT);
  }
}

void setupBoard() {
  pinMode(LED, OUTPUT);    
  pinMode(DATA_IN, INPUT);
  pinMode(DATA_OUT, OUTPUT);
  pinMode(DBG, OUTPUT);
  pinMode(BUTTON, INPUT);

  attachInterrupt(digitalPinToInterrupt(DATA_IN), handleDataInterrupt, RISING); 
  attachInterrupt(digitalPinToInterrupt(BUTTON), handleButtonPress, RISING);
}

void setupOTA() {
  // Set Hostname.
  String hostname(HOSTNAME);
  hostname += String(ESP.getChipId(), HEX);
  WiFi.hostname(hostname);
  
  ArduinoOTA.setHostname((const char *)hostname.c_str());
  ArduinoOTA.onStart([]() {
    Serial.println("Start");
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });
  ArduinoOTA.begin();
}

void setupWebServer() {
  iotWebConf.setStatusPin(LED);
  iotWebConf.skipApStartup();
  iotWebConf.init();

  // -- Set up required URL handlers on the web server.
  server.on("/", handle_root);
  server.on("/config", []{ iotWebConf.handleConfig(); });
  server.onNotFound([](){ iotWebConf.handleNotFound(); });

  server.on("/status", handle_status);
  server.on("/autoRestart", HTTP_POST, handle_autoRestart);
  server.on("/pump", HTTP_POST, handle_pump);
  server.on("/heater", HTTP_POST, handle_heater);
  server.on("/blower", HTTP_POST, handle_blower);
  server.on("/command", HTTP_POST, handle_command);
  server.addHandler(new RouteHandler("/lock/panel", HTTP_POST, handle_panelLock));
  server.addHandler(new RouteHandler("/lock/temperature", HTTP_POST, handle_temperatureLock));
  server.addHandler(new RouteHandler("/temperature/target", HTTP_POST, handle_temperature));
  server.addHandler(new RouteHandler("/temperature/max", HTTP_POST, handle_maxTemperature));

  server.begin();
  Serial.println("HTTP server started");
}

void setup(void)
{
  Serial.begin(115200);
  Serial.println("Starting...");
  
  setupBoard();
  setupOTA();
  setupWebServer();
}

void loop(void)
{  
  iotWebConf.doLoop(); // doLoop should be called as frequently as possible.
  server.handleClient();
  ArduinoOTA.handle();
  yield();
  
  testCommandCheck();
  #ifndef DISABLE_TARGET_TEMPERATURE
  targetTemperatureCheck();
  #endif
  autoRestartCheck();
} 
