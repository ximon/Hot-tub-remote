#ifndef HOTTUB_H
#define HOTTUB_H

#include <ArduinoJson.h>
#include "Commands.h"
#include "Temperatures.h"
#include "SendReceive.h"

#define INIT_TEMP 37 //Temperature to initially set to when powering up.

#define WAIT_BETWEEN_SENDING_AUTO_COMMANDS 500
#define WAIT_BEFORE_INITIAL_TARGET_TEMP_CHECK 5000 //milliseconds after startup to wait before checking the initial target temperature

#define DELAY_BETWEEN_PRINTS 1000

#define TEMP_BUTTON_DELAY 5000 //time to wait after a button is pressed to run the target temperature check
#define TEMP_TARGET_INITIAL_DELAY 6000 //Time to wait before sending a button command to get the target temperature
#define TEMP_CURRENT_DELAY 6000 //Time to wait before taking the reading as the current temperature
#define TEMP_TARGET_DELAY 250 //Time to wait before taking the reading as the target temperature

#define TEMP_ADJUST_DELAY 3000 //must be less than TEMP_IGNORE_TIME!!
#define BUTTON_SEND_DELAY 3000 //time to wait before sending buttons to change state


//todo - extract Serial.print lines to a debug function that can be handled externally.

#define PUMP_UNKNOWN -1 //haven't received a status command yet
#define PUMP_OFF 0
#define PUMP_FILTERING 1
#define PUMP_HEATER_STANDBY 2
#define PUMP_HEATING 3
#define PUMP_BUBBLES 4
#define PUMP_END 5
#define PUMP_ERROR 6

struct CurrentState {
  int pumpState;            //Pump's current
  int temperature;          //Pump's current actual water temperature
  int targetTemperature;    //Pump's current water temperature
  bool temperatureIsCelsius;
  
  int errorCode;
};

struct TargetState {
  int pumpState;            //State we want the pump to be in
  int targetTemperature;    //Target temperature we want to set the pump to
};

#define TEMP_CURRENT 0 //received temperatures will be used as the current temperature
#define TEMP_TARGET 1 //received temperatures will be used as the target temperature
#define TEMP_PREP_CURRENT 2 //received temperatures will be ignored, preparing to set Current Temp
#define TEMP_PREP_TARGET 3 //received temperatures will be ignored, prepaing to set Target Temp

class HotTub : public SendReceive
{
  public:
    HotTub(int dataInPin, int dataOutPin, int debugPin);
    
    CurrentState* getCurrentState();
    TargetState* getTargetState();
    void setTargetState(int newState);
    int setTargetTemperature(int temp); //todo - setting in EEPROM

    int setLimitTemperature(int temp); //todo - setting in EEPROM
    int getLimitTemperature();

    int targetTemperatureValid(int targetTemp); //todo - make private and do all temperature validation in this class
    int maxTemperatureValid(int maxTemp); //todo - make private and do all temperature validation in this class

    void setTemperatureLock(bool enable); //todo - setting in EEPROM
    void setAutoRestart(bool enable); //todo - setting in EEPROM

    int getErrorCode();
    char* getStateJson();

    void onCommandSent(unsigned int command);
    void onCommandReceived(unsigned int command);
  
    void loop();
    void setup(void (&onStateChange)());
  private:
    void (*stateChanged)();
  
    CurrentState* currentState;
    TargetState* targetState;

    unsigned long tempIgnoreStart = 0; //start time to ignore temperature commands from

    int temperatureDestination = TEMP_PREP_CURRENT;
    bool manuallyTurnedOff; //indicates to the autoRestart code that the pump was turned off via the button / command, rather than the 24h timeout
    unsigned int lastButton; //to be used instead of manuallyTurnedOff to check if the pump was turned off with the control panel
    int limitTemperature = MAX_TEMP;//limits the temperature that can be set to via the control panel
    bool limitTemperatureIsCelsius;

    bool temperatureLockEnabled = false; //todo - setting in EEPROM
    bool autoRestartEnabled = false; //todo - setting in EEPROM
  
    void autoRestartCheck();
    void targetTemperatureCheck();
    void targetStateCheck();
           
    void decodeStatus(unsigned int command);
    void decodeError(unsigned int command);
    int decodeTemperature(unsigned int command);

    void handleReceivedError(unsigned int command);
    void handleReceivedStatus(unsigned int command);
    void handleReceivedTemperature(unsigned int command);
    void handleReceivedButton(unsigned int command);
    
    String stateToString(int pumpState);
    String errorToString(int errorCode);
    String buttonToString(int buttonCommand);

    void setReceivedTemperature(int temperature);

    unsigned long lastButtonPressTime;
};

#endif
