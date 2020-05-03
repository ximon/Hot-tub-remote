//todo - test waiting ~20ms after a comand has ended to send any commands

//todo - option to pull line high for a short period to reset disiplay controller on error?

//test temperature lock
//need to check target state changing and autorestart

#define HOSTNAME "HotTubRemote"

#include <ArduinoJson.h>
#include <IotWebConf.h>
#include <DNSServer.h>
#include <WiFiClient.h>
#include <ESP8266WiFi.h>
#include <esp8266_peri.h>

#include <ESPAsyncTCP.h>
#include <ESP8266mDNS.h>

#include <ArduinoOTA.h>
#include <SPI.h>
#include <FS.H>
#include "HotTub.h"
#include "HotTubApi.h"
#include "HotTubMqtt.h"
#include "Pins.h"

// -- Initial name of the Thing. Used e.g. as SSID of the own Access Point.
const char thingName[] = "HotTubRemote";
// -- Initial password to connect to the Thing, when it creates an own Access Point.
const char wifiInitialApPassword[] = "HotTubRemote";

#define CONFIG_VERSION "beta3"

DNSServer dnsServer;
ESP8266WebServer server(80);
//WebSocketsServer webSocket(81);
HTTPUpdateServer httpUpdater;

#define STRING_LEN 128
#define NUMBER_LEN 32

char mqttServer[STRING_LEN] = "";
char mqttPort[NUMBER_LEN] = "";
char mqttUser[STRING_LEN] = "";
char mqttPass[STRING_LEN] = "";

IotWebConf iotWebConf(thingName, &dnsServer, &server, wifiInitialApPassword, CONFIG_VERSION);
IotWebConfSeparator separator = IotWebConfSeparator("MQTT Settings");
IotWebConfParameter mqttServerParam = IotWebConfParameter("Server", "mqttServer", mqttServer, STRING_LEN);
IotWebConfParameter mqttPortParam = IotWebConfParameter("Port", "mqttPort", mqttPort, NUMBER_LEN, "number", "1883", NULL, "min='1' max='65535' step='1'");
IotWebConfParameter mqttUserParam = IotWebConfParameter("User", "mqttUser", mqttUser, STRING_LEN);
IotWebConfParameter mqttPassParam = IotWebConfParameter("Password", "mqttPass", mqttPass, STRING_LEN);

HotTub hotTub(DATA_IN, DATA_ENABLE, DBG);
HotTubApi hotTubApi(&server, &hotTub);
HotTubMqtt hotTubMqtt(&hotTub);

bool OTASetup = false;
bool sendTestCommand = false;

void onStateChange()
{
  hotTubMqtt.sendStatus();

  //webSocket.broadcastTXT(hotTub.getStateJson());
}

void handle_root()
{
  // -- Captive portal request were already served.
  if (iotWebConf.handleCaptivePortal())
  {
    return;
  }

  String html = "<!DOCTYPE html><html lang=\"en\"><head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1, user-scalable=no\"/>";
  html += "<title>Hot Tub Remote</title></head><body>Hot Tub Remote<br /><br />";
  html += "Go to <a href='config'>configure page</a> to change settings.<br />";
  html += "Go to <a href='status'>status</a> to view status.<br />";
  html += "</body></html>\n";

  server.send(200, "text/html", html);
}

void ICACHE_RAM_ATTR handleDataInterrupt()
{
  hotTub.dataInterrupt();
}

