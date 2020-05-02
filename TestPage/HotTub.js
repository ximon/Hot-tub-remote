export class HotTub {

	constructor(ip, outputElement) {
		this.ip = ip;
		this.outputElement = outputElement;
	}
	
	async http(uri, method) {
		this.setText("");
		
		try {
			var response = await fetch(`http://${this.ip}/${uri}`, {	
				method: method, 
				mode: 'cors',
				cache: 'no-cache',
				headers: {
					'Content-Type': 'text/plain',
				}
			});
		
			var data = JSON.stringify(await response.json(), null, 2);
			this.setText((response.ok ? 'SUCCESS: ' : 'ERROR: ') + data);
					
		} catch(err) {
			this.setText(err);
		}		
	}			

	setText(text) {
		document.getElementById(this.outputElement).innerHTML = text;
	}

	async post(uri) {
		return await this.http(uri, 'POST');
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
		await this.device('heater', state)
	}
	
	async pump(state) {
		await this.device('pump', state);
	}
	
	async blower(state) {
		await this.device('blower', state);
	}

	async tempUp() {
		await this.sendCommand('0x1A30');
	}
	
	async tempDown() {
		await this.sendCommand('0x1840');
	}
	
	async tempLock(state) {
		await this.post(`temperature/lock?state=${state}`);
	}

	async autoRestart(state) {
		await this.post(`autoRestart?state=${state}`);
	}


	async getStatus() {
		await this.http('status', 'GET');
	}
}
