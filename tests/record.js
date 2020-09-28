const { AudioServer } = require('../');
const audioServer = new AudioServer();

const main = async () => {
  const inDevice = audioServer.getDefaultInputDevice();
  console.log(`Recording from ${inDevice.name}`);

  const CHANNELS = 2;
  const SAMPLE_RATE = 48000;
  console.log(inDevice, AudioServer.S16LE);
  const inputStream = audioServer.initInputStream(inDevice.id, {
    sampleRate: SAMPLE_RATE,
    format: AudioServer.S16LE,
    channels: CHANNELS,
  });

  inputStream.registerReadHandler((chunk) => {
    console.log(chunk);
  });

  inputStream.start();

  setTimeout(() => {
    inputStream.stop();
  }, 10000);
}

main();
