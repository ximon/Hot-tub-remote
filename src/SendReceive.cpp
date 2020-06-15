#include "SendReceive.h"
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
  RECV_MODE = RM_WAIT_PULSE;

  dataIndex = 0;
  dataStart = 0;
  lastBitStartTime = 0;
}

void SendReceive::printMessageData(bool includeBreakdown)
{
  logger->logf("SR->Data: 0x%X", receivedCommand);

  if (!includeBreakdown)
    return;

  int i;

  char msg[100];

  for (i = 0; i < dataIndex; i++)
  {
    sprintf(msg, "%s%i\t", msg, i);
    //Serial.print(i);
    //Serial.print("\t");
  }
  logger->log(msg);
  Serial.println();

  msg[0] = '\0';

  for (i = 0; i < dataIndex; i++)
  {
    sprintf(msg, "%s%i\t", msg, states[i]);
    //Serial.print(states[i]);
    //Serial.print("\t");
  }
  logger->log(msg);
  //Serial.println();

  msg[0] = '\0';

  for (i = 0; i < dataIndex; i++)
  {
    sprintf(msg, "%s%i\t", msg, times[i]);
    //Serial.print(times[i]);
    //Serial.print("\t");
  }
  logger->log(msg);
  //Serial.println();
}

int eraseCount;
void SendReceive::loop()
{
  if (dataStart > 0 && micros() - dataStart >= MAX_MESSAGE_LEN && dataIndex > 0)
  {
    //GPOC = debugPinBit;
    //logger->log("Decoding Data...");
    if (dataIndex < 15)
    {
      times[dataIndex] = micros() - lastBitStartTime;

      if (times[dataIndex] >= BIT_TIME && states[dataIndex - 1] == true)
        states[dataIndex] = lastBitState;
    }
    receivedCommand = decode(times, states);

    if (receivedCommand == 0x16D2 || receivedCommand == 0x518 || receivedCommand == 0x5E8)
      printMessageData(false);

    dataStart = 0;
    dataIndex = 0;
    for (eraseCount = 0; eraseCount <= BIT_COUNT; eraseCount++)
    {
      times[eraseCount] = 0;
      states[eraseCount] = false;
    }
  }

  processIncomingCommand();

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

  processOutgoingCommandQueue();
}

int SendReceive::getReceiveBitTime(int bitIndex)
{
  switch (bitIndex)
  {
  case 0:
    return DATA_START_LEN;
  case 1:
    return BUTTON_TIME;
  case 4:
    return SHORT_BIT_TIME;
  default:
    return BIT_TIME;
  }
}

unsigned int SendReceive::decode(volatile unsigned int times[], volatile bool states[])
{
  unsigned int data = 0;
  int decodeBit = 1;
  int bitTime = getReceiveBitTime(decodeBit);
  int remaining = 0;
  int totalTime = times[0];

  for (int i = 1; i < 13; i++)
  {
    remaining = times[i];

    if (totalTime > MAX_MESSAGE_LEN || times[i] == 0)
      return data;

    while (remaining >= bitTime - BIT_TOLERANCE)
    {
      data <<= 1;
      data += states[i] ? 1 : 0;
      remaining -= bitTime - BIT_TOLERANCE;
      totalTime += bitTime - BIT_TOLERANCE;
      decodeBit++;

      bitTime = getReceiveBitTime(decodeBit);

      if (totalTime > MAX_MESSAGE_LEN)
        return data;
    }
  }

  //logger->logf("Total time was %i", totalTime);

  return data;
}

void ICACHE_RAM_ATTR SendReceive::dataInterrupt()
{
  unsigned now = micros();
  bool bitState = GPIP(dataInPin);

  if (SEND_MODE != SM_WAIT)
    return;

  if (dataStart == 0 && bitState == 1)
  {
    states[dataIndex] = 1;
    lastBitState = bitState;
    dataStart = now - BIT_TIME_ADJUST;
    dataIndex = 0;
  }
  else
  {
    bitTime = dataIndex == 0
                  ? (now - dataStart)
                  : (now - lastBitStartTime);

    nonStartBit = dataIndex > 0;
    validStartBit = dataIndex == 0 && bitTime >= DATA_START_LEN_MIN && bitTime <= DATA_START_LEN_MAX && bitState == 0;

    //only start incrementing once a valid start pulse has been found
    if (validStartBit || nonStartBit)
    {
      times[dataIndex] = bitTime;
      dataIndex++;
      lastBitState = !lastBitState;
      states[dataIndex] = lastBitState;
    }
    else
    {
      dataStart = now - BIT_TIME_ADJUST;
    }
  }

  lastBitStartTime = now - BIT_TIME_ADJUST;
}

unsigned long SendReceive::getLastCommandSentTime()
{
  return lastCommandSentTime;
}

unsigned long SendReceive::getLastCommandReceivedTime()
{
  return lastCommandReceivedTime;
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

void SendReceive::processIncomingCommand()
{
  if (receivedCommand == 0)
    return;

  unsigned int command = receivedCommand;
  receivedCommand = 0;

  //logger->logf("Received command 0x%X", command);

  lastCommandReceivedTime = millis();
  onCommandReceived(command);
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
}
