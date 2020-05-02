import { HotTub } from './HotTub.js';

const DEFAULT_IP = '192.168.0.44';
const ip = getIp();

document.getElementById('ipAddress').innerHTML = ip;
		
function getIp() {
	const urlParams = new URLSearchParams(location.search);
	const ip = urlParams.has('ip') ? urlParams.get('ip') : DEFAULT_IP;
	const regex = /^(?:(25[0-5]|2[0-4][0-9]|1[0-9][0-9]|[1-9][0-9]|[0-9])(\.(?!$)|$)){4}$/gm;
	
	return ip.match(regex) ? ip : DEFAULT_IP;
}

function addIdClick(id, func) {
	document.getElementById(id).addEventListener('click', func);
}

function addClassClick(className, func) {
	var elements = document.getElementsByClassName(className);

	Array
		.from(elements)
		.forEach(e => e.addEventListener('click', func));
}

var tub = new HotTub(ip, 'response');

addIdClick('getStatus', e => tub.getStatus(e));
addIdClick('tempUp', e => tub.tempUp(e));
addIdClick('tempDown', e => tub.tempDown(e));

addClassClick('tempLock', e => tub.tempLock(e.target.dataset.state));
addClassClick('autoRestart', e => tub.autoRestart(e.target.dataset.state));
addClassClick('maxTemp', e => tub.maxTemp(e.target.dataset.temp));
addClassClick('targetTemp', e => tub.targetTemp(e.target.dataset.temp));

addClassClick('blower', e => tub.blower(e.target.dataset.state));
addClassClick('pump', e => tub.pump(e.target.dataset.state));
addClassClick('heater', e => tub.heater(e.target.dataset.state));





