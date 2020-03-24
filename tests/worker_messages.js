const path = require('path');
const { Soundio } = require('../');
const soundio = new Soundio();

soundio.openOutputStream({
  format: Soundio.SoundIoFormatS16LE,
  sampleRate: 48000,
  name: "test test",
});

const worklet = soundio.attachProcessFunctionFromWorker(path.resolve(__dirname, './workers/messages.js'));
soundio.startOutputStream();

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
