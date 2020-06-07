//temperature lock - ok
//temperature limit - ok
//changing temp via api is broken?
//auto restart - tbd

//picking up temperature changes is still a touch flaky when changing via the pump buttons
//todo - test waiting ~20ms after a comand has ended to send any commands, display goes a bit funky sometimes, suspect this may be the cause?
//todo - option to pull line high for a short period to reset display controller on error?

#define HOSTNAME "HotTubRemote"
#define EEPROM_START_LOC 480

#include <ArduinoJson.h>
#include <IotWebConf.h>
#include <DNSServer.h>
#include <WiFiClient.h>
#include <ESP8266WiFi.h>
#include <esp8266_peri.h>
#include <WebSocketsServer.h>
#include <ESP8266mDNS.h>
#include <ArduinoOTA.h>
#include <SPI.h>
#include <EEPROM.h>
#include "HotTub.h"
#include "HotTubApi.h"
#include "HotTubMqtt.h"
#include "Pins.h"

#include <WiFiUdp.h>
#include <Syslog.h>
#include <stdarg.h>

// -- Initial name of the Thing. Used e.g. as SSID of the own Access Point.
const char thingName[] = "HotTubRemote";
// -- Initial password to connect to the Thing, when it creates an own Access Point.
const char wifiInitialApPassword[] = "HotTubRemote";

#define CONFIG_VERSION "RC1"

DNSServer dnsServer;
ESP8266WebServer server(80);
WebSocketsServer webSocket = WebSocketsServer(81);
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
IotWebConfParameter mqttPortParam = IotWebConfParameter("Port", "mqttPort", mqttPort, NUMBER_LEN, "number", "", NULL, "min='1' max='65535' step='1'");
IotWebConfParameter mqttUserParam = IotWebConfParameter("User", "mqttUser", mqttUser, STRING_LEN);
IotWebConfParameter mqttPassParam = IotWebConfParameter("Password", "mqttPass", mqttPass, STRING_LEN);

HotTub hotTub(DATA_IN, DATA_ENABLE, DBG);
HotTubApi hotTubApi(&server, &hotTub);
HotTubMqtt hotTubMqtt(&hotTub);

bool settingFromEEPROM;

bool justStarted = true;

// Syslog server connection info
#define SYSLOG_SERVER "255.255.255.255"
#define SYSLOG_PORT 514

// This device info
#define DEVICE_HOSTNAME "HotTubRemote"
#define APP_NAME "-"
WiFiUDP udpClient;
Syslog syslog(udpClient, SYSLOG_PROTO_IETF);

void debug(char *message);
void debugf(char *msg, ...);

void sendStartInfo();
void sendConnected();

void setupSyslog()
{
  syslog.server(SYSLOG_SERVER, SYSLOG_PORT);
  syslog.deviceHostname(DEVICE_HOSTNAME);
  syslog.appName(APP_NAME);
  syslog.defaultPriority(LOG_INFO);
}

void debug(char *msg)
{
  syslog.log(LOG_INFO, msg);
}

void debugf(char *msg, ...)
{
  va_list args;
  va_start(args, msg);
  syslog.vlogf(LOG_INFO, msg, args);
  va_end(args);
}

struct SaveData
{
  int targetTemperature;
  int targetState;
  int limitTemperature;
  bool autoRestartEnabled;
  bool temperatureLockEnabled;
} eepromSaveData;

void updateEEPROM()
{
  bool changed;

  if (hotTub.getTargetState()->targetTemperature != eepromSaveData.targetTemperature)
  {
    eepromSaveData.targetTemperature = hotTub.getTargetState()->targetTemperature;
    changed = true;
  }

  if (hotTub.getTargetState()->pumpState != eepromSaveData.targetState)
  {
    eepromSaveData.targetState = hotTub.getTargetState()->pumpState;
    changed = true;
  }

  if (hotTub.getLimitTemperature() != eepromSaveData.limitTemperature)
  {
    eepromSaveData.limitTemperature = hotTub.getLimitTemperature();
    changed = true;
  }

  if (hotTub.getAutoRestart() != eepromSaveData.autoRestartEnabled)
  {
    eepromSaveData.autoRestartEnabled = hotTub.getAutoRestart();
    changed = true;
  }

  if (!changed)
    return;

  Serial.println("Updating EEPROM...");

  EEPROM.put(EEPROM_START_LOC, eepromSaveData);
  EEPROM.commit();
}

void setStateFromEEPROM()
{
  debug("Getting state from EEPROM");

  Serial.print("EEPROM Auto Restart: ");
  Serial.println(eepromSaveData.autoRestartEnabled);

  Serial.print("EEPROM Limit Temperature: ");
  Serial.println(eepromSaveData.limitTemperature);

  Serial.print("EEPROM Target Temperature: ");
  Serial.println(eepromSaveData.targetTemperature);

  Serial.print("EEPROM Target State: ");
  Serial.println(eepromSaveData.targetState);

  settingFromEEPROM = true;
  hotTub.setAutoRestart(eepromSaveData.autoRestartEnabled);
  hotTub.setLimitTemperature(eepromSaveData.limitTemperature);
  hotTub.setTargetTemperature(eepromSaveData.targetTemperature);
  hotTub.setTargetState(eepromSaveData.targetState);
  settingFromEEPROM = false;
}

