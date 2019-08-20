#include "SendReceive.h"
#include "Commands.h"
#include "Temperatures.h"
#include "Pins.h"

bool ledState;
bool sendCommandNow = true;

const int CMD_COUNT = 3;
word commands[CMD_COUNT] = {CMD_STATE_PUMP_ON_HEATER_HEATING, TEMP_37C, CMD_BTN_HEAT};
int commandIndex = 0;

void setup() {
  pinMode(DATA_OUT, OUTPUT);
  Serial.begin(115200);
  Serial.println("Starting timer");

  timer1_attachInterrupt(onTimerISR);
  timer1_enable(TIM_DIV256, TIM_EDGE, TIM_SINGLE);
}


void onTimerISR() {
  sendCommandNow = true;

}


void loop() {
  if (sendCommandNow) {
    sendCommand(commands[commandIndex]);
    commandIndex++;
    if (commandIndex >= CMD_COUNT)
      commandIndex = 0;
    
    ledState = !ledState;
    digitalWrite(LED, ledState);
    sendCommandNow = false;
    timer1_write(10937); //35ms, 1 tick = 3.2us
    //timer1_write(312500);
  }
}
