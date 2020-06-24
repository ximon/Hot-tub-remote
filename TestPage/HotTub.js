export class HotTub {
  constructor(ip, outputElement) {
    this.ip = ip;
    this.websocketPort = 81;
    this.outputElement = outputElement;

    this.state = {
      targetState: {
        pumpState: 0,
        state: "",
        targetTemperature: 0,
      },
      currentState: {
        pumpState: 0,
        state: "",
        targetTemperature: 0,
      },
      autoRestart: false,
      errorCode: 0,
      limitTemperature: 40,
      temperatureLock: false,
    };

    //this.connect();
  }

  /*
  connect() {
    this.websocket = new WebSocket(`ws://${this.ip}:${this.websocketPort}`);

    // Listen for messages
    this.websocket.addEventListener("message", (event) => {
      this.state = JSON.parse(event.data);
    });
  }
  */

  async http(uri, method) {
    this.setText("");

    try {
      var response = await fetch(`http://${this.ip}/${uri}`, {
        method: method,
        mode: "cors",
        cache: "no-cache",
        headers: {
          "Content-Type": "text/plain",
        },
      });

      var data = await response.json();

      var json = JSON.stringify(data, null, 2);
      this.setText((response.ok ? "SUCCESS: " : "ERROR: ") + json);

      return data;
    } catch (err) {
      this.setText(err);
    }
  }

  setText(text) {
    document.getElementById(this.outputElement).innerHTML = text;
  }

  async post(uri) {
    return await this.http(uri, "POST");
  }

  async sendCommand(command) {
    await this.post(`command?command=${command}`);
  }

  async device(deviceType, state) {
    await this.post(`${deviceType}?state=${state}`);
  }

  async targetTemp(temp) {
    await this.post(`temperature/target?value=${temp}`);
  }

  async maxTemp(temp) {
    await this.post(`temperature/max?value=${temp}`);
  }

  async heater(state) {
    await this.device("heater", state);
  }

  async pump(state) {
    await this.device("pump", state);
  }

  async blower(state) {
    await this.device("blower", state);
  }

  async tempUp() {
    await this.sendCommand("0x1A30");
  }

  async tempDown() {
    await this.sendCommand("0x1840");
  }

  async tempLock(state) {
    await this.post(`temperature/lock?state=${state}`);
  }

  async autoRestart(state) {
    await this.post(`autoRestart?state=${state}`);
  }

  updateState(state) {
    this.state = state;
  }

  async getStatus() {
    var newState = await this.http("status", "GET");

    this.updateState(newState);
  }

  stateToString(state) {
    switch (state) {
      case -1:
        return "UNKNOWN";
      case 0:
        return "Off";
      case 1:
        return "Filtering";
      case 2:
        return "Heater Standby";
      case 3:
        return "Heating";
      case 4:
        return "Bubbles";
      case 5:
        return "End";
      case 6:
        return "Error";
      case 7:
        return "Flashing";
    }
  }
}
