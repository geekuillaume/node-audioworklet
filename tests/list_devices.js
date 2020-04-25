const { Soundio } = require('../');

const soundio = new Soundio();

const logDevice = (device) => {
  console.log('---------------');
  for (let prop in device) {
    console.log(`${prop}:`, device[prop]);
  }
}

soundio.getDevices().outputDevices.forEach(logDevice);
soundio.getDevices().inputDevices.forEach(logDevice);

console.log('-------')

console.log('default output:', soundio.getDefaultOutputDevice().name);
console.log('default input:', soundio.getDefaultInputDevice().name);
console.log('API:', soundio.getApi());
