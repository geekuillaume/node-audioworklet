const { Soundio } = require('../src');
const soundio = new Soundio();

let streamStatus = true;

let currentSample = 0;
const pitch = 440;
let currentPitch = 0;

const processFrame = (outputBuffer) => {
  const channels = outputBuffer.map((buf) => new Float32Array(buf.buffer));
  const radPerSecond = Math.PI * 2 * currentPitch;

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


const triggerBeep = () => {
  currentPitch = pitch;
  console.log(`Latency: ${soundio.getStreamLatency()}s`);
  console.log('Beep');
  setTimeout(() => {
    console.log('End beep');
    currentPitch = 0;
    setTimeout(triggerBeep, 1000);
  }, 1000);
}

setTimeout(triggerBeep, 500);

setTimeout(() => {
  console.log('Stopping stream');
  streamStatus = false;
  setTimeout(() => {
    process.exit(0);
  }, 1000);
}, 10000);
