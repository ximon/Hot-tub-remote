#include "Commands.h"
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

  timer1_attachInterrupt(onTimerISR);

  attachInterrupt(digitalPinToInterrupt(BUTTON), onButtonInterrupt, FALLING);
  attachInterrupt(digitalPinToInterrupt(DATA_IN), onDataInterrupt, CHANGE);

  sendCommand(CMD_BTN_HEAT);
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

void onTimerISR()
{
  os_intr_lock();
  GPOS = DBG_BIT;

  switch (RECV_MODE)
  {
  case RM_WAIT_PULSE_END:
    timer1_write(DATA_CLOCK_TIME_BTN);
    RECV_MODE = RM_WAIT_DATA_END;
    bitPos = DATA_MASK;
    data = 0;
    //disable pin interrupts
    break;
  case RM_WAIT_DATA_END:
    timer1_write(DATA_CLOCK_TIME);

    if (digitalRead(DATA_IN))
      data |= bitPos;

    bitPos >>= 1;

    if (bitPos == 0)
    {
      timer1_disable();
      RECV_MODE = RM_WAIT_PULSE;
      commandReceived = true;
    }
    break;
  }

  switch (SEND_MODE)
  {
  case SM_START_END: //end the start pulse, set timer to 150us (start gap time)
    GPOC = DATA_OUT_BIT;

    timer1_write(OUT_START_GAP_TIME);
    SEND_MODE = SM_GAP_END;
    break;
  case SM_GAP_END: //end of the start gap, set timer to 150us (button pulse)
    timer1_write(OUT_BUTTON_TIME);
    SEND_MODE = SM_BUTTON_END;

    if (outCommand & outBitPos)
      GPOS = DATA_OUT_BIT;
    else
      GPOC = DATA_OUT_BIT;

    outBitPos >>= 1;
    break;
  case SM_BUTTON_END:
    timer1_write(DATA_CLOCK_TIME);

    if (outCommand & outBitPos)
      GPOS = DATA_OUT_BIT;
    else
      GPOC = DATA_OUT_BIT;

    outBitPos >>= 1;

    if (outBitPos == 0)
      SEND_MODE = SM_DATA_END;
    break;
  case SM_DATA_END:
    GPOC = DATA_OUT_BIT;
    SEND_MODE = SM_WAIT;
  }

  GPOC = DBG_BIT;
  os_intr_unlock();
}

void sendCommand(word command)
{
  Serial.print("Sending command 0x");
  Serial.println(command, HEX);

  outCommand = command;

  os_intr_lock();
  GPOS = DATA_OUT_BIT; //start the start pulse
  os_intr_unlock();

  SEND_MODE = SM_START_END;           //next step is to end the start pulse
  timer1_write(OUT_START_PULSE_TIME); //set to trigger @ 4440us
  timer1_enable(TIM_DIV256, TIM_EDGE, TIM_SINGLE);
  outBitPos = DATA_MASK;
}

void loop()
{
  if (commandReceived)
  {
    commandReceived = false;
    Serial.print("Received 0x");
    Serial.println(data, HEX);
  }

  if (invalidStartPulse)
  {
    invalidStartPulse = false;
    Serial.print("Invalid start pulse length detected @");
    Serial.print(startPulseLength);
    Serial.println("us");
  }

  if (commandToSend > 0)
  {
    sendCommand(commandToSend);
  }
}

ICACHE_RAM_ATTR void onDataInterrupt()
{
  os_intr_lock();
  GPOS = DBG_BIT;

  if (SEND_MODE != SM_WAIT)
    return;

  switch (RECV_MODE)
  {
  case RM_WAIT_PULSE:
    if (digitalRead(DATA_IN))
    {
      invalidStartPulse = false;
      pulseStartTime = micros();
      RECV_MODE = RM_WAIT_PULSE_END;
      timer1_enable(TIM_DIV256, TIM_EDGE, TIM_SINGLE);
      timer1_write(IN_START_PULSE_TIME);
    }
    break;
  case RM_WAIT_PULSE_END: //we have received a state change before or after we expected one!?
    startPulseLength = micros() - pulseStartTime;
    if (startPulseLength < START_PULSE_LEN - 100 || startPulseLength > START_PULSE_LEN + 100)
    {
      invalidStartPulse = true;
    }
    break;
  }

  GPOC = DBG_BIT;
  os_intr_unlock();
}

ICACHE_RAM_ATTR void onButtonInterrupt()
{
  commandToSend = CMD_BTN_TEMP_UP;
}
