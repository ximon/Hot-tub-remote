#include "SendReceive.h"
#include "Pins.h"
#include <i2s.h>
#include <i2s_reg.h>

#include <Syslog.h>

#define send(data)                       \
  while (i2s_write_sample_nb(data) == 0) \
    ;

SendReceive::SendReceive(int dataInPin, int dataEnablePin, int debugPin, Syslog *syslog)
    : logger(syslog)
{
  this->dataInPin = dataInPin;
  this->dataInPinBit = 1 << dataInPin;

  this->dataEnablePin = dataEnablePin;
  this->dataEnablePinBit = 1 << dataEnablePin;

  this->debugPin = debugPin;
  this->debugPinBit = 1 << debugPin;

  SEND_MODE = SM_WAIT;
}

void SendReceive::setup(void (&onSetTimer)(uint32_t ticks))
{
  setTimer = onSetTimer;

  if (getCommandQueueCount() > 0)
    printCommandQueue();
}

void SendReceive::loop()
{
  //dont send if we haven't received anything yet
  if (!ALLOW_SEND_WITHOUT_RECEIVE && lastCommandReceivedTime == 0)
  {
#ifdef DEBUG_SR
    logger->log("SR->Haven't received anthing yet...");
#endif
    return;
  }

  //dont send if we have sent one in the last 100msec
  if (millis() - WAIT_AFTER_SEND_COMMANDS < lastCommandSentTime)
  {
#ifdef DEBUG_SR
    logger->log("SR->Havent sent one for a while");
#endif
    return;
  }

  //dont send if we've received something in the last 20 mSec
  if (millis() - WAIT_AFTER_RECEIVE_COMMAND < lastCommandReceivedTime && lastCommandReceivedTime > 0)
  {
#ifdef DEBUG_SR
    logger->log("SR->Received one recently");
#endif
    return;
  }

  processIncomingCommand();
  processOutgoingCommandQueue();
}

int ICACHE_RAM_ATTR SendReceive::getReceiveBitTime(int bitIndex)
{
  //start off at half a bit so that each full bit lands us in the middle of a bit
  if (bitIndex == 0)
    return HALF_BIT_RELOAD;

  //bit 10 is the last bit of the start pulse, bit 11 is the first low after the start bit
  if (bitIndex == 10 || bitIndex == 11)
    return THREE_QUARTERS_BIT_RELOAD;

  return ONE_BIT_RELOAD;
}

void ICACHE_RAM_ATTR SendReceive::dataInterrupt()
{
  //cant disable the interrupt as the code in the lib doesn't have ICACHE_RAM_ATTR -> Exceptions Occur
  if (!dataInterruptEnabled)
    return;

  incomingData = 0;
  sampleCounter = 0;

  setTimer(getReceiveBitTime(sampleCounter));
  dataInterruptEnabled = false;
  digitalWrite(D6, LOW);
  digitalWrite(LED, LOW);
}

#define Debug_All_Interrupts
void ICACHE_RAM_ATTR SendReceive::timerInterrupt()
{
  bool dataState = digitalRead(DATA_IN);
  sampleCounter++;

  //if the interrupt is enabled we've missed the rising edge of the start bit!
  //it looks like this happens when dealing with wifi, so we have to start a timer
  //and periodically check to see if we're in the start bit
  if (dataInterruptEnabled)
  {
    startPulseLength = 0;
    pollingForStartPulseEnd = true;
    dataInterruptEnabled = false;
    digitalWrite(D6, LOW);
    digitalWrite(D3, true);
    delayMicroseconds(10);
    digitalWrite(D3, false);
  }

  if (pollingForStartPulseEnd)
  {
    digitalWrite(D3, true);
    delayMicroseconds(10);
    digitalWrite(D3, false);

    if (dataState)
    {
      startPulseLength++;
      setTimer(START_POLL_RELOAD);
      return;
    }
    else
    {
      //we've found the end of a start bit
      if (startPulseLength > START_PULSE_MIN_LENGTH && startPulseLength <= START_PULSE_MAX_LENGTH)
      {
        sampleCounter = SAMPLE_COUNT_END_OF_START_PULSE;
        commandRecovered = true;
      }
      else
      {
        //we've missed it, ignore this one and look for next pulse
        dataInterruptEnabled = true;
        digitalWrite(D6, HIGH);
        sampleCounter = 0;
      }

      pollingForStartPulseEnd = false;
    }
  }

#ifdef Debug_All_Interrupts
  digitalWrite(DBG, true);
#endif

  //the start bit is 10 bits long
  if (sampleCounter < SAMPLE_COUNT_LAST_START_BIT)
  {
    if (dataState)
    {
      //increment start bit length counter
      startBitCount++;
    }
    else
    {
      //reset and start looking for a start bit again
      badDataReason = 1;
      sampleCounter = 0;
      startBitCount = 0;
    }
  }

  //the last bit of the start bits
  if (sampleCounter == SAMPLE_COUNT_LAST_START_BIT)
  {
    //check if start bit count was 10
    if (!dataState)
    {
      //reset and start looking for a start bit again
      badDataReason = 2;
      sampleCounter = 0;
      startBitCount = 0;
    }
  }

  //the low before the first data bit
  if (sampleCounter == SAMPLE_COUNT_END_OF_START_PULSE)
  {
    if (dataState)
    {
      //reset and start looking for a start bit again
      badDataReason = 3;
      sampleCounter = 0;
      startBitCount = 0;
    }
  }

  //for all bits after the start bit
  if (sampleCounter > SAMPLE_COUNT_END_OF_START_PULSE && sampleCounter < SAMPLE_COUNT_TOTAL_BITS)
  {
#if defined Debug_Data_Only || defined Debug_All_Interrupts
    digitalWrite(DBG, true);
#endif
    incomingData <<= 1;
    incomingData += dataState ? 1 : 0;
  }

  if (sampleCounter < SAMPLE_COUNT_TOTAL_BITS && badDataReason == 0)
  {
    //reload the timer to test the next bit
    setTimer(getReceiveBitTime(sampleCounter));
  }
  else
  {
    lastCommandReceivedTime = millis();
    receivedCommand = incomingData;
    incomingData = 0;

    //re-enable data interrupt to detect next start bit
    digitalWrite(D6, HIGH);
    dataInterruptEnabled = true;
    digitalWrite(LED, true);

    //set the timer to trigger ~31ms from now
    //if !dataInterruptEnabled then we'll retrigger every 100us
    //to try and pick up the missed start of the start pulse
    setTimer(DETECT_MISSED_NEXT_START_PULSE);
  }

#if defined Debug_Data_Only || defined Debug_All_Interrupts
  digitalWrite(DBG, false);
#endif

  if (badDataReason > 0)
  {
    digitalWrite(D3, true);
    delayMicroseconds(10);
    digitalWrite(D3, false);
  }
}

