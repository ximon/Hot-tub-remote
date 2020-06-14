//if mqtt doesnt send the lib may need pathcing
//todo - test waiting ~20ms after a comand has ended to send any commands

//todo - option to pull line high for a short period to reset disiplay controller on error?

//mqtt sending status doesnt seem to work anymore

//need to check target state changing and autorestart

//target temperature changes seem a bit flakey
//picking up which temperature is which is still a bit flaky when sending temperature buttons

//need to somehow ignore first button press so it doesn;t decrease temperature when it shouldnt
//  maybe only do this if the state is flashing??

//temperature lock - ok
//auto restart -

#define HOSTNAME "HotTubRemote"

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
#include "HotTub.h"
#include "HotTubApi.h"
#include "HotTubMqtt.h"
#include "Pins.h"

#include <syslog.h>

// -- Initial name of the Thing. Used e.g. as SSID of the own Access Point.
const char thingName[] = "HotTubRemote";
// -- Initial password to connect to the Thing, when it creates an own Access Point.
const char wifiInitialApPassword[] = "HotTubRemote";

#define CONFIG_VERSION "beta4"

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
IotWebConfParameter mqttPortParam = IotWebConfParameter("Port", "mqttPort", mqttPort, NUMBER_LEN, "number", "1883", NULL, "min='1' max='65535' step='1'");
IotWebConfParameter mqttUserParam = IotWebConfParameter("User", "mqttUser", mqttUser, STRING_LEN);
IotWebConfParameter mqttPassParam = IotWebConfParameter("Password", "mqttPass", mqttPass, STRING_LEN);

// Syslog server connection info
#define SYSLOG_SERVER "255.255.255.255" //broadcast
#define SYSLOG_PORT 514

// This device info
#define DEVICE_HOSTNAME "HotTubRemote"
#define APP_NAME "-"
WiFiUDP udpClient;
Syslog logger(udpClient, SYSLOG_PROTO_IETF);

HotTub hotTub(DATA_IN, DATA_ENABLE, DBG, &logger);
HotTubApi hotTubApi(&server, &hotTub, &logger);
HotTubMqtt hotTubMqtt(&hotTub, &logger);

bool justStarted = true;

void setupSyslog()
{
  logger.server(SYSLOG_SERVER, SYSLOG_PORT);
  logger.deviceHostname(DEVICE_HOSTNAME);
  logger.appName(APP_NAME);
  logger.defaultPriority(LOG_DEBUG);
}

bool OTASetup = false;
bool sendTestCommand = false;

void onStateChange(const char *reason)
{
  logger.logf("MAIN->State changed - %s", reason);
  hotTubMqtt.sendStatus();

  webSocket.broadcastTXT(hotTub.getStateJson());
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

void webSocketEvent(uint8_t num, WStype_t type, uint8_t *payload, size_t length)
{
  if (type == WStype_DISCONNECTED)
  {
    logger.logf("WS->[%u] Disconnected!\n", num);
  }
  else if (type == WStype_CONNECTED)
  {
    IPAddress ip = webSocket.remoteIP(num);
    logger.logf("WS->[%u] Connected from %d.%d.%d.%d url: %s\n", num, ip[0], ip[1], ip[2], ip[3], payload);
    webSocket.sendTXT(num, hotTub.getStateJson());
  }
  else if (type == WStype_TEXT)
  {
    logger.logf("WS->[%u] get Text: %s\n", num, payload);
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
}

void setupMqtt()
{
  if (strlen(mqttServer) > 0)
    hotTubMqtt.setServer(mqttServer, atoi(mqttPort));

  if (strlen(mqttUser) > 0)
    hotTubMqtt.setCredentials(&mqttUser[0], &mqttPass[0]);
}

const char *otaErrorToString(ota_error_t error)
{
  switch (error)
  {
  case OTA_AUTH_ERROR:
    return "Auth Failed";
  case OTA_BEGIN_ERROR:
    return "Begin Failed";
  case OTA_CONNECT_ERROR:
    return "Connect Failed";
  case OTA_RECEIVE_ERROR:
    return "Receive Failed";
  case OTA_END_ERROR:
    return "End Failed";
  default:
    return "";
  }
}

static int lastOTAPercent;
void setupOTA()
{
  ArduinoOTA.setHostname(HOSTNAME);
  ArduinoOTA.onStart([]() {
    const char *type = ArduinoOTA.getCommand() == U_FLASH ? "sketch" : "filesystem";
    // NOTE: if updating FS this would be the place to unmount FS using FS.end()
    logger.logf("OTA->Start updating %s", type);
  });
  ArduinoOTA.onEnd([]() { logger.logf("OTA->End"); });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    int percent = (progress / (total / 100));
    if (lastOTAPercent == percent || percent % 5 != 0)
      return;

    logger.logf("OTA->Progress: %u%%", percent);
    lastOTAPercent = percent;
  });
  ArduinoOTA.onError([](ota_error_t error) {
    logger.logf("OTA->Error: %s", otaErrorToString(error));
  });
  ArduinoOTA.begin();

  logger.log("OTA->Setup complete.");
}

void sendConnected()
{
  logger.log("MAIN->HotTubRemote Connected");
}

void sendStartInfo()
{
  justStarted = false;
  logger.log("MAIN->##############################");
  logger.log("MAIN->##     HOTTUB  STARTED      ##");
  logger.log("MAIN->##############################");
  logger.logf("MAIN->Uptime: %lu s", millis() / 1000);

  struct rst_info *rst = system_get_rst_info();
  const char *reasons[] = {"Normal Startup", "Hardware WDT", "Exception", "Software WDT", "Soft Restart", "Deep Sleep Awake", "External System Reset"};

  logger.logf("MAIN->Restart Reason: %s (%i)", reasons[rst->reason], rst->reason);

  if (rst->exccause > 0)
  {
    logger.logf("MAIN->excCause: %i, excVaddr: %i, epc1: %i, epc2: %i, epc3: %i, depc: %i", rst->exccause, rst->excvaddr, rst->epc1, rst->epc2, rst->epc3, rst->depc);
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
  logger.log("MAIN->HTTP server started.");
}

void setupWebSocket()
{
  webSocket.begin();
  webSocket.onEvent(webSocketEvent);
}

void mqttCallback(char *topic, byte *payload, unsigned int length)
{
  hotTubMqtt.callback(topic, payload, length);
}

void enableInterrupts()
{
  attachInterrupt(digitalPinToInterrupt(DATA_IN), handleDataInterrupt, CHANGE);
}

void setup(void)
{
  Serial.begin(115200);

  setupBoard();
  setupIotWebConf();
  setupWebServer();
  setupWebSocket();
  hotTub.setup(onStateChange);
  hotTubApi.setup();
  hotTubMqtt.setup(*mqttCallback);

  enableInterrupts();
}

void loop(void)
{
  ArduinoOTA.handle();
  iotWebConf.doLoop(); // doLoop should be called as frequently as possible.
  webSocket.loop();
  server.handleClient();

  hotTubMqtt.loop();
  hotTub.loop();
}
