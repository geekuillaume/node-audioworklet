// const {RtAudio, RtAudioFormat} = require('../');

// const rtaudio = new RtAudio();

// let streamStatus = true;

// console.log('Opening stream');
// rtaudio.openStream(
// 	{ deviceId: rtaudio.getDefaultOutputDevice(), // Output device id (Get all devices using `getDevices`)
// 	  nChannels: 1, // Number of channels
// 	  firstChannel: 0 // First channel index on device (default = 0).
// 	},
// 	{ deviceId: rtaudio.getDefaultInputDevice(), // Input device id (Get all devices using `getDevices`)
// 	  nChannels: 1, // Number of channels
// 	  firstChannel: 0 // First channel index on device (default = 0).
// 	},
// 	RtAudioFormat.RTAUDIO_SINT16, // PCM Format - Signed 16-bit integer
// 	48000, // Sampling rate is 48kHz
// 	1920, // Frame size is 1920 (40ms)
// 	"MyStream", // The name of the stream (used for JACK Api)
//   (inputData, outputData) => {
//     inputData[0].copy(outputData[0])
//     return streamStatus;
//   }
// );

// console.log('Starting stream');
// rtaudio.start();

// setTimeout(() => {
//   console.log('Stopping stream');
// 	streamStatus = false;
//   process.exit(0);
// }, 2000);


