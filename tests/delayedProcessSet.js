const {RtAudio, RtAudioFormat} = require('../');

const rtaudio = new RtAudio();

let streamStatus = true;

const processFrame = (inputData, outputData) => {
  for (let i = 0; i < outputData[0].length; i++) {
    outputData[0][i] = Math.floor(Math.random() * 16000);
  }
  return streamStatus;
}

console.log('Opening stream');
rtaudio.openStream({ nChannels: 2 }, null, RtAudioFormat.RTAUDIO_SINT16, 48000, 480, '');
console.log('Starting stream');
rtaudio.start();

setTimeout(() => {
  console.log('Setting process function');
  rtaudio.setProcessFunction(processFrame);
}, 500);

setTimeout(() => {
  console.log('Stopping stream');
  streamStatus = false;
  process.exit(0);
}, 2000);
