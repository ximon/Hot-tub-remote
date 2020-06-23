#ifndef SENDRECEIVE_H
#define SENDRECEIVE_H

#include <Arduino.h>

#include <Syslog.h>

#define ALLOW_SEND_WITHOUT_RECEIVE false //enable this to allow sending commands without having first received any
#define IGNORE_INVALID_START_PULSE false //enable this to allow testing without a valid start pulse

#define WAIT_AFTER_RECEIVE_COMMAND 10 //milliseconds to wait after a command has been received before sending a command, the window is ~40ms
#define WAIT_AFTER_SEND_COMMANDS 100  //millisconds to wait after sending a command before sending another command

//Timings in us

//Timings in ticks
#define BIT_COUNT 13 //number of bits to read in
#define DATA_MASK 1 << (BIT_COUNT - 1)

#define MAX_OUT_COMMANDS 20 //maximum number of commands to keep in the outgoing queue

/*
 * SM_WAIT, data in out buffer -> SM_START_END, timer elapses -> SM_GAP_END, timer elapses -> SM_BUTTON_END, 13 bits clocked out -> SM_WAIT
 */
#define SM_WAIT 0
#define SM_SENDING 1

class SendReceive
{
public:
  SendReceive(int dataInPin, int dataOutPin, int debugPin, Syslog *syslog);
  void setup(void (&onSetTimer)(uint32_t ticks));
  void loop();

  virtual void onCommandReceived(unsigned int command) = 0;
  virtual void onCommandSent(unsigned int command) = 0;

  void ICACHE_RAM_ATTR dataInterrupt();
  void ICACHE_RAM_ATTR timerInterrupt();

  unsigned long getLastCommandSentTime();
  unsigned long getLastCommandQueuedTime();
  unsigned long getLastCommandReceivedTime();

  bool queueCommand(unsigned int command);
  bool queueCommands(unsigned int command, int count);
  int getCommandQueueCount();
  void printMessageData(bool includeBreakdown);

private:
  int dataInPin;
  int dataInPinBit;
  int dataEnablePin;
  int dataEnablePinBit;
  int debugPin;
  int debugPinBit;

  void (*setTimer)(uint32_t ticks);
  Syslog *logger;

  //receiving
  volatile bool dataInterruptEnabled = true;
  volatile int sampleCounter = 0;
  volatile int startBitCount = 0;
  volatile bool startBitOk = false;
  volatile unsigned int incomingData = 0;
  int ICACHE_RAM_ATTR getReceiveBitTime(int bitIndex);
  volatile unsigned int commandReceivedAt = 0;
  void processIncomingCommand();
  int startPulseLength = 0;
  volatile unsigned int receivedCommand;
  bool pollingForStartPulseEnd = false;
  volatile bool commandRecovered = false;

#define ONE_BIT_RELOAD 2185 // 440us
#define HALF_BIT_RELOAD ONE_BIT_RELOAD / 2
#define THREE_QUARTERS_BIT_RELOAD (ONE_BIT_RELOAD / 4) * 3
#define START_POLL_RELOAD 431 //100us

#define DETECT_MISSED_NEXT_START_PULSE 155000 //~31 ms

#define START_PULSE_MIN_LENGTH 4
#define START_PULSE_MAX_LENGTH 44

//bit positions
#define SAMPLE_COUNT_LAST_START_BIT 10
#define SAMPLE_COUNT_END_OF_START_PULSE 11
#define SAMPLE_COUNT_TOTAL_BITS 25

#define BAD_DATA_START_BIT_TOO_SHORT 1
#define BAD_DATA_START_BIT_LAST_BIT_NOT_HIGH 2
#define BAD_DATA_BIT_AFTER_START_BIT_NOT_LOW 3
  volatile int badDataReason = 0;

  volatile int RECV_MODE;

  //sending
  volatile int bitIndex;
  int getSendBitTime(int bitPos);

  unsigned int commandQueue[MAX_OUT_COMMANDS];
  volatile int commandQueueCount;

  unsigned int commandToSend;
  int SEND_MODE;

  unsigned long lastCommandSentTime;
  unsigned long lastCommandQueuedTime;
  unsigned long lastCommandReceivedTime;

  void processOutgoingCommandQueue();
  void sendCommand(unsigned int command);

  void printCommandQueue();
};

#endif
