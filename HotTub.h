#ifndef HOTTUB_H
#define HOTTUB_H

#include <Arduino.h>
#include "Commands.h"
#include "Temperatures.h"

#define MAX_TEMP 40 //Maximum temperature supported by the hot tub
#define MIN_TEMP 20 //Minimum temperature supported by the hot tub

#define INIT_TEMP 37 //Temperature to initially set to when powering up.

//these are the widths of the pulses
#define START_PULSE 4440                //width of the start pulse
#define START_PULSE_TOLERANCE 100
#define START_PULSE_MIN START_PULSE - START_PULSE_TOLERANCE
#define START_PULSE_MAX START_PULSE + START_PULSE_TOLERANCE
#define DATA_START_DELAY 200            //amount of time after start pulse has gone low to wait before sampling the bits
#define BIT_PULSE 440                   //width of one bit
#define THIRD_BIT_PULSE 440 / 3
#define BUTTON_PULSE 147                //width of the low pulse when a button is pressed
#define BIT_COUNT 13                    //number of bits to read in
#define DATA_MASK 1 << (BIT_COUNT - 1)
#define MILLIS_WAIT_AFTER_COMMAND 20    //milliseconds to wait after a command has been received before sending a command
#define MILLIS_BETWEEN_COMMANDS 500    //millisconds to wait after sending a command before sending another command
#define MILLIS_BEFORE_INITIAL_TARGET_TEMP_CHECK 5000 //milliseconds after startup to wait before checking the initial target temperature
#define ALLOW_SEND_WITHOUT_RECEIVE false //enable this to allow sending commands without having first received any
#define IGNORE_INVALID_START_PULSE false //enable this to allow testing without a valid start pulse
#define TEMP_IGNORE_TIME 5000 //time to wait before sending a temperature button press to get the current target temperature
#define TEMP_ADJUST_DELAY 3000 //must be less than TEMP_IGNORE_TIME!!
#define BUTTON_SEND_DELAY 3000 //time to wait before sending buttons to change state

#define MAX_COMMANDS 10 //maximum number of commands to keep in the outgoing queue
//todo - extract Serial.print lines to a debug function that can be handled externally.

const int PUMP_UNKNOWN = -1; //haven't received a status command yet
const int PUMP_OFF = 0;
const int PUMP_FILTERING = 1;
const int PUMP_HEATER_STANDBY = 2;
const int PUMP_HEATING = 3;
const int PUMP_BUBBLES = 4;
const int PUMP_END = 5;
const int PUMP_ERROR = 6;

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

const int TEMP_CURRENT = 0; //received temperatures will be used as the current temperature
const int TEMP_TARGET = 1; //received temperatures will be used as the target temperature
const int TEMP_IGNORE = 2; //received temperatures will be ignored as we cant exactly say what the destination is just yet

class HotTub
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

    bool queueCommand(word command);
    void dataAvailable();
  
    void loop();
    void setup(void (&onStateChange)(int oldState, int newState), void (&onSetInterrupt)(bool enable, int direction), void (&logger)(String event));
  private:
    int dataInPin;
    int dataInPinBit;
    int dataOutPin;
    int dataOutPinBit;
    int ledPin;
    int ledPinBit;
    int debugPin;
    int debugPinBit;

    void (*stateChanged)(int oldState, int newState);
    void (*setInterrupt)(bool enable, int direction);
    void (*logger)(String event);
  
    CurrentState* currentState;
    TargetState* targetState;

    unsigned long tempIgnoreStart = 0; //start time to ignore temperature commands from

    int temperatureDestination = TEMP_CURRENT;
    bool manuallyTurnedOff; //indicates to the autoRestart code that the pump was turned off via the button / command, rather than the 24h timeout
    int limitTemperature = MAX_TEMP;//limits the temperature that can be set to via the control panel
    bool limitTemperatureIsCelsius;
  
    void autoRestartCheck();
    void targetTemperatureCheck();
    void targetStateCheck();
    word receiveCommand();
    void handleCommand(word command);
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

    word commandQueue[MAX_COMMANDS];
    int commandQueueCount;

    unsigned long pulseStartTime;
    bool timingStartPulse;

    void processCommandQueue();
    
    unsigned long lastSentCommand;
    unsigned long lastReceivedCommand;
    unsigned long lastButtonPress;
};

#endif