void ICACHE_RAM_ATTR handleButtonPress()
{
  sendTestCommand = true;
}
/*
void webSocketEvent(uint8_t num, WStype_t type, uint8_t *payload, size_t length)
{
  switch (type)
  {
  case WStype_DISCONNECTED: // if the websocket is disconnected
    Serial.printf("[%u] Disconnected!\n", num);
    break;
  case WStype_CONNECTED:
  { // if a new websocket connection is established
    IPAddress ip = webSocket.remoteIP(num);
    Serial.printf("[%u] Connected from %d.%d.%d.%d url: %s\n", num, ip[0], ip[1], ip[2], ip[3], payload);
  }
  break;
  case WStype_TEXT: // if new text data is received
    Serial.printf("[%u] get Text: %s\n", num, payload);

    webSocket.sendTXT(num, "Received!");

    break;
  }
}
*/
void setupBoard()
{
  pinMode(BUTTON, INPUT_PULLUP);
  pinMode(DBG, OUTPUT);
  pinMode(LED, OUTPUT);
  pinMode(DATA_IN, INPUT);
  pinMode(DATA_OUT, OUTPUT); // Override default Serial initiation
  pinMode(DATA_ENABLE, OUTPUT);
  digitalWrite(DATA_OUT, 0);
  digitalWrite(DATA_ENABLE, LOW);
}

void setupMqtt()
{
  hotTubMqtt.setServer(mqttServer, atoi(mqttPort));

  if (strlen(mqttUser) > 0)
    hotTubMqtt.setCredentials(&mqttUser[0], &mqttPass[0]);
}

void setupOTA()
{
  // Set Hostname
  //String hostname();
  WiFi.hostname(HOSTNAME);

  ArduinoOTA.setHostname(HOSTNAME);
  ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH)
    {
      type = "sketch";
    }
    else
    { // U_FS
      type = "filesystem";
    }

    // NOTE: if updating FS this would be the place to unmount FS using FS.end()
    Serial.println("Start updating " + type);
  });
  ArduinoOTA.onEnd([]() { Serial.println("\nEnd"); });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR)
      Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR)
      Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR)
      Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR)
      Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR)
      Serial.println("End Failed");
  });
  ArduinoOTA.begin();

  Serial.println("OTA Setup complete.");
}

void wifiConnected()
{
  setupOTA();

  if (strlen(mqttServer) > 0)
    setupMqtt();
}

void setupIotWebConf()
{
  iotWebConf.setStatusPin(LED);
  iotWebConf.skipApStartup();
  iotWebConf.setupUpdateServer(&httpUpdater);
  iotWebConf.setWifiConnectionCallback(wifiConnected);

  iotWebConf.addParameter(&separator);
  iotWebConf.addParameter(&mqttServerParam);
  iotWebConf.addParameter(&mqttPortParam);
  iotWebConf.addParameter(&mqttUserParam);
  iotWebConf.addParameter(&mqttPassParam);

  iotWebConf.init();
}

void handleNotFound()
{
  iotWebConf.handleNotFound();
}

void setupWebServer()
{
  // -- Set up required URL handlers on the web server.
  server.on("/", handle_root);
  server.on("/config", [] { iotWebConf.handleConfig(); });
  server.onNotFound(handleNotFound);
  server.begin();
  Serial.println("HTTP server started.");
}

void setupWebSocket()
{
  //webSocket.begin();
  //webSocket.onEvent(webSocketEvent);
}

void mqttCallback(char *topic, byte *payload, unsigned int length)
{
  hotTubMqtt.callback(topic, payload, length);
}

void setupSPIFFS()
{
  SPIFFS.begin();
}

void enableInterrupts()
{
  attachInterrupt(digitalPinToInterrupt(BUTTON), handleButtonPress, FALLING);
  attachInterrupt(digitalPinToInterrupt(DATA_IN), handleDataInterrupt, CHANGE);
}

void setup(void)
{
  Serial.begin(115200);
  Serial.println();
  Serial.println();
  Serial.println("Starting HotTub Remote...");

  setupBoard();
  setupIotWebConf();
  setupWebServer();
  setupWebSocket();
  setupSPIFFS();
  hotTub.setup(onStateChange);
  hotTubApi.setup();
  hotTubMqtt.setup(*mqttCallback);

  enableInterrupts();
}

void loop(void)
{
  ArduinoOTA.handle();
  iotWebConf.doLoop(); // doLoop should be called as frequently as possible.
  //webSocket.loop();
  server.handleClient();

  //testCommandCheck();

  hotTubMqtt.loop();
  hotTub.loop();
}

void testCommandCheck()
{
  if (sendTestCommand)
  {
    sendTestCommand = false;

    Serial.println("Button Pressed!");
  }
}
