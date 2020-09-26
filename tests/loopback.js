const { AudioServer } = require('../');
const audioServer = new AudioServer();

const main = async () => {
  const inDevice = audioServer.getDefaultInputDevice();
  const outDevice = audioServer.getDefaultOutputDevice();
  console.log(`Recording from ${inDevice.name} and sending to ${outDevice.name}`);

  const CHANNELS = 1;
  const SAMPLE_RATE = 48000;

  const inputStream = audioServer.initInputStream(inDevice.id, {
    sampleRate: SAMPLE_RATE,
    format: AudioServer.F32LE,
    channels: CHANNELS,
  });

  const outputStream = audioServer.initOutputStream(outDevice.id, {
    sampleRate: SAMPLE_RATE,
    format: AudioServer.F32LE,
    channels: CHANNELS,
  });

  inputStream.start();

  inputStream.registerReadHandler((buffer) => {
    outputStream.pushAudioChunk(undefined, buffer);
    // console.log(buffer.length);
  });

  setTimeout(() => {
    // let some time for the outputStream buffer to fill from the read handler
    outputStream.start();
  }, 500);

  setTimeout(() => {
    console.log('Stopping');
    inputStream.stop();
    outputStream.stop();
  }, 10000);
}

main();
