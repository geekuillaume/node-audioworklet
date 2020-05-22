const { Soundio } = require('../');
const soundio = new Soundio();

let streamStatus = true;

const processFrame = (outputChannels) => {
  for (let sample = 0; sample < outputChannels[0].length; sample++) {
    outputChannels[0][sample] = Math.random();
    outputChannels[1][sample] = Math.random();
  }

  return streamStatus;
}
const main = async () => {
  await soundio.refreshDevices();

  const device = soundio.getDefaultOutputDevice();
  console.log('Opening stream');
  const outputStream = device.openOutputStream({
    format: Soundio.SoundIoFormatFloat32LE,
    sampleRate: 48000,
    name: "test test",
    process: processFrame,
  });

  console.log('Starting stream');
  outputStream.start();

  setTimeout(() => {
    console.log('Stopping stream');
    // streamStatus = false;
    outputStream.close();
  }, 2000);
  setTimeout(() => {
    process.exit(0);
  }, 3000);
}
main();
