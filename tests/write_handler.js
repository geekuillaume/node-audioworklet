const { AudioServer } = require('../');
const audioServer = new AudioServer();

const SAMPLE_RATE = 48000;
const CHANNELS = 2;

const BUFFER_SIZE_TARGET = 0.5 * SAMPLE_RATE; // 500ms

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

  console.log(`Initialized stream, buffer capacity: ${stream.getBufferCapacity()}, buffer filled size: ${stream.getBufferSize()}`);

  stream.registerWriteHandler((buffer) => {
    for (let sample = 0; sample < buffer.length / CHANNELS; sample++) {
      const sinSample = Math.sin(radPerSecond * (currentSample / 48000));
      for (let channel = 0; channel < CHANNELS; channel++) {
        buffer[(sample * CHANNELS) + channel] = sinSample;
      }
      currentSample += 1;
    }
    return true;
  }, {
    targetBufferSize: SAMPLE_RATE * 0.1,
    interval: 20,
  });

  console.log('Starting stream');
  stream.start();

  const logInterval = setInterval(() => {
    console.log(`buffer capacity: ${stream.getBufferCapacity()}, buffer filled size: ${stream.getBufferSize()}`);
  }, 500);

  setTimeout(() => {
    console.log('Stopping stream');
    stream.stop();
    clearInterval(logInterval);
  }, 20000);
}
main();
