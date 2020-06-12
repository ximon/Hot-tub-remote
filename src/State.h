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

#endif