void SendReceive::processIncomingCommand()
{
  if (receivedCommand == 0)
  {
    if (badDataReason > 0)
    {
      Serial.print("SR->Bad Data, Reason: ");
      Serial.println(badDataReason);
      badDataReason = 0;
    }

    return;
  }

  unsigned int command = receivedCommand;
  receivedCommand = 0;

  if (commandRecovered)
  {
    commandRecovered = false;
    logger->logf("SR->Recovered command 0x%X", receivedCommand);
    Serial.println("SR->Recovered a command!!");
  }

  lastCommandReceivedTime = millis();
  onCommandReceived(command);
}

unsigned long SendReceive::getLastCommandReceivedTime()
{
  return lastCommandReceivedTime;
}

/*##############################*/
/*##         Sending          ##*/
/*##############################*/

unsigned long SendReceive::getLastCommandSentTime()
{
  return lastCommandSentTime;
}

unsigned long SendReceive::getLastCommandQueuedTime()
{
  return lastCommandQueuedTime;
}

bool SendReceive::queueCommands(unsigned int command, int count)
{
  bool queued;

  for (int i = 0; i < count; i++)
  {
    queued = queueCommand(command);

    if (!queued)
      return false;
  }

  return true;
}

bool SendReceive::queueCommand(unsigned int command)
{
#ifdef DEBUG_SR
  logger->logf("SR->queueingCommand 0x%X", command);
#endif

  if (commandQueueCount + 1 > MAX_OUT_COMMANDS)
  {
#ifdef DEBUG_SR
    logger->log("Too many commands in the queue, current queue is:");
    printCommandQueue();
#endif
    return false;
  }

  commandQueue[commandQueueCount++] = command;
#ifdef DEBUG_SR
  logger->logf("Queued command 0x%X, count is now %i", commandQueueCount);
#endif

  lastCommandQueuedTime = millis();

  return true;
}

void SendReceive::printCommandQueue()
{
  for (int i = 0; i < commandQueueCount; i++)
  {
    logger->logf("   %i = 0x%X", i, commandQueue[i]);
  }
}

int SendReceive::getCommandQueueCount()
{
  return commandQueueCount;
}

void SendReceive::processOutgoingCommandQueue()
{
  if (commandQueueCount == 0)
    return;

#ifdef DEBUG_SR
  logger->log("SR->Processing outgoing command queue");
#endif

  unsigned int command = commandQueue[0];

  for (int i = 1; i < commandQueueCount; i++)
  {
    commandQueue[i - 1] = commandQueue[i];
  }
  commandQueueCount--;

#ifdef DEBUG_SR
  logger->log("SR->Outgoing queue count is %i", commandQueueCount);
#endif

  sendCommand(command);

  onCommandSent(command);
}

int SendReceive::getSendBitTime(int bitPos)
{
  switch (bitPos)
  {
  case 2:
    return 33;
  default:
    return 44;
  }
}

void SendReceive::sendCommand(unsigned int command)
{
#ifdef DEBUG_SR
  logger->logf("SR->Sending command 0x%X", command);
#endif

  dataInterruptEnabled = false;

  commandToSend = command;

  i2s_begin();
  i2s_set_rate(100000);

  SEND_MODE = SM_SENDING;
  GPOS = dataEnablePinBit;

  int i = 0;

  //Start Pulse
  for (i = 0; i <= 440; i++)
  {
    send(0xFFFFFFFF);
  }

  for (i = 0; i <= 18; i++)
  {
    send(0x00000000);
  }

  int bitTime;
  int bitCount = 0;
  bool bitValue = 0;
  while (bitCount < BIT_COUNT)
  {
    bitValue = (commandToSend & 0x1000) > 0;
    bitTime = getSendBitTime(bitCount);

    for (i = 0; i <= bitTime; i++)
    {
      send(bitValue ? 0xFFFFFFFF : 0x00000000);
      if (i % 1000 > 0)
        yield();
    }

    bitCount++;
    commandToSend <<= 1;
    commandToSend &= 0x3FFF;
  }

  delay(10);

  GPOC = dataEnablePinBit;

  lastCommandSentTime = millis();
  SEND_MODE = SM_WAIT;

  i2s_end();

  dataInterruptEnabled = true;
}
