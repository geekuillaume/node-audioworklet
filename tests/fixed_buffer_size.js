const { AudioServer } = require('../');
const { assert } = require('console');
const audioServer = new AudioServer();

let streamStatus = true;

let currentSample = 0;
const pitch = 440;
const radPerSecond = Math.PI * 2 * pitch;

const processFrame = (outputChannels) => {
  assert(outputChannels[0].length === 542);
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
  const device = audioServer.getDefaultOutputDevice();
  console.log('Opening stream');
  const outputStream = audioServer.initOutputStream(device.id, {
    format: AudioServer.F32LE,
    sampleRate: 48000,
    frameSize: 542,
    name: "test test",
    process: processFrame,
  });

  console.log('Starting stream');
  outputStream.start();

  setTimeout(() => {
    console.log('Stopping stream');
    streamStatus = false;
  }, 2000);
}
main();
