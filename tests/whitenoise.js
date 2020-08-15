const { AudioServer } = require('../');
const audioServer = new AudioServer();

let streamStatus = true;

const processFrame = (outputChannels) => {
  for (let sample = 0; sample < outputChannels[0].length; sample++) {
    outputChannels[0][sample] = Math.random();
    outputChannels[1][sample] = Math.random();
  }

  return streamStatus;
}
const main = async () => {
  const device = audioServer.getDefaultOutputDevice();
  console.log('Opening stream');
  const outputStream = audioServer.initOutputStream(device.id, {
    format: AudioServer.F32LE,
    sampleRate: 48000,
    name: "test test",
    process: processFrame,
  });

  console.log('Starting stream');
  outputStream.start();

  setTimeout(() => {
    console.log('Stopping stream');
    outputStream.stop();
  }, 2000);
}
main();
