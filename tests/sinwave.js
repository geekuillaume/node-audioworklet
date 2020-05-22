const { Soundio } = require('../');
const soundio = new Soundio();

let streamStatus = true;

let currentSample = 0;
const pitch = 440;
const radPerSecond = Math.PI * 2 * pitch;

const processFrame = (outputChannels) => {
  for (let sample = 0; sample < outputChannels[0].length; sample++) {
    const sinSample = Math.sin(radPerSecond * (currentSample / 48000));
    outputChannels.forEach((channel) => {
      channel[sample] = sinSample;
    });
    currentSample += 1;
  }
  return streamStatus;
}

const main = async () => {
  await soundio.refreshDevices();

  const device = soundio.getDefaultOutputDevice();
  console.log('Opening stream');
  const outputStream = device.openOutputStream({
    format: Soundio.SoundIoFormatFloat32LE,
    process: processFrame,
    sampleRate: 48000,
  });

  console.log('Starting stream');
  outputStream.start();

  setTimeout(() => {
    console.log('Stopping stream');
    streamStatus = false;
  }, 2000);
  setTimeout(() => {
    process.exit(0);
  }, 3000);
}
main();
