# Hot Tub Remote
This is a WiFi remote using a Esp8266 in the form of a Wemos D1 mini.
It enables remote control of a Lay-z-Spa (3 wire model, #54075) via a Web API.

## Features
The following features can be controlled:
* Turn pump, heater and bubbles on / off
* Adjust temperature and max temperature
* View pump, heater and bubbles status
* View current temperature
* Lock control panel - revert any changes made by the control panel
* Auto-restart - the pump turns off after 24H, this will re-start the pump automatically

## Web API
The following URIs are available:

|Uri|Method|Sample data|
|--|:--:|--|
| /autoRestart | POST | `{ "state":"on" }` |
| /pump | POST | `{ "state": "on" }` |
| /blower | POST | `{ "state": "on" }` |
| /temperature | POST | `{ "value": 37, "max" : 38 }` |
| /locks | POST | `{ "panel": "on", "temperature": "on" }` |
| /states | GET  | `{ "autoRestart":"off", "pump":"off", "blower":"off", "heater":"off", "temperature":37, "targetTemperature":37 }` |

## Controller interface schematic
![Interface circuit](https://raw.githubusercontent.com/ximon/Hot-tub-remote/master/Interface.png "Interface circuit")

All available commands were recorded using an OLS Logic sniffer and the [signals.xml](https://raw.githubusercontent.com/ximon/Hot-tub-remote/master/signals.xml) file was created using [WaveMe](https://waveme.weebly.com/)