const { performance } = require('perf_hooks');
const path = require('path');
const { AudioServer } = require('../');
const audioServer = new AudioServer();

const sampleRate = 48000;
let messageIndex = 0;
const MESSAGE_LOG_INTERVAL = 100;

const main = async () => {
  const device = audioServer.getDefaultOutputDevice();
  console.log('Opening stream');
  const outputStream = audioServer.initOutputStream(device.id, {
    sampleRate,
    format: AudioServer.F32LE,
  });

  const worker = outputStream.attachProcessFunctionFromWorker(path.resolve(__dirname, './workers/position.js'));
  console.log(`Starting stream, start position: ${outputStream.getPosition()}, configured latency: ${outputStream.latencyFrames / (sampleRate / 1000)}`);
  outputStream.start();

  // worker.on('message', ({processPosition}) => {
  //   if (messageIndex % MESSAGE_LOG_INTERVAL === 0) {
  //     const positionTime = Math.round((outputStream.getPosition()) / (sampleRate / 1000));
  //     const processPositionTime = Math.round(processPosition / (sampleRate / 1000));
  //     console.log(`Position: ${positionTime}ms, diff from process position: ${positionTime - processPositionTime}, Diff from now: ${positionTime - performance.now()}`)
  //   }
  //   messageIndex++;
  // })

  setInterval(() => {
    const now = performance.now();
    const positionTime = Math.round((outputStream.getPosition() + outputStream.getLatency()) / (sampleRate / 1000));
    console.log(`Position: ${positionTime}ms, Diff from now: ${positionTime - now}`)
  }, 500);

  setTimeout(() => {
    console.log('Stopping stream');
    outputStream.stop();
  }, 10000);
}
main();
