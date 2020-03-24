const path = require('path');
const { Soundio } = require('../');
const soundio = new Soundio();

soundio.openOutputStream({
  format: Soundio.SoundIoFormatS16LE,
  sampleRate: 48000,
  name: "test test",
});

soundio.attachProcessFunctionFromWorker(path.resolve(__dirname, './workers/whitenoise.js'));
soundio.startOutputStream();

setTimeout(() => {
  console.log('exiting');
  process.exit(0);
}, 1000);
