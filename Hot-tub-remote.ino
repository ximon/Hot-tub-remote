    //todo - test waiting ~20ms after a comand has ended to send any commands

//getting target temp at startup works
//temperature lock works
//adjusting target temp via buttons works
//adjusting target temp externally works
//need to check target state changing and autorestart

#define HOSTNAME "HotTubRemote"

#include <ArduinoJson.h>
#include <IotWebConf.h>
#include <WiFiClient.h>
#include <ESP8266WiFi.h>
#include <esp8266_peri.h>
#include <ESP8266WebServer.h>
#include <WebSocketsServer.h>
#include <ArduinoOTA.h>
#include <SPI.h>
#include <FS.H>
#include "HotTub.h"
#include "HotTubApi.h"
#include "Pins.h"


// -- Initial name of the Thing. Used e.g. as SSID of the own Access Point.
const char thingName[] = "HotTubRemote";
// -- Initial password to connect to the Thing, when it creates an own Access Point.
const char wifiInitialApPassword[] = "HotTubRemote";

#define CONFIG_VERSION "beta2"

DNSServer dnsServer;
ESP8266WebServer server(80);
WebSocketsServer webSocket(81);
HTTPUpdateServer httpUpdater;
IotWebConf iotWebConf(thingName, &dnsServer, &server, wifiInitialApPassword, CONFIG_VERSION);

File fsUploadFile;

HotTub hotTub(DATA_IN, DATA_ENABLE, DBG);
HotTubApi hotTubApi(&server, &hotTub);

bool sendTestCommand = false;

void onStateChange(int oldState, int newState) {
  char data[100];
  sprintf(data, "State changed from %d to %d", oldState, newState);
  webSocket.broadcastTXT(data);
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

void ICACHE_RAM_ATTR handleDataInterrupt() {
  hotTub.dataInterrupt();
}

void ICACHE_RAM_ATTR handleButtonPress() {
  sendTestCommand = true;
}

void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length) {
  switch (type) {
    case WStype_DISCONNECTED:             // if the websocket is disconnected
      Serial.printf("[%u] Disconnected!\n", num);
      break;
    case WStype_CONNECTED: {              // if a new websocket connection is established
        IPAddress ip = webSocket.remoteIP(num);
        Serial.printf("[%u] Connected from %d.%d.%d.%d url: %s\n", num, ip[0], ip[1], ip[2], ip[3], payload);
      }
      break;
    case WStype_TEXT:                     // if new text data is received
      Serial.printf("[%u] get Text: %s\n", num, payload);

      webSocket.sendTXT(num, "Received!");
      //webSocket.broadcastTXT("message here");

      break;
  }  
}

String getContentType(String filename) { // determine the filetype of a given filename, based on the extension
  if (filename.endsWith(".html")) return "text/html";
  else if (filename.endsWith(".css")) return "text/css";
  else if (filename.endsWith(".js")) return "application/javascript";
  else if (filename.endsWith(".ico")) return "image/x-icon";
  else if (filename.endsWith(".gz")) return "application/x-gzip";
  return "text/plain";
}

bool handleFileRead(String path) { // send the right file to the client (if it exists)
  Serial.println("handleFileRead: " + path);
  if (path.endsWith("/")) path += "index.html";          // If a folder is requested, send the index file
  String contentType = getContentType(path);             // Get the MIME type
  String pathWithGz = path + ".gz";
  if (SPIFFS.exists(pathWithGz) || SPIFFS.exists(path)) { // If the file exists, either as a compressed archive, or normal
    if (SPIFFS.exists(pathWithGz))                         // If there's a compressed version available
      path += ".gz";                                         // Use the compressed verion
    File file = SPIFFS.open(path, "r");                    // Open the file
    size_t sent = server.streamFile(file, contentType);    // Send it to the client
    file.close();                                          // Close the file again
    Serial.println(String("\tSent file: ") + path);
    return true;
  }
  Serial.println(String("\tFile Not Found: ") + path);   // If the file doesn't exist, return false
  return false;
}

String formatBytes(size_t bytes) { // convert sizes in bytes to KB and MB
  if (bytes < 1024) {
    return String(bytes) + "B";
  } else if (bytes < (1024 * 1024)) {
    return String(bytes / 1024.0) + "KB";
  } else if (bytes < (1024 * 1024 * 1024)) {
    return String(bytes / 1024.0 / 1024.0) + "MB";
  }
}

