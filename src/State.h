#ifndef STATE__H
#define STATE__H

struct CurrentState
{
    int pumpState;         //Pump's current state
    int temperature;       //Pump's current actual water temperature
    int targetTemperature; //Pump's current water temperature
    bool temperatureIsCelsius;

    bool flashing;                 //whether the display is blinking or not
    unsigned int autoRestartCount; //the number of times autorestart has started the pump;

    int errorCode;
};

struct TargetState
{
    int pumpState;         //State we want the pump to be in
    int targetTemperature; //Target temperature we want to set the pump to
};

#define PUMP_UNKNOWN -1 //haven't received a status command yet
#define PUMP_OFF 0
#define PUMP_FILTERING 1
#define PUMP_HEATER_STANDBY 2
#define PUMP_HEATING 3
#define PUMP_BUBBLES 4
#define PUMP_END 5
#define PUMP_ERROR 6
#define PUMP_FLASH 7

#define TM_NORMAL 0               //temperatures are current temperatures
#define TM_TEMP_BUTTON_DETECTED 1 //ignore temperatures until we know if it's flashing
#define TM_FLASH_DETECTED 2       //flash detected, wait for next flash to confirm
#define TM_FLASHING 3             //the display is currently flashing
#define TM_TEMP_MANUAL_CHANGE 4   //the target temperature was changed on the pump

#endif