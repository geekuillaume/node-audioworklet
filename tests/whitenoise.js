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
