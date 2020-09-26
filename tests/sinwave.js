const { AudioServer } = require('../');
const audioServer = new AudioServer();

const SAMPLE_RATE = 48000;
const CHANNELS = 2;

let currentSample = 0;
const pitch = 440;
const radPerSecond = Math.PI * 2 * pitch;

const main = async () => {
  const device = audioServer.getDefaultOutputDevice();
  console.log(`Opening stream on device ${device.name}`);
  const stream = audioServer.initOutputStream(device.id, {
    format: AudioServer.F32LE,
    sampleRate: SAMPLE_RATE,
    channels: CHANNELS,
  });

  console.log('Starting stream');

  while (currentSample < SAMPLE_RATE * 5) {
    const buffer = new Float32Array(SAMPLE_RATE * CHANNELS * 0.5);
    for (let sample = 0; sample < buffer.length / CHANNELS; sample++) {
      const sinSample = Math.sin(radPerSecond * (currentSample / 48000));
      for (let channel = 0; channel < CHANNELS; channel++) {
        buffer[(sample * CHANNELS) + channel] = sinSample;
      }
      currentSample += 1;
    }
    console.log(`Written ${stream.pushAudioChunk(currentSample - (buffer.length / CHANNELS), buffer)} frames`);
    currentSample += SAMPLE_RATE * 0.1;
  }

  stream.start();

  setTimeout(() => {
    console.log('Stopping stream');
    stream.stop();
  }, 2000);
}
main();
