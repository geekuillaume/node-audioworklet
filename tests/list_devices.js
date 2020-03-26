const { Soundio } = require('../');

const soundio = new Soundio();
console.log(JSON.stringify(soundio.getDevices(), null, 2))

console.log('default output:', soundio.getDefaultOutputDeviceIndex());
console.log('default input:', soundio.getDefaultInputDeviceIndex());
console.log('API:', soundio.getApi());
