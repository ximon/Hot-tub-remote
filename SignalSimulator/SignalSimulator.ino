#include "Commands.h"
#include "SendReceive.h"
#include "Temperatures.h"
#define LED D4
#define LED_BIT 1 << LED
#define DATA_IN D1
#define DATA_OUT D2
#define DATA_OUT_BIT 1 << DATA_OUT
#define DBG D8
#define DBG_BIT 1 << DBG
#define BUTTON D5

#define DATA_CLOCK_TIME_BTN 106
#define DATA_CLOCK_TIME 138  //440us 1 bit time
#define START_PULSE_LEN 4440 //length of the start pulse in us
#define BIT_COUNT 13         //number of bits to read in
#define DATA_MASK 1 << (BIT_COUNT - 1)

#define IN_START_PULSE_TIME 1410 //4540us, 100us after start pulse

#define OUT_START_PULSE_TIME 1380 //4440us, start pulse
#define OUT_START_GAP_TIME 44     //150us
#define OUT_BUTTON_TIME 137       // 440us

void setup()
{
  pinMode(DATA_OUT, OUTPUT);
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(DBG, OUTPUT);
  Serial.begin(115200);
  Serial.println("");
  Serial.println("Starting timer");

  //attachInterrupt(digitalPinToInterrupt(BUTTON), onButtonInterrupt, FALLING);
  //attachInterrupt(digitalPinToInterrupt(DATA_IN), onDataInterrupt, CHANGE);
}

volatile int bitCount;
volatile bool commandReceived;
word data;
word bitPos;

unsigned long pulseStartTime;
unsigned long startPulseLength;
bool invalidStartPulse;

/*
 * RM_WAIT_PULSE, start pulse received -> RM_WAIT_PULSE_END, timer elapses -> RM_WAIT_DATA_END, 13 bits clocked in -> RM_WAIT_PULSE
 */
const int RM_WAIT_PULSE = 0;     //waiting for the start pulse to go high
const int RM_WAIT_PULSE_END = 1; //waiting for the start pulse to go low
const int RM_WAIT_DATA_END = 2;  //waiting for the data to end

volatile int RECV_MODE = RM_WAIT_PULSE;

/*
 * SM_WAIT, data in out buffer -> SM_START_END, timer elapses -> SM_GAP_END, timer elapses -> SM_BUTTON_END, 13 bits clocked out -> SM_WAIT
 */
const int SM_WAIT = 0;
const int SM_START_END = 1;
const int SM_GAP_END = 2;
const int SM_BUTTON_END = 3;
const int SM_DATA_END = 4;

volatile int SEND_MODE = SM_WAIT;

volatile word commandToSend;
volatile int outBitPos;
volatile word outCommand;

unsigned long lastSend;
int lastMessage = 0;
void loop()
{
  delay(40);
  //if (millis() - 1000 > lastSend);
  //{
    GPOS = DBG_BIT;
    //lastSend = millis();
    //sendCommand(CMD_STATE_PUMP_ON_HEATER_HEATING);
    
    if (lastMessage == 0)
    {
      sendCommand(CMD_BTN_PUMP); //0x2C7
      lastMessage = 1;
    }
    else
    {
      sendCommand(TEMP_63F); //0xB69
      lastMessage = 0;
    }

    GPOC = DBG_BIT;
  //}
}
