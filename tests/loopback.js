const { Soundio } = require('../');
const soundio = new Soundio();

const main = async () => {
  await soundio.refreshDevices();

  const inDevice = soundio.getDefaultInputDevice();
  const outDevice = soundio.getDefaultOutputDevice();
  console.log(`Recording from ${inDevice.name} and sending to ${outDevice.name}`);

  const CHANNELS = 1;
  const SAMPLE_RATE = 48000;
  const BUFFER_DURATION_IN_SECONDS = 20;
  const buffer = new Float32Array(CHANNELS * SAMPLE_RATE * BUFFER_DURATION_IN_SECONDS);
  let inPtr = SAMPLE_RATE * 2; // 2 seconds in the future
  let outPtr = 0;

  const inputStream = inDevice.openInputStream({
    sampleRate: SAMPLE_RATE,
    format: Soundio.SoundIoFormatFloat32LE,
    process: (inputChannels) => {
      buffer.set(inputChannels[0], inPtr);
      inPtr += inputChannels[0].length;
      return true;
    },
  });

  const outputStream = outDevice.openOutputStream({
    sampleRate: SAMPLE_RATE,
    format: Soundio.SoundIoFormatFloat32LE,
    process: (outputChannels) => {
      outputChannels.forEach((channel) => {
        channel.set(buffer.slice(outPtr, outPtr + outputChannels[0].length));
      });
      outPtr += outputChannels[0].length;
      return true;
    }
  });
  inputStream.start();
  outputStream.start();

  setTimeout(() => {
    inputStream.close();
    outputStream.close();
  }, 10000);
}

main();
