#include "HotTubMqtt.h"
#include "PubSubClient.h"

#define DEBUG_MQTT

HotTubMqtt::HotTubMqtt(HotTub *hotTubInstance)
    : hotTub(hotTubInstance)
{
}

void HotTubMqtt::setup(void (&onMqttEvent)(char *topic, byte *payload, unsigned int length))
{
  client.setClient(espClient);
  client.setCallback(onMqttEvent);
}

void HotTubMqtt::setServer(char *ipAddress, int port)
{
  mqttServer = ipAddress;
  mqttPort = port;
  client.setServer(mqttServer, mqttPort);
}

void HotTubMqtt::setCredentials(char *user, char *pass)
{
  mqttUser = user;
  mqttPass = pass;
  hasCredentials = true;
}

void HotTubMqtt::loop()
{
  if (mqttPort == 0)
    return;

  if (!client.connected())
  {
    reconnect();
    return;
  }

  client.loop();
}

void HotTubMqtt::callback(char *topic, byte *payload, unsigned int length)
{
  char message[length + 1];
  memcpy(message, payload, length);
  message[length] = '\0';

#ifdef DEBUG_MQTT
  Serial.print("MQTT->");
  Serial.print(topic);
  Serial.print("->");
  Serial.println(message);
#endif

  if (strcmp(topic, "hottub/cmnd/pump") == 0 ||
      strcmp(topic, "hottub/cmnd/heater") == 0 ||
      strcmp(topic, "hottub/cmnd/blower") == 0 ||
      strcmp(topic, "hottub/cmnd/temperature/lock") == 0 ||
      strcmp(topic, "hottub/cmnd/autorestart") == 0)
  {
#ifdef DEBUG_MQTT
    Serial.println("MQTT->Handling state message");
#endif

    handleStateMessages(topic, message, length);
    return;
  }

  if (strcmp(topic, "hottub/cmnd/temperature/target") == 0 ||
      strcmp(topic, "hottub/cmnd/temperature/max") == 0)
  {
#ifdef DEBUG_MQTT
    Serial.print("MQTT->Handing value message");
#endif

    handleValueMessages(topic, message, length);
    return;
  }

  if (strcmp(topic, "hottub/cmnd/command") == 0)
  {
#ifdef DEBUG_MQTT
    Serial.println("MQTT->Handing raw command");
#endif

    handleRawCommand(message);
    return;
  }
}

void HotTubMqtt::handleStateMessages(char *topic, char *message, unsigned int length)
{
  const size_t capacity = JSON_OBJECT_SIZE(1) + 20;
  DynamicJsonDocument doc(capacity);
  deserializeJson(doc, message);

  bool enable = false;

  if (doc["state"].is<bool>())
    enable = doc["state"];
  else if (doc["state"].is<const char *>())
    enable = strncmp(doc["state"], "true", 4) == 0;
  else
    return;

  if (strcmp(topic, "hottub/cmnd/pump") == 0)
  {
    hotTub->setTargetState(enable ? PUMP_FILTERING : PUMP_OFF);
    return;
  }

  if (strcmp(topic, "hottub/cmnd/heater") == 0)
  {
    hotTub->setTargetState(enable ? PUMP_HEATING : PUMP_FILTERING);
    return;
  }

  if (strcmp(topic, "hottub/cmnd/blower") == 0)
  {
    hotTub->setTargetState(enable ? PUMP_BUBBLES : PUMP_HEATING); //todo - configurable state after blower turned off??
    return;
  }

  if (strcmp(topic, "hottub/cmnd/temperature/lock") == 0)
  {
    hotTub->setTemperatureLock(enable);
    return;
  }

  if (strcmp(topic, "hottub/cmnd/autorestart") == 0)
  {
    hotTub->setAutoRestart(enable);
    return;
  }
}

void HotTubMqtt::handleValueMessages(char *topic, char *message, unsigned int length)
{
  const size_t capacity = JSON_OBJECT_SIZE(1) + 20;
  DynamicJsonDocument doc(capacity);
  deserializeJson(doc, message);

  int value = doc["value"];

  if (strcmp(topic, "hottub/cmnd/temperature/target") == 0)
  {
    hotTub->setTargetTemperature(value);
    return;
  }

  if (strcmp(topic, "hottub/cmnd/temperature/max") == 0)
  {
    hotTub->setLimitTemperature(value);
    return;
  }
}

