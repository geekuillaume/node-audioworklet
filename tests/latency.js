const { AudioServer } = require('../');
const audioServer = new AudioServer();

let streamStatus = true;

const sampleRate = 48000;
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

const main = async () => {
  const device = audioServer.getDefaultOutputDevice();
  console.log('Opening stream');
  const outputStream = audioServer.initOutputStream(device.id, {
    sampleRate,
    format: AudioServer.F32LE,
    process: processFrame,

  });

  console.log('Starting stream');
  outputStream.start();

  const triggerBeep = () => {
    if (!outputStream.isStarted()) {
      return;
    }
    currentPitch = pitch;
    const latency = outputStream.getLatency();
    console.log(`Latency: ${latency} frames - ${Math.round((latency / sampleRate) * 1000)}ms`);
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
  }, 10000);
}
main();
