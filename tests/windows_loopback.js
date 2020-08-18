const { AudioServer } = require('../');
const audioServer = new AudioServer();

const main = async () => {
  const outDevice = audioServer.getDefaultOutputDevice();
  console.log(`Recording from ${outDevice.name}`);

  const CHANNELS = 1;
  const SAMPLE_RATE = 48000;
  const BUFFER_DURATION_IN_SECONDS = 20;
  // const buffer = new Float32Array(CHANNELS * SAMPLE_RATE * BUFFER_DURATION_IN_SECONDS);
  // let inPtr = SAMPLE_RATE * 2; // 2 seconds in the future
  // let outPtr = 0;

  const inputStream = audioServer.initInputStream(outDevice.id, {
    sampleRate: SAMPLE_RATE,
    format: AudioServer.F32LE,
    channels: 1,
    process: (inputChannels) => {
      console.log(inputChannels);
      return true;
    },
  }, true);

  inputStream.start();

  setTimeout(() => {
    inputStream.stop();
  }, 10000);
}

main();
