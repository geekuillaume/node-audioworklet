const { AudioServer } = require('../');
const audioServer = new AudioServer();

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
  const device = audioServer.getDefaultOutputDevice();
  console.log(`Opening stream on device ${device.name}`);
  const stream = audioServer.initOutputStream(device.id, {
    format: AudioServer.F32LE,
    process: processFrame,
    sampleRate: 48000,
    channels: 2,
  });

  console.log('Starting stream');
  stream.start();

  setTimeout(() => {
    console.log('Stopping stream');
    streamStatus = false;
  }, 100);
  // setTimeout(() => {
  //   process.exit(0);
  // }, 1000);
}
main();
