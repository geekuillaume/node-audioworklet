const path = require('path');
const { Soundio } = require('../');
const soundio = new Soundio();

const device = soundio.getDefaultOutputDevice();
const outputStream = device.openOutputStream();

const worklet = outputStream.attachProcessFunctionFromWorker(path.resolve(__dirname, './workers/messages.js'));
outputStream.start();

setTimeout(() => {
  console.log('Muting worklet');
  worklet.postMessage({
    mute: true,
  });
}, 1000);

setTimeout(() => {
  console.log('exiting');
  process.exit(0);
}, 2000);