void onStateChange(char *reason)
{
  if (settingFromEEPROM)
    return;

  updateEEPROM();

  debugf("State Changed - %s", reason);

  hotTubMqtt.sendStatus();
  webSocket.broadcastTXT(hotTub.getStateJson());
}

void handle_root()
{
  // -- Captive portal request were already served.
  if (iotWebConf.handleCaptivePortal())
    return;

  String html = "<!DOCTYPE html><html lang=\"en\"><head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1, user-scalable=no\"/>";
  html += "<title>Hot Tub Remote</title></head><body><h3>Hot Tub Remote<h3>";
  html += "<p>Go to <a href='config'>configure page</a> to change settings.</p>";
  html += "<p>Go to <a href='status'>status</a> to view status.</p>";
  html += "</body></html>\n";

  server.send(200, "text/html", html);
}

volatile bool iStarted;
volatile unsigned long iStart;
volatile unsigned long iEnd;

void ICACHE_RAM_ATTR handleDataInterrupt()
{
  iStarted = true;
  iStart = micros();
  hotTub.dataInterrupt();
  iEnd = micros();
}

void webSocketEvent(uint8_t num, WStype_t type, uint8_t *payload, size_t length)
{
  if (type == WStype_DISCONNECTED)
  {
    //Serial.printf("[%u] Disconnected!\n", num);
  }
  else if (type == WStype_CONNECTED)
  {
    //IPAddress ip = webSocket.remoteIP(num);
    //Serial.printf("[%u] Connected from %d.%d.%d.%d url: %s\n", num, ip[0], ip[1], ip[2], ip[3], payload);
    webSocket.sendTXT(num, hotTub.getStateJson());
  }
  else if (type == WStype_TEXT)
  {
    //Serial.printf("[%u] get Text: %s\n", num, payload);
    webSocket.sendTXT(num, "Received!");
  }
}

void setupBoard()
{
  pinMode(DBG, OUTPUT);
  pinMode(LED, OUTPUT);
  pinMode(DATA_IN, INPUT);
  pinMode(DATA_OUT, OUTPUT); // Override default Serial initiation
  pinMode(DATA_ENABLE, OUTPUT);
  digitalWrite(DATA_OUT, 0);
  digitalWrite(DATA_ENABLE, LOW);

  WiFi.hostname(HOSTNAME);
  WiFi.setSleepMode(WIFI_NONE_SLEEP);

  EEPROM.begin(512);
  EEPROM.get(EEPROM_START_LOC, eepromSaveData);
}

void setupMqtt()
{
  if (strlen(mqttServer) > 0)
    hotTubMqtt.setServer(mqttServer, atoi(mqttPort));

  if (strlen(mqttUser) > 0)
    hotTubMqtt.setCredentials(&mqttUser[0], &mqttPass[0]);
}

void setupOTA()
{
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

void sendConnected()
{
  debug("HotTubRemote Connected");
}

void sendStartInfo()
{
  justStarted = false;
  debug("##############################");
  debug("##     HOTTUB  STARTED      ##");
  debug("##############################");
  debugf("Uptime: %is", millis() / 1000);

  struct rst_info *rst = system_get_rst_info();
  const char *reasons[] = {"Normal Startup", "Hardware WDT", "Exception", "Software WDT", "Soft Restart", "Deep Sleep Awake", "External System Reset"};

  debugf("Restart Reason: %s (%i)", reasons[rst->reason], rst->reason);

  if (rst->exccause > 0)
  {
    debugf("  excCause: %i, excVaddr: %i, epc1: %i, epc2: %i, epc3: %i, depc: %i", rst->exccause, rst->excvaddr, rst->epc1, rst->epc2, rst->epc3, rst->depc);
  }
}

void wifiConnected()
{
  setupSyslog();
  setupOTA();

  if (justStarted)
    sendStartInfo();

  sendConnected();

  if (strlen(mqttServer) > 0)
    setupMqtt();
}

void configChanged()
{
  setupMqtt();
}

void setupIotWebConf()
{
  iotWebConf.setStatusPin(LED);
  iotWebConf.skipApStartup();
  iotWebConf.setupUpdateServer(&httpUpdater);
  iotWebConf.setWifiConnectionCallback(wifiConnected);
  iotWebConf.setConfigSavedCallback(configChanged);

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
  webSocket.begin();
  webSocket.onEvent(webSocketEvent);
  Serial.println("WebSocket server started");
}

void mqttCallback(char *topic, byte *payload, unsigned int length)
{
  hotTubMqtt.callback(topic, payload, length);
}

void onSetDataInterrupt(bool state)
{
  if (state)
    attachInterrupt(digitalPinToInterrupt(DATA_IN), handleDataInterrupt, CHANGE);
  else
    detachInterrupt(digitalPinToInterrupt(DATA_IN));
}

void enableInterrupts()
{
  onSetDataInterrupt(true);
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
  hotTub.setup(onSetDataInterrupt, onStateChange, debug, debugf);
  hotTubApi.setup();
  hotTubMqtt.setup(*mqttCallback);

  enableInterrupts();

  setStateFromEEPROM();
}

void loop(void)
{
  ArduinoOTA.handle();
  iotWebConf.doLoop(); // doLoop should be called as frequently as possible.
  webSocket.loop();
  server.handleClient();

  hotTubMqtt.loop();
  hotTub.loop();

  if (iStarted)
  {
    iStarted = false;
    int interruptLength = iEnd - iStart;

    if (interruptLength > 3)
      debugf("MAIN->Interrupt took %ius", interruptLength);
  }
}