unsigned int validateCommand(char *message)
{
  const size_t capacity = JSON_OBJECT_SIZE(1) + 20;
  DynamicJsonDocument doc(capacity);
  deserializeJson(doc, message);

  unsigned int command;

  if (doc["command"].is<unsigned int>())
  {
    command = doc["command"];
  }
  else if (doc["command"].is<const char *>())
  {
    const char *commandStr = doc["command"]; // "0x4000"
    command = strtol(commandStr, 0, 16);
  }
  else
  {
    return 0;
  }

  if (command == 0)
  {
#ifdef DEBUG_MQTT
    Serial.println("MQTT->Commands must be in the format 0x1234");
#endif
    return 0;
  }

  if (command >= 0x4000)
  {
#ifdef DEBUG_MQTT
    Serial.println("MQTT->Commands must be less than 0x4000");
#endif
    return 0;
  }

  return command;
}

void HotTubMqtt::handleRawCommand(char *message)
{
  unsigned int command = validateCommand(message);

  if (command <= 0)
    return;

  if (hotTub->queueCommand(command))
  {
#ifdef DEBUG_MQTT
    Serial.print("MQTT->Sending command 0x");
    Serial.println(command, HEX);
#endif
    return;
  }

#ifdef DEBUG_MQTT
  Serial.println("MQTT->Too many commands in queue!");
#endif
}

void HotTubMqtt::sendStatus()
{
  if (!client.connected())
    return;

#ifdef DEBUG_MQTT
  Serial.println("MQTT->Sending status json...");
#endif

  publish("hottub/state/status", hotTub->getStateJson());
}

void HotTubMqtt::sendConnected()
{
  char temp[10];
  ltoa(millis(), temp, 10);

  publish("hottub/state/connected", temp);
}

void HotTubMqtt::publish(const char *topic, char *payload)
{
  client.publish_P(topic, (const uint8_t *)payload, strnlen(payload, 512), false);
}

void HotTubMqtt::subscribe()
{
  client.subscribe("hottub/cmnd/pump");
  client.subscribe("hottub/cmnd/heater");
  client.subscribe("hottub/cmnd/blower");
  client.subscribe("hottub/cmnd/command");
  client.subscribe("hottub/cmnd/autorestart");
  client.subscribe("hottub/cmnd/temperature/lock");
  client.subscribe("hottub/cmnd/temperature/target");
  client.subscribe("hottub/cmnd/temperature/max");
}

bool HotTubMqtt::connect()
{
  String clientId = "ESP8266mqttClient-" + String(random(0xffff), HEX);

  if (hasCredentials)
    return client.connect(clientId.c_str(), mqttUser, mqttPass);

  return client.connect(clientId.c_str());
}

unsigned long lastConnectAttempt = 0;
void HotTubMqtt::reconnect()
{
  if (millis() < 5000 || millis() - 5000 < lastConnectAttempt)
    return;

  lastConnectAttempt = millis();

  if (!client.connected())
  {
#ifdef DEBUG_MQTT
    Serial.print("MQTT->Connecting...");
#endif

    if (connect())
    {
#ifdef DEBUG_MQTT
      Serial.println("connected");
#endif

      subscribe();
      sendConnected();
      sendStatus();
      return;
    }

#ifdef DEBUG_MQTT
    Serial.print("failed, state = ");

    switch (client.state())
    {
    case -4:
      Serial.print("MQTT_CONNECTION_TIMEOUT");
      break;
    case -3:
      Serial.print("MQTT_CONNECTION_LOST");
      break;
    case -2:
      Serial.print("MQTT_CONNECT_FAILED");
      break;
    case -1:
      Serial.print("MQTT_DISCONNECTED");
      break;
    case 0:
      Serial.print("MQTT_CONNECTED");
      break;
    case 1:
      Serial.print("MQTT_CONNECT_BAD_PROTOCOL");
      break;
    case 2:
      Serial.print("MQTT_CONNECT_BAD_CLIENT_ID");
      break;
    case 3:
      Serial.print("MQTT_CONNECT_UNAVAILABLE");
      break;
    case 4:
      Serial.print("MQTT_CONNECT_BAD_CREDENTIALS");
      break;
    case 5:
      Serial.print("MQTT_CONNECT_UNAUTHORIZED");
      break;
    }

    Serial.println(" try again in 5 seconds");
#endif
  }
}
