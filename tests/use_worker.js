const path = require('path');
const { Soundio } = require('../');
const soundio = new Soundio();

const device = soundio.getDefaultOutputDevice();
const outputStream = device.openOutputStream();

outputStream.attachProcessFunctionFromWorker(path.resolve(__dirname, './workers/whitenoise.js'));
outputStream.start();

setTimeout(() => {
  console.log('exiting');
  process.exit(0);
}, 1000);
