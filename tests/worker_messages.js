const path = require('path');
const {RtAudio, RtAudioFormat} = require('../');

const rtaudio = new RtAudio();

rtaudio.openStream({ nChannels: 2 }, null, RtAudioFormat.RTAUDIO_SINT16, 48000, 480, '');

const worklet = rtaudio.attachProcessFunctionFromWorker(path.resolve(__dirname, './workers/messages.js'));
rtaudio.start();

setTimeout(() => {
  console.log('Muting worklet');
  worklet.postMessage({
    mute: true,
  });
}, 1000);


setTimeout(() => {
  process.exit(0);
}, 2000);
