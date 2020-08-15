const { AudioServer } = require('../');
const audioServer = new AudioServer();

const main = async () => {
  const inDevice = audioServer.getDefaultInputDevice();
  const outDevice = audioServer.getDefaultOutputDevice();
  console.log(`Recording from ${inDevice.name} and sending to ${outDevice.name}`);

  const CHANNELS = 1;
  const SAMPLE_RATE = 48000;
  const BUFFER_DURATION_IN_SECONDS = 20;
  const buffer = new Float32Array(CHANNELS * SAMPLE_RATE * BUFFER_DURATION_IN_SECONDS);
  let inPtr = SAMPLE_RATE * 2; // 2 seconds in the future
  let outPtr = 0;

  const inputStream = audioServer.initInputStream(inDevice.id, {
    sampleRate: SAMPLE_RATE,
    format: AudioServer.F32LE,
    channels: 1,
    process: (inputChannels) => {
      buffer.set(inputChannels[0], inPtr);
      inPtr = (inPtr + inputChannels[0].length) % buffer.length;
      return true;
    },
  });

  const outputStream = audioServer.initOutputStream(outDevice.id, {
    sampleRate: SAMPLE_RATE,
    format: AudioServer.F32LE,
    process: (outputChannels) => {
      outputChannels.forEach((channel) => {
        channel.set(buffer.slice(outPtr, outPtr + outputChannels[0].length));
      });
      outPtr = (outPtr + outputChannels[0].length) % buffer.length;
      return true;
    }
  });

  inputStream.start();
  outputStream.start();

  setTimeout(() => {
    inputStream.stop();
    outputStream.stop();
  }, 10000);
}

main();