void handleFileUpload(){ // upload a new file to the SPIFFS
  HTTPUpload& upload = server.upload();
  String path;
  if(upload.status == UPLOAD_FILE_START){
    path = upload.filename;
    if(!path.startsWith("/")) path = "/"+path;
    if(!path.endsWith(".gz")) {                          // The file server always prefers a compressed version of a file 
      String pathWithGz = path+".gz";                    // So if an uploaded file is not compressed, the existing compressed
      if(SPIFFS.exists(pathWithGz))                      // version of that file must be deleted (if it exists)
         SPIFFS.remove(pathWithGz);
    }
    Serial.print("handleFileUpload Name: "); Serial.println(path);
    fsUploadFile = SPIFFS.open(path, "w");            // Open the file for writing in SPIFFS (create if it doesn't exist)
    path = String();
  } else if(upload.status == UPLOAD_FILE_WRITE){
    if(fsUploadFile)
      fsUploadFile.write(upload.buf, upload.currentSize); // Write the received bytes to the file
  } else if(upload.status == UPLOAD_FILE_END){
    if(fsUploadFile) {                                    // If the file was successfully created
      fsUploadFile.close();                               // Close the file again
      Serial.print("handleFileUpload Size: "); Serial.println(upload.totalSize);
      server.sendHeader("Location","/success.html");      // Redirect the client to the success page
      server.send(303);
    } else {
      server.send(500, "text/plain", "500: couldn't create file");
    }
  }
}

void setupBoard() {
  pinMode(BUTTON, INPUT);
  pinMode(DATA_IN, INPUT);
  pinMode(DBG, OUTPUT);
  pinMode(LED, OUTPUT);
  pinMode(DATA_ENABLE, OUTPUT);
 
  iotWebConf.setStatusPin(LED);
  //iotWebConf.skipApStartup();
  iotWebConf.setupUpdateServer(&httpUpdater);
  iotWebConf.init();
}

void setupOTA() {
  // Set Hostname
  //String hostname();
  WiFi.hostname(HOSTNAME);
  
  ArduinoOTA.setHostname(HOSTNAME);
  ArduinoOTA.onStart([]() { Serial.println("Start"); });
  ArduinoOTA.onEnd([]() { Serial.println("\nEnd"); });
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

void handleNotFound() {  
  if (!handleFileRead(server.uri())) {
    iotWebConf.handleNotFound();
  }
}

void setupWebServer() {
  // -- Set up required URL handlers on the web server.
  server.on("/", handle_root);
  server.on("/config", []{ iotWebConf.handleConfig(); });
  server.on("/edit.html", HTTP_POST, []() {
    server.send(200, "text/plain", "");
    }, handleFileUpload);
  server.onNotFound(handleNotFound);
  server.begin();
  Serial.println("HTTP server started.");
}

void setupWebSocket() {
  webSocket.begin();
  webSocket.onEvent(webSocketEvent);
}

void setupSPIFFS() {
  SPIFFS.begin();
}

void enableInterrupts() {  
  attachInterrupt(digitalPinToInterrupt(BUTTON), handleButtonPress, RISING);
  attachInterrupt(digitalPinToInterrupt(DATA_IN), handleDataInterrupt, CHANGE);
}

void setup(void)
{
  Serial.begin(115200);
  Serial.println();
  Serial.println();
  Serial.println("Starting...");
  
  setupBoard();

  setupWebServer();
  setupWebSocket();

  setupSPIFFS();
  setupOTA();

  
  pinMode(3, OUTPUT); // Override default Serial initiation
  digitalWrite(3,0); // GPIO3 = I2S data out
  pinMode(D2, OUTPUT);
  digitalWrite(D2, LOW);
  hotTub.setup(onStateChange);
  hotTubApi.setup();
  
  enableInterrupts();
}

void loop(void)
{  
  iotWebConf.doLoop(); // doLoop should be called as frequently as possible.
  webSocket.loop();
  server.handleClient();
  ArduinoOTA.handle();
  
  testCommandCheck();
  
  hotTub.loop();
} 

void testCommandCheck() {
  if (sendTestCommand) {
    sendTestCommand = false;
    
    Serial.println("Button Pressed!");
    hotTub.autoRestartEnabled = !hotTub.autoRestartEnabled;
    //onStateChange(0,1);
  }
}
