const path = require('path');
const {RtAudio, RtAudioFormat} = require('../');

const rtaudio = new RtAudio();

rtaudio.openStream({ nChannels: 2 }, null, RtAudioFormat.RTAUDIO_SINT16, 48000, 480, '');

rtaudio.attachProcessFunctionFromWorker(path.resolve(__dirname, './workers/whitenoise.js'));
rtaudio.start();
