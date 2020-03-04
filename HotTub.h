#ifndef HOTTUB_H
#define HOTTUB_H

#include <Arduino.h>
#include "Commands.h"
#include "Temperatures.h"
#include "SendReceive.h"

#define MAX_TEMP 40 //Maximum temperature supported by the hot tub
#define MIN_TEMP 20 //Minimum temperature supported by the hot tub

#define INIT_TEMP 37 //Temperature to initially set to when powering up.

#define WAIT_BETWEEN_SENDING_AUTO_COMMANDS 500
#define WAIT_BEFORE_INITIAL_TARGET_TEMP_CHECK 5000 //milliseconds after startup to wait before checking the initial target temperature


#define TEMP_IGNORE_TIME 5000 //time to wait before sending a temperature button press to get the current target temperature
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
#define TEMP_IGNORE 2 //received temperatures will be ignored as we cant exactly say what the destination is just yet


class HotTub : public SendReceive
{
  public:
    HotTub(int dataInPin, int dataOutPin, int debugPin);
    
    CurrentState* getCurrentState();
    TargetState* getTargetState();
    void setTargetState(int newState);
    void setTargetTemperature(int temp); //todo - setting in EEPROM

    void setLimitTemperature(int temp); //todo - setting in EEPROM
    int getLimitTemperature();
    
    bool temperatureLockEnabled; //todo - setting in EEPROM
    bool autoRestartEnabled = true; //todo - setting in EEPROM

    int getErrorCode();

    virtual void onCommandSent(word command);
    virtual void onCommandReceived(word command);
  
    void loop();
    void setup(void (&onStateChange)(int oldState, int newState));
  private:
    void (*stateChanged)(int oldState, int newState);
  
    CurrentState* currentState;
    TargetState* targetState;

    unsigned long tempIgnoreStart = 0; //start time to ignore temperature commands from

    int temperatureDestination = TEMP_CURRENT;
    bool manuallyTurnedOff; //indicates to the autoRestart code that the pump was turned off via the button / command, rather than the 24h timeout
    word lastButton; //to be used instead of manuallyTurnedOff to check if the pump was turned off with the control panel
    int limitTemperature = MAX_TEMP;//limits the temperature that can be set to via the control panel
    bool limitTemperatureIsCelsius;
  
    void autoRestartCheck();
    void targetTemperatureCheck();
    void targetStateCheck();
           
    void decodeStatus(word command);
    void decodeError(word command);
    int decodeTemperature(word command);

    void handleReceivedError(word command);
    void handleReceivedStatus(word command);
    void handleReceivedTemperature(word command);
    void handleReceivedButton(word command);
    
    String stateToString(int pumpState);
    String errorToString(int errorCode);
    String buttonToString(int buttonCommand);

    void setReceivedTemperature(int temperature);

    unsigned long lastButtonPressTime;
};

#endif
