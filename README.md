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

## Controller interface schematic
![Interface circuit](https://raw.githubusercontent.com/ximon/Hot-tub-remote/master/Interface.png "Interface circuit")

All available commands were recorded using an OLS Logic sniffer and the [signals.xml](https://raw.githubusercontent.com/ximon/Hot-tub-remote/master/signals.xml) file was created using [WaveMe](https://waveme.weebly.com/)