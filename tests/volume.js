const { Soundio } = require('../src');
const soundio = new Soundio();

let streamStatus = true;

let currentSample = 0;
const pitch = 440;

const processFrame = (outputBuffer) => {
  const channels = outputBuffer.map((buf) => new Float32Array(buf.buffer));
  const radPerSecond = Math.PI * 2 * pitch;

  for (let sample = 0; sample < channels[0].length; sample++) {
    const sinSample = Math.sin(radPerSecond * (currentSample / 48000));
    channels[0][sample] = sinSample;
    channels[1][sample] = sinSample;
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
  bufferDuration: 0.1,
});
console.log('Starting stream');
soundio.startOutputStream();


const triggerVolumeDown = () => {
  console.log('Volume down');
  soundio.setOutputVolume(0.5);
  setTimeout(() => {
    console.log('Volume up');
    soundio.setOutputVolume(1);
    setTimeout(triggerVolumeDown, 1000);
  }, 1000);
}

setTimeout(triggerVolumeDown, 500);

setTimeout(() => {
  console.log('Stopping stream');
  streamStatus = false;
  setTimeout(() => {
    process.exit(0);
  }, 1000);
}, 10000);
