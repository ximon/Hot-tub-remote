#include "SendReceive.h"
#include <i2s.h>
#include <i2s_reg.h>

#define send(data) while(i2s_write_sample_nb(data)==0);

SendReceive::SendReceive(int dataInPin, int dataEnablePin, int debugPin) {
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

void SendReceive::printMessageData() {
  int i;

  Serial.print("Data: 0x");
  Serial.println(receivedCommand, HEX);
  
  for(i = 0; i < dataIndex; i++) {
    Serial.print(i);
    Serial.print("\t");      
  }
  Serial.println();

  for (i = 0; i < dataIndex; i++) {
    Serial.print(states[i]);
    Serial.print("\t");
  }
  Serial.println();

  for (i = 0; i < dataIndex; i++) {
    Serial.print(times[i]);
    Serial.print("\t");
  }
  Serial.println();
}

int eraseCount;
void SendReceive::loop() {  
  if (dataStart > 0 && micros() - dataStart >= MAX_MESSAGE_LEN && dataIndex > 0)
  {
      //GPOC = debugPinBit;
      //Serial.println();
      //Serial.println();
      //Serial.println("Decoding Data...");
      if (dataIndex < 15)
      {
          times[dataIndex] = micros() - lastBitStartTime;

          if (times[dataIndex] >= BIT_TIME && states[dataIndex-1] == true)
              states[dataIndex] = lastBitState;
      }
      receivedCommand = decode(times, states);
      //printMessageData();
      
      dataStart = 0;
      dataIndex = 0;
      for (eraseCount = 0; eraseCount <= BIT_COUNT; eraseCount++)
      {
          times[eraseCount] = 0;
          states[eraseCount] = false;
      }
  }
  
  if (sentCommand > 0) {
    onCommandSent(sentCommand);
    sentCommand = 0;
  }
  
  processIncomingCommand();

  //dont send if we haven't received anything yet
  if (!ALLOW_SEND_WITHOUT_RECEIVE && lastCommandReceivedTime == 0) {
    //Serial.println("Haven't received anthing yet...");
    return;
  }

  //dont send if we have sent one in the last 100msec
  if (millis() - WAIT_AFTER_SEND_COMMANDS < lastCommandSentTime) {
    //Serial.println("Havent sent one for a while");    return;
  }

  //dont send if we've received something in the last 20 mSec
  if (millis() - WAIT_AFTER_RECEIVE_COMMAND < lastCommandReceivedTime && lastCommandReceivedTime > 0) {
    //Serial.println("received one recently");
    return;
  }

  processOutgoingCommandQueue();
}

int SendReceive::getReceiveBitTime(int bitIndex)
{
  switch (bitIndex)
  {
      case 0: return DATA_START_LEN;
      case 1: return BUTTON_TIME;
      case 4: return SHORT_BIT_TIME;
      default: return BIT_TIME;
  }
}

word SendReceive::decode(word times[], bool states[])
{
    word data = 0;
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

    Serial.print("Total time was ");
    Serial.println(totalTime);

    return data;
}

void ICACHE_RAM_ATTR SendReceive::dataInterrupt() {
  now = micros();
  bitState = digitalRead(dataInPin);
  
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
    validStartBit = dataIndex == 0 
                      && bitTime >= DATA_START_LEN_MIN
                      && bitTime <= DATA_START_LEN_MAX
                      && bitState == 0;

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

unsigned long SendReceive::getLastCommandSentTime() {
    return lastCommandSentTime;
}

unsigned long SendReceive::getLastCommandReceivedTime() {
  return lastCommandReceivedTime;
}

unsigned long SendReceive::getLastCommandQueuedTime() {
  return lastCommandQueuedTime;
}

bool SendReceive::queueCommand(word command) {
  if (commandQueueCount + 1 > MAX_OUT_COMMANDS) {
    Serial.println("Too many commands in the queue, current queue is:");

    for (int i = 0; i < commandQueueCount; i++) {
      Serial.print("   ");
      Serial.print(i);
      Serial.print(" = 0x");
      Serial.println(commandQueue[i], HEX);
    }
    
    return false;
  }
  
  commandQueue[commandQueueCount++] = command;
  Serial.print("Queued command, count is now ");
  Serial.println(commandQueueCount);

  lastCommandQueuedTime = millis();
  
  return true;
}

int SendReceive::getCommandQueueCount() {
  return commandQueueCount;
}

void SendReceive::processIncomingCommand() {
  if (receivedCommand == 0)
    return;  

  word command = receivedCommand;
  receivedCommand = 0;
  
  //Serial.print("Received command 0x");
  //Serial.println(command, HEX);

  lastCommandReceivedTime = millis();
  onCommandReceived(command);
}

void SendReceive::processOutgoingCommandQueue() {
  if (commandQueueCount == 0)
    return;

  Serial.println("Processing outgoing command queue");

  word command = commandQueue[0];

  for (int i = 1; i < commandQueueCount; i++) {
    commandQueue[i-1] = commandQueue[i];
  }
  commandQueueCount--;

  Serial.print("Outgoing queue count is ");
  Serial.println(commandQueueCount);

  sendCommand(command);
}

int SendReceive::getSendBitTime(int bitPos){
    switch (bitPos) {
      case 2: return 33;
      default: return 44;
    }
}

void SendReceive::sendCommand(word command) {
  Serial.print("Sending command 0x");
  Serial.println(command,HEX);

  commandToSend = command;
  
  i2s_begin();
  i2s_set_rate(100000);

  SEND_MODE = SM_SENDING;
  GPOS = dataEnablePinBit;

  int i = 0;

  //Start Pulse
  for (i = 0; i <= 440; i++) {
    send(0xFFFFFFFF);  
  }

  for (i = 0; i<= 18; i++)
  {
    send(0x00000000);
  }

  Serial.print("Sending ");
  int bitTime;
  int bitCount = 0;
  bool bitValue = 0;
  while (bitCount < BIT_COUNT)
  {
    bitValue = (commandToSend & 0x1000) > 0;
    bitTime = getSendBitTime(bitCount);
    Serial.print(bitValue ? "1" : "0");
    for (i = 0; i<= bitTime; i++)
    {      
      send(bitValue ? 0xFFFFFFFF : 0x00000000);
      if (i % 1000 > 0)
        yield();
    }

    bitCount++;
    commandToSend <<= 1;
    commandToSend &= 0x3FFF;
  }
  Serial.println();

  delay(10);
  
  GPOC = dataEnablePinBit;

  lastCommandSentTime = millis(); 
  SEND_MODE = SM_WAIT;

  i2s_end();
}
