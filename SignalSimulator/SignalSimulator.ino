#include "SendReceive.h"
#include "Commands.h"
#include "Temperatures.h"
#include "Pins.h"

bool ledState;
bool sendCommandNow = true;

word commands[] = {CMD_ERROR_PKT1, CMD_ERROR_1};
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
    Serial.print("Sending command ");
    Serial.println(commands[commandIndex]);
    sendCommand(commands[commandIndex]);
    commandIndex = commandIndex == 1 ? 0 : 1;
    
    ledState = !ledState;
    digitalWrite(LED, ledState);
    sendCommandNow = false;
    //timer1_write(10937); //35ms, 1 tick = 3.2us
    timer1_write(312500);
  }
}
