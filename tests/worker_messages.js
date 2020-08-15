const path = require('path');
const { AudioServer } = require('../');
const audioServer = new AudioServer();

const main = async () => {
  const device = audioServer.getDefaultOutputDevice();
  const outputStream = audioServer.initOutputStream(device.id, {
    format: AudioServer.F32LE,
  });

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
}
main();
