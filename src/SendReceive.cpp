#include "SendReceive.h"
#include <i2s.h>
#include <i2s_reg.h>

#define send(data)                       \
  while (i2s_write_sample_nb(data) == 0) \
    ;

SendReceive::SendReceive(int dataInPin, int dataEnablePin, int debugPin)
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
  Serial.print("SR->Data: 0x");
  Serial.println(receivedCommand, HEX);

  if (!includeBreakdown)
    return;

  int i;
  for (i = 0; i < dataIndex; i++)
  {
    Serial.print(i);
    Serial.print("\t");
  }
  Serial.println();

  for (i = 0; i < dataIndex; i++)
  {
    Serial.print(states[i]);
    Serial.print("\t");
  }
  Serial.println();

  for (i = 0; i < dataIndex; i++)
  {
    Serial.print(times[i]);
    Serial.print("\t");
  }
  Serial.println();
}

void SendReceive::setup(
    void (&onSetDataInterrupt)(bool state),
    void (&onDebug)(char *message),
    void (&onDebugf)(char *fmt, ...))
{
  setDataInterrupt = onSetDataInterrupt;
  debug = onDebug;
  debugf = onDebugf;
}

int eraseCount;
void SendReceive::loop()
{
  if (dataStart > 0 && micros() - dataStart >= MAX_MESSAGE_LEN && dataIndex > 0)
  {
    //GPOC = debugPinBit;
    //Serial.println();
    //Serial.println();
    //Serial.println("Decoding Data...");
    if (dataIndex < 15)
    {
      times[dataIndex] = micros() - lastBitStartTime;

      if (times[dataIndex] >= BIT_TIME && states[dataIndex - 1] == true)
        states[dataIndex] = lastBitState;
    }
    receivedCommand = decode(times, states);
    //printMessageData(false);

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
    Serial.println("SR->Haven't received anthing yet...");
#endif
    return;
  }

  //dont send if we have sent one in the last 100msec
  if (millis() - WAIT_AFTER_SEND_COMMANDS < lastCommandSentTime)
  {
#ifdef DEBUG_SR
    Serial.println("SR->Havent sent one for a while");
#endif
    return;
  }

  //dont send if we've received something in the last 20 mSec
  if (millis() - WAIT_AFTER_RECEIVE_COMMAND < lastCommandReceivedTime && lastCommandReceivedTime > 0)
  {
#ifdef DEBUG_SR
    Serial.println("SR->Received one recently");
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

unsigned int SendReceive::decode(unsigned int times[], bool states[])
{
  unsigned int data = 0;
  int decodeBit = 1;
  int bitTime = getReceiveBitTime(decodeBit);
  int remaining = 0;
  int totalTime = times[0];

  for (int i = 1; i < BIT_COUNT; i++)
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

  //Serial.print("Total time was ");
  //Serial.println(totalTime);

  return data;
}

void ICACHE_RAM_ATTR SendReceive::dataInterrupt()
{
  if (SEND_MODE == SM_SENDING)
    return;

  now = micros();
  bitState = GPIP(dataInPin);

  if (dataIndex > BIT_COUNT)
  {
    dataIndex = 0;
    return;
  }

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
  Serial.print("SR->queueingCommand 0x");
  Serial.println(command, HEX);
#endif

  if (commandQueueCount + 1 > MAX_OUT_COMMANDS)
  {
#ifdef DEBUG_SR
    Serial.println("Too many commands in the queue, current queue is:");
    printCommandQueue();
#endif
    return false;
  }

  commandQueue[commandQueueCount++] = command;
#ifdef DEBUG_SR
  Serial.print("Queued command 0x");
  Serial.print(command, HEX);
  Serial.print(", count is now ");
  Serial.println(commandQueueCount);
#endif

  lastCommandQueuedTime = millis();

  return true;
}

void SendReceive::printCommandQueue()
{
  for (int i = 0; i < commandQueueCount; i++)
  {
    Serial.print("   ");
    Serial.print(i);
    Serial.print(" = 0x");
    Serial.println(commandQueue[i], HEX);
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

  //Serial.print("Received command 0x");
  //Serial.println(command, HEX);

  lastCommandReceivedTime = millis();
  onCommandReceived(command);
}

void SendReceive::processOutgoingCommandQueue()
{
  if (commandQueueCount == 0)
    return;

#ifdef DEBUG_SR
  Serial.println("SR->Processing outgoing command queue");
#endif

  unsigned int command = commandQueue[0];

  for (int i = 1; i < commandQueueCount; i++)
  {
    commandQueue[i - 1] = commandQueue[i];
  }
  commandQueueCount--;

#ifdef DEBUG_SR
  Serial.print("SR->Outgoing queue count is ");
  Serial.println(commandQueueCount);
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
  setDataInterrupt(false);

#ifdef DEBUG_SR
  Serial.print("SR->Sending command 0x");
  Serial.println(command, HEX);
#endif

  commandToSend = command;

  i2s_begin();
  i2s_set_rate(100000);

  SEND_MODE = SM_SENDING;
  GPOS = dataEnablePinBit;

  int i = 0;

  //Start Pulse
  for (i = 0; i <= START_PULSE_COUNT; i++)
  {
    send(0xFFFFFFFF);
  }

  for (i = 0; i <= START_SPACE_COUNT; i++)
  {
    send(0x00000000);
  }

#ifdef DEBUG_SR
  Serial.print("Sending ");
#endif

  int bitTime;
  int bitCount = 0;
  bool bitValue = 0;
  while (bitCount < BIT_COUNT)
  {
    bitValue = (commandToSend & 0x1000) > 0;
    bitTime = getSendBitTime(bitCount);
#ifdef DEBUG_SR
    Serial.print(bitValue ? "1" : "0");
#endif
    for (i = 0; i <= bitTime; i++)
    {
      send(bitValue ? 0xFFFFFFFF : 0x00000000);
    }

    bitCount++;
    commandToSend <<= 1;
    commandToSend &= 0x3FFF;
  }
#ifdef DEBUG_SR
  Serial.println();
#endif

  delay(10);

  GPOC = dataEnablePinBit;

  lastCommandSentTime = millis();
  SEND_MODE = SM_WAIT;

  i2s_end();

  setDataInterrupt(true);
}
