#include <ArduinoJson.h>
#include <IotWebConf.h>
#include <WiFiClient.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <SPI.h>

//D1 Mini
#define LEDPIN 16
#define DATAINPIN D1
#define DATAOUTPIN D2
#define DBGPIN D8

#define MIN_TEMP 9 //Minimum temperature supported by the hot tub
#define MAX_TEMP 40 //Maximum temperature supported by the hot tub

// -- Initial name of the Thing. Used e.g. as SSID of the own Access Point.
const char thingName[] = "HotTubRemote";
// -- Initial password to connect to the Thing, when it creates an own Access Point.
const char wifiInitialApPassword[] = "HotTubRemote";

DNSServer dnsServer;
ESP8266WebServer server(80);
IotWebConf iotWebConf(thingName, &dnsServer, &server, wifiInitialApPassword);
 

int autoRestart = 0;

//these are the current states (decoded from the serial stream)
int pumpState = 0;
int blowerState = 0;
int heaterState = 0;
int temperature = 0;

//desired water temperature
int targetTemperature = 37;












//these are the widths of the pulses
#define ignoreInvalidStartPulse true  //enable this to allow testing without a valid waveform
#define startPulse 4440               //width of the start pulse
#define startPulseTolerance 100
#define startPulseMin startPulse - startPulseTolerance
#define startPulseMax startPulse + startPulseTolerance
#define dataStartDelay 200            //amount of time after start pulse has gone low to wait before sampling the bits
#define bitPulse 440                  //width of one bit
#define thirdBitPulse bitPulse / 3
#define buttonWidth 147               //width of the low pulse when a button is pressed
#define bitCount 14                   //number of bits to read in
#define datamask 1 << bitCount





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

bool ledState = true;

void disableDataInterrupt() {
  detachInterrupt(digitalPinToInterrupt(DATAINPIN));
}

void enableDataInterrupt() {
  attachInterrupt(digitalPinToInterrupt(DATAINPIN), handleDataInterrupt, RISING);
}

void handleDataInterrupt() {
  disableDataInterrupt();

  digitalWrite(DBGPIN, HIGH); //Debug high while checking start pulse length

  //here the first long pulse has gone high
  unsigned long startTime;
  unsigned long endTime;
  unsigned long pulseDuration;

  //start timing first pulse
  startTime = micros();

  //Serial.print("Start Time is ");
  //Serial.println(startTime);

  //wait for the first long pulse to go low
  while (digitalRead(DATAINPIN) && pulseDuration < startPulseMax)
  { 
    endTime = micros(); //update endTime to check if we have detected a too long pulse
    pulseDuration = endTime - startTime;
  }
  digitalWrite(DBGPIN, LOW); //Debug low after checking start pulse length

  //Serial.print("End time is ");
  //Serial.println(endTime);


  
  //if start pulse wasn't the correct duration, get out
  if (pulseDuration < startPulseMin || pulseDuration > startPulseMax) {

    Serial.print("Timed out detecting start pulse @ ");
    Serial.println(pulseDuration);
    
    if (!ignoreInvalidStartPulse) {
      enableDataInterrupt();
      return;
    }
  }

  //here we have confirmed that the start pulse was ~4440us and the signal is now low (Start Confirm on diagram)

  delayMicroseconds(dataStartDelay);

  // (Button detect on diagram)

  word data;

  //Get the bits  
  for (word i=0x2000; i > 0; i >>= 1)
  {
    delayMicroseconds(thirdBitPulse);
    digitalWrite(DBGPIN, HIGH); //Debug signal high whilst checking the bit
    
    word  noti = ~i;
    if (digitalRead(DATAINPIN))
      data |= i;
    else // else clause added to ensure function timing is ~balanced
      data &= noti;

    delayMicroseconds(thirdBitPulse);
    digitalWrite(DBGPIN, LOW); //Debug signal low after checking the bit
    delayMicroseconds(thirdBitPulse);
  }


  digitalWrite(DBGPIN, LOW); //Debug signal high after checking button is pressed
  
  //End getting the bits



  enableDataInterrupt();
}



void setup(void)
{
  pinMode(LEDPIN, OUTPUT);    
  pinMode(DATAINPIN, INPUT);
  pinMode(DATAOUTPIN, OUTPUT);
  pinMode(DBGPIN, OUTPUT);
  enableDataInterrupt();
  
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
