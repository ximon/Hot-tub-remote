#ifndef HOTTUB_H
#define HOTTUB_H

#include <Arduino.h>
#include "Commands.h"
#include "Temperatures.h"

#define MAX_TEMP 40 //Maximum temperature supported by the hot tub
#define MIN_TEMP 9 //Minimum temperature supported by the hot tub

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
#define MILLIS_WAIT_AFTER_COMMAND 10    //milliseconds to wait after a command has been received before sending a command
#define MILLIS_BETWEEN_COMMANDS 500    //millisconds to wait after sending a command before sending another command

#define ALLOW_SEND_WITHOUT_RECEIVE false //enable this to allow sending commands without having first received any
#define IGNORE_INVALID_START_PULSE false //enable this to allow testing without a valid start pulse
#define DISABLE_TARGET_TEMPERATURE false

//todo - extract Serial.print lines to a debug function that can be handles externally.

struct HotTubState {
  bool pumpEnabled;
  bool blowerEnabled;
  bool heaterEnabled;
  bool heaterHeating;
  
  int temperature;
  bool temperatureIsCelsius;
  
  int errorCode;
};

class HotTub
{
  public:
    HotTub(int dataInPin, int dataOutPin, int debugPin);
    
    HotTubState* getCurrentState();
    HotTubState* getTargetState();

    void maximumTemperature(int temperature, bool isCelsius);
    int getMaximumTemperature();
  
    void temperatureLock(bool state);
    bool isTemperatureLockEnabled();
  
    void panelLock(bool state);
    bool isPanelLockEnabled();
  
    void autoRestart(bool state);
    bool isAutoRestartEnabled();

    int getErrorCode();

    void sendCommand(word command);

    void dataAvailable();
  
    void loop();
    void setup(void (&onStateChange)());
  private:
    private:
    int dataInPin;
    int dataInPinBit;
    int dataOutPin;
    int dataOutPinBit;
    int ledPin;
    int ledPinBit;
    int debugPin;
    int debugPinBit;

    void(*stateChanged)();
  
    HotTubState* currentState;
    HotTubState* targetState;
  
    void autoRestartCheck();
    void targetTemperatureCheck();
    word receiveCommand();
    void decodeCommand(word command);
    void decodeButton(word command);
    void decodeStatus(word command);
    int decodeError(word command);
    int decodeTemperature(word command);
    
    int limitTemperature; //limits the temperature that can be set to via the control panel
    bool limitTemperatureIsCelsius;
    bool temperatureLockEnabled;
    bool controlPanelLockEnabled;
    bool autoRestartEnabled;

    unsigned long lastSentCommand;
    unsigned long lastReceivedCommand;
};

#endif
