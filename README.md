# Hot Tub Remote
This is a WiFi remote using a Esp8266 in the form of a Wemos D1 mini.
It enables remote control of a Lay-z-Spa (3 wire model, #54075) via a Web API.

## Features
The following features can be controlled:
* Turn pump, heater and bubbles on / off
* Adjust temperature and max temperature
* View pump, heater and bubbles status
* View current temperature
* Lock temperature setting - revert any changes made by the control panel
* Auto-restart - the pump turns off after 24H, this will re-start the pump automatically

## Web API
The following URIs are available:

|Uri|Method|Sample data|
|--|:--:|--|
| /autoRestart | POST | `{ "state": true }` |
| /pump | POST | `{ "state": true }` |
| /blower | POST | `{ "state": true }` |
| /heater | POST | `{ "state": true}` |
| /temperature/target | POST | `{ "value": 37 }` |
| /temperature/limit | POST | `{ "value": 40 }` |
| /temperature/lock | POST | `{ "state": true }` |
| /status | GET  | `{ "currentState": { "pumpState": 1, "temperature": 35 }, "targetState": { "pumpState": 3, temperature": 35 }, "autoRestart": true, "temperatureLock": true, "limitTemperature": 40, "errorCode": 0 }` |

## MQTT
The following Topics are available:

|Topic|Sample Data|
|--|--|
| /hottub/cmnd/autoRestart | `{ "state": true }` |
| /hottub/cmnd/pump | `{ "state": true }` |
| /hottub/cmnd/blower | `{ "state": true }` |
| /hottub/cmnd/heater | `{ "state": true }` |
| /hottub/cmnd/temperature/target |`{ "value": 37 }` |
| /hottub/cmnd/temperature/limit | `{ "value": 40 }` |
| /hottub/cmnd/temperature/lock | `{ "state": true }` |


On any state change a message containg the complete status is sent to the following topic:
/hottub/state/status


## Controller interface
The esp8266 connects to the hot tub via a 1 bit bus bidirectional transceiver (74LVC1T45 in this case).
The hot side is 5v, the esp8266 side is 3v3.
The switch in the circuit is to allow local programming, when rx is connected to d2 and the bus transceiver it prevents receiving serial data.

![Interface circuit](https://raw.githubusercontent.com/ximon/Hot-tub-remote/master/Interface.png "Interface circuit")

All available commands were recorded using an OLS Logic sniffer and the [signals.xml](https://raw.githubusercontent.com/ximon/Hot-tub-remote/master/signals.xml) file was created using [WaveMe](https://waveme.weebly.com/)