const path = require('path');
const { AudioServer } = require('../');
const audioServer = new AudioServer();

const main = async () => {
  const device = audioServer.getDefaultOutputDevice();
  const outputStream = audioServer.initOutputStream(device.id, {
    format: AudioServer.F32LE,
  });

  outputStream.attachProcessFunctionFromWorker(path.resolve(__dirname, './workers/whitenoise.js'));
  outputStream.start();

  setTimeout(() => {
    console.log('exiting');
    process.exit(0);
  }, 1000);
}
main();
