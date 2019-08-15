//todo - theres ~40ms between commands from the controller, wait ~20us after a comand has ended to send any commands
//todo - occasionally we will need to send a temp up / down command to read the desired temp set on the pump
//todo - the commands should probably be added to a queue to be sent to the pump
//todo - still need to check how the flashing works
//todo - sending temperature up / down commands results in printing 0x0 to the serial port?!?!

#define HOSTNAME "HotTubRemote-" // Hostname. The setup function adds the Chip ID at the end.

#include <ArduinoJson.h>
#include <IotWebConf.h>
#include <WiFiClient.h>
#include <ESP8266WiFi.h>
#include <esp8266_peri.h>
#include <ESP8266WebServer.h>
#include <ArduinoOTA.h>
#include <SPI.h>
#include "Interrupts.h"
#include "HotTub.h"
#include "HotTubApi.h"
#include "Pins.h"


// -- Initial name of the Thing. Used e.g. as SSID of the own Access Point.
const char thingName[] = "HotTubRemote";
// -- Initial password to connect to the Thing, when it creates an own Access Point.
const char wifiInitialApPassword[] = "HotTubRemote";

//todo - check mDNS is working
#define IOTWEBCONF_CONFIG_USE_MDNS

DNSServer dnsServer;
ESP8266WebServer server(80);
IotWebConf iotWebConf(thingName, &dnsServer, &server, wifiInitialApPassword);

HotTub hotTub(DATA_IN, DATA_OUT, DBG);
HotTubApi hotTubApi(&server, &hotTub);

bool sendTestCommand = false;

void onStateChange() {
  
}

//outputs the html
void okHtml(String html) {
  server.send(200, "text/html", html);
}

void handle_root() {
  // -- Captive portal request were already served.
  if (iotWebConf.handleCaptivePortal()) {
    return;
  }
  
  String s = "<!DOCTYPE html><html lang=\"en\"><head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1, user-scalable=no\"/>";
  s += "<title>Hot Tub Remote</title></head><body>Hot Tub Remote<br /><br />";
  s += "Go to <a href='config'>configure page</a> to change settings.<br />";
  s += "Go to <a href='status'>status</a> to view status.<br />";
  s += "</body></html>\n";

  okHtml(s);
}

void handleDataInterrupt() {
  hotTub.dataAvailable();
}

void handleButtonPress() {
  sendTestCommand = true;
}

void setupBoard() {
  pinMode(BUTTON, INPUT);
  attachInterrupt(digitalPinToInterrupt(BUTTON), handleButtonPress, RISING);
  attachInterrupt(digitalPinToInterrupt(DATA_IN), handleDataInterrupt, RISING);
  
  iotWebConf.setStatusPin(LED);
  iotWebConf.skipApStartup();
  iotWebConf.init();
}

void setupOTA() {
  // Set Hostname
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
  // -- Set up required URL handlers on the web server.
  server.on("/", handle_root);
  server.on("/config", []{ iotWebConf.handleConfig(); });
  server.onNotFound([](){ iotWebConf.handleNotFound(); });
  server.begin();
}

void setup(void)
{
  Serial.begin(115200);
  Serial.println("Starting...");
  
  setupBoard();
  setupOTA();
  
  hotTub.setup(onStateChange);
  hotTubApi.setup();

  setupWebServer();
}

void loop(void)
{  
  iotWebConf.doLoop(); // doLoop should be called as frequently as possible.
  server.handleClient();
  ArduinoOTA.handle();
  
  testCommandCheck();
  
  hotTub.loop();
  yield();
} 

void testCommandCheck() {
  if (sendTestCommand) {
    sendTestCommand = false;
    
    Serial.println("Button Pressed!");
    hotTub.sendCommand(CMD_BTN_HEAT);
  }
}
