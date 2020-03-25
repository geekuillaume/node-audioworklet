const { Soundio } = require('../');
const soundio = new Soundio();

let streamStatus = true;

let currentSample = 0;
const pitch = 440;
let currentPitch = 0;

const processFrame = (outputChannels) => {
  const radPerSecond = Math.PI * 2 * currentPitch;

  for (let sample = 0; sample < outputChannels[0].length; sample++) {
    const sinSample = Math.sin(radPerSecond * (currentSample / 48000));
    outputChannels[0][sample] = sinSample;
    outputChannels[1][sample] = sinSample;
    currentSample += 1;
  }

  return streamStatus;
}

console.log('Opening stream');
soundio.openOutputStream({
  format: Soundio.SoundIoFormatFloat32LE,
  sampleRate: 48000,
  name: "Latency",
  process: processFrame,
  bufferDuration: 0.05,
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
