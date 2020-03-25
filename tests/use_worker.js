const path = require('path');
const { Soundio } = require('../');
const soundio = new Soundio();

soundio.openOutputStream();

soundio.attachProcessFunctionFromWorker(path.resolve(__dirname, './workers/whitenoise.js'));
soundio.startOutputStream();

setTimeout(() => {
  console.log('exiting');
  process.exit(0);
}, 1000);
