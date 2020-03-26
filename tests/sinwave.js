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

console.log('Opening stream');
soundio.openOutputStream({
  format: Soundio.SoundIoFormatFloat32LE,
  sampleRate: 48000,
  name: "test test",
  process: processFrame,
});
console.log('Starting stream');
soundio.startOutputStream();

setTimeout(() => {
  console.log('Stopping stream');
  streamStatus = false;
}, 2000);
setTimeout(() => {
  process.exit(0);
}, 3000);
