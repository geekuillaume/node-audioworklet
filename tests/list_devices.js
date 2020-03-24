const { Soundio } = require('../');

const soundio = new Soundio();
console.log(soundio.getDevices())

console.log('default output', soundio.getDefaultOutputDeviceIndex());
console.log('default input', soundio.getDefaultInputDeviceIndex());
console.log('API:', soundio.getApi());
