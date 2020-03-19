# Node Audioworklet

This is a NodeJS lib based on the awesome [Audify](https://github.com/almogh52/audify) project to mimic the AudioWorklet interface of WebAudio. It uses [RtAudio](https://www.music.mcgill.ca/~gary/rtaudio/) to support a wide variety of OS and configuration (Linux (native ALSA, JACK, PulseAudio and OSS), Macintosh OS X (CoreAudio and JACK), and Windows (DirectSound, ASIO and WASAPI)). It can be used to output data to speaker or record from an audio input. It differs from Audify as this lib doesn't handle the output buffer but instead calls a function every frame which can read from the input buffer or write into the output buffer.

## Installation
```
npm install audioworklet
```

***Most regular installs will support prebuilds that are built with each release.***

#### Requirements for source build

* Node or Electron versions that support N-API 4 and up ([N-API Node Version Matrix](https://nodejs.org/docs/latest/api/n-api.html#n_api_n_api_version_matrix))
* [CMake](http://www.cmake.org/download/)
* A proper C/C++ compiler toolchain of the given platform
    * **Windows**:
        * [Visual C++ Build Tools](https://visualstudio.microsoft.com/visual-cpp-build-tools/) or a recent version of Visual C++ will do ([the free Community](https://www.visualstudio.com/products/visual-studio-community-vs) version works well)
    * **Unix/Posix**:
        * Clang or GCC
        * Ninja or Make (Ninja will be picked if both present)

## Example

#### Record audio and play it back realtime
```javascript
const { RtAudio, RtAudioFormat } = require("audioworklet");

// Init RtAudio instance using default sound API
const rtAudio = new RtAudio(/* Insert here specific API if needed */);

// Open the input/output stream
rtAudio.openStream(
	{ deviceId: rtAudio.getDefaultOutputDevice(), // Output device id (Get all devices using `getDevices`)
	  nChannels: 1, // Number of channels
	  firstChannel: 0 // First channel index on device (default = 0).
	},
	{ deviceId: rtAudio.getDefaultInputDevice(), // Input device id (Get all devices using `getDevices`)
	  nChannels: 1, // Number of channels
	  firstChannel: 0 // First channel index on device (default = 0).
	},
	RtAudioFormat.RTAUDIO_SINT16, // PCM Format - Signed 16-bit integer
	48000, // Sampling rate is 48kHz
	1920, // Frame size is 1920 (40ms)
	"MyStream", // The name of the stream (used for JACK Api)
  (inputData, outputData) => {
    inputData[0].copy(outputData[0])
    return true; // returning false will stop the stream
  }
);

// Start the stream
rtAudio.start();
```

The `inputData` and `outputData` are two array of 0 or 1 Buffer depending on the configuration. For multi-channels configuration, the data is interleaved.

#### Play white noise

```javascript
const { RtAudio, RtAudioFormat } = require("audioworklet");

// Init RtAudio instance using default sound API
const rtAudio = new RtAudio(/* Insert here specific API if needed */);

// Open the input/output stream
rtAudio.openStream(
	{
	  nChannels: 2, // Number of channels
	},
	null, // no input
	RtAudioFormat.RTAUDIO_SINT16, // PCM Format - Signed 16-bit integer
	48000, // Sampling rate is 48kHz
	1920, // Frame size is 1920 (40ms)
	"MyStream", // The name of the stream (used for JACK Api)
  (inputData, outputData) => {
		for (let i = 0; i < outputData[0].length; i++) {
			outputData[0][i] = Math.floor(Math.random() * 16000);
		}
    return true; // returning false will stop the stream
  }
```

## Legal

This project is licensed under the MIT license.
