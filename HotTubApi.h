#ifndef HOTTUBAPI_H
#define HOTTUBAPI_H

#include <Arduino.h>

#include <ArduinoJson.h>
#include "RouteHandler.h"
#include "HotTub.h"
#include <ESP8266WebServer.h>

class HotTubApi {
  public:
    HotTubApi(ESP8266WebServer* webServer, HotTub* hotTubInstance);
    void setup();
    
  protected:
    ESP8266WebServer* server;
    HotTub* hotTub;
    
    /*
     * Helper functions
     */
     
    String toOnOff(bool value) {
      return value ? "on" : "off";
    }
    
    void valueNotProvided(String argumentName) {
      String output = "No " + argumentName + " provided";
      errorMessage(output);
      Serial.println(output);  
    }

    bool checkArgument(String argumentName) {
      if (!server->hasArg(argumentName)) {
        valueNotProvided(argumentName);
        return true;
      }
    
      return false;
    }
        
    bool valueIsTrue(String argumentName) {
      String value = server->arg(argumentName);
      value.trim();
      value.toLowerCase();
      
      return value == "on" || value == "1" || value == "true" || value == "yes";
    }

    
    /*
     * Request Handlers
     */
    
    void handle_status() {
      const size_t capacity = 2*JSON_OBJECT_SIZE(5) + JSON_OBJECT_SIZE(7);
      DynamicJsonDocument doc(capacity);
      String json;

      HotTubState* currentState = hotTub->getCurrentState();
      JsonObject target = doc.createNestedObject("targetState");
      target["pump"] = currentState->pumpEnabled;
      target["heater"] = currentState->heaterEnabled;
      target["heating"] = currentState->heaterHeating;
      target["blower"] = currentState->blowerEnabled;
      target["temperature"] = currentState->temperature;

      HotTubState* targetState = hotTub->getTargetState();
      JsonObject current = doc.createNestedObject("currentState");
      current["pump"] = targetState->pumpEnabled;
      current["heater"] = targetState->heaterEnabled;
      current["heating"] = targetState->heaterHeating;
      current["blower"] = targetState->blowerEnabled;
      current["temperature"] = targetState->temperature;
      
      doc["autoRestart"] = hotTub->isAutoRestartEnabled();
      doc["panelLock"] = hotTub->isPanelLockEnabled();
      doc["maxTemperature"] = hotTub->getMaximumTemperature();
      doc["temperatureLock"] = hotTub->isTemperatureLockEnabled();
      doc["errorCode"] = hotTub->getErrorCode();
    
      serializeJson(doc, json);
      okJson(json);
    }
    
    void handle_autoRestart() {
      if (checkArgument("state")) 
        return;

      bool enable = valueIsTrue("state");
      String msg = "Auto restart is " + toOnOff(enable);

      if (hotTub->isAutoRestartEnabled() != enable) {
        hotTub->autoRestart(enable);
      } else {
        msg = "Auto restart is already " + toOnOff(enable);
      }
      
      okMessage(msg);
      Serial.println(msg);
    }
    
    void handle_pump() {
      if (checkArgument("state"))
        return;

      bool enable = valueIsTrue("state");
      String msg = "Sending pump command";
      
      if (hotTub->getCurrentState()->pumpEnabled != enable) {
        hotTub->getTargetState()->pumpEnabled = enable;
      } else {
        msg = "Pump is already " + toOnOff(enable);
      }
      
      okMessage(msg);
      Serial.println(msg);
    }
    
    void handle_heater() {
      if (checkArgument("state"))
        return;

      bool enable = valueIsTrue("state");
      String msg = "Sending heater command";
      
      if (hotTub->getCurrentState()->heaterEnabled != enable) {
        hotTub->getTargetState()->heaterEnabled = enable;
      } else {
        msg = "Heater is already " + toOnOff(enable);
      }
      
      okMessage(msg);
      Serial.println(msg);
    }
    
    void handle_blower() {
      if (checkArgument("state"))
        return;

      bool enable = valueIsTrue("state");
      String msg = "Sending blower command";
      
      if (hotTub->getCurrentState()->blowerEnabled != enable) {
        hotTub->getTargetState()->blowerEnabled = enable;
      } else {
        msg = "Blower is already " + toOnOff(enable);
      }
        
      okMessage(msg);
      Serial.println(msg);
    }
      
    void handle_temperature() {
      if (checkArgument("value")) 
        return;
          
      int temp = server->arg("value").toInt();
      
      if (!temperatureValid(temp, hotTub->getMaximumTemperature()))
        return;
      
      hotTub->getTargetState()->temperature = temp;
      setTempMessage(temp, "target temperature");
    }
   
    void handle_maxTemperature() {
      if (checkArgument("value"))
        return;
      
      int maxTemp = server->arg("value").toInt();
     
      if (!temperatureValid(maxTemp, MAX_TEMP))
        return;
      
      hotTub->maximumTemperature(maxTemp, true);
    
      String msg = "max temperature";

      //todo - move this into HotTub.h
      if (hotTub->getCurrentState()->temperature > maxTemp) {
        hotTub->maximumTemperature(maxTemp, true);
        msg += " and target temperature";
      }
      
      setTempMessage(maxTemp, msg);
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

    void handle_rawCommand() {
      if (checkArgument("command"))
        return;
    
      String commandStr =  server->arg("command");
      commandStr.toLowerCase();
      word command = strtol(commandStr.c_str(), 0, 16);
    
      if (command = 0) {
        errorMessage("Commands must be in the format 0x1234");
        return;
      } else if (command >= 0x4000) {
        errorMessage("Commands must be less than 0x4000");
        return;
      }
    
      hotTub->sendCommand(command);
      String msg = "Sending command " + commandStr;
      okMessage(msg);
    }
    
    void handle_panelLock() {
      if (checkArgument("state"))
        return;

      bool enable = valueIsTrue("state");
      String msg = "Panel lock is ";
      
      if (hotTub->isPanelLockEnabled() != enable) {
        hotTub->panelLock(enable);
      } else {
        msg += "already ";
      }

      msg += toOnOff(enable);
      
      okMessage(msg);
      Serial.println(msg);
    }
    
    void handle_temperatureLock() {
      if (checkArgument("state")) 
        return;

      bool enable = valueIsTrue("state");
      String msg = "Temperature lock is ";
      
      if (hotTub->isTemperatureLockEnabled() != enable) {
        hotTub->temperatureLock(enable);
      }else {
        msg += "already ";
      }
    
      msg += toOnOff(enable);
      
      okMessage(msg);
      Serial.println(msg);
    }

    /*
     * Http Helper functions
     */
    
    void addCorsHeaders() {
      server->sendHeader("Access-Control-Allow-Origin", "*");
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
      server->send(200, "application/json", json);
    }
    
    //outputs the plain text
    void okText(String message) {
      server->send(200, "text/plain", message);
    }
    
    //sends the error message wrapped in a json object
    void errorMessage(String message) {
      server->send(400, "application/json", toJson(message));
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
};

#endif
