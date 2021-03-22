# Node Audioworklet

Node Audioworklet is a NodeJS lib exposing a set of function to output audio on an audio card. It uses [cubeb](https://github.com/kinetiknz/cubeb) to support a wide variety of OS and configuration ([details](https://github.com/kinetiknz/cubeb/wiki/Backend-Support)). It only handle raw PCM frames that should be set into a Buffer for every frame. A timestamped audio buffer is used to transmit the raw data to the OS audio system, this can be used to precisely time when the audio samples will be outputted.

Look in the `tests/` folder for more informations about the options available in the lib.

This project has been built mainly for [Soundsync](https://soundsync.app).

## Installation

```
npm install --save audioworklet
```

Or

```
yarn add audioworklet
```

## Example

### List devices

```javascript
const { AudioServer } = require('audioworklet');
 
const audioServer = new AudioServer();
 
const logDevice = (device) => {
  console.log('---------------');
  for (let prop in device) {
    console.log(`${prop}:`, device[prop]);
  }
}
 

audioServer.getDevices().outputDevices.forEach(logDevice);
audioServer.getDevices().inputDevices.forEach(logDevice);

console.log('-------')

console.log('default output:', audioServer.getDefaultOutputDevice()?.name);
console.log('default input:', audioServer.getDefaultInputDevice()?.name);
console.log('API:', audioServer.getApi());
```

Will output:

```
---------------
name: Built-in Audio Analog Stereo
id: alsa_output.pci-0000_00_1f.3.analog-stereo
formats: [
   2,  3, 4, 15, 16,
  11, 12, 7,  8
]
sampleRates: [ { min: 8000, max: 5644800 } ]
channelLayouts: [
  { name: 'Mono', channelCount: 1 },
  { name: 'Stereo', channelCount: 2 },
  { name: '2.1', channelCount: 3 },
  { name: '3.0', channelCount: 3 },
  { name: '3.0 (back)', channelCount: 3 },
  { name: '3.1', channelCount: 4 },
  { name: '4.0', channelCount: 4 },
  { name: 'Quad', channelCount: 4 },
  { name: 'Quad (side)', channelCount: 4 },
  { name: '4.1', channelCount: 5 },
  { name: '5.0 (back)', channelCount: 5 },
  { name: '5.0 (side)', channelCount: 5 },
  { name: '5.1', channelCount: 6 },
  { name: '5.1 (back)', channelCount: 6 },
  { name: '6.0 (side)', channelCount: 6 },
  { name: '6.0 (front)', channelCount: 6 },
  { name: 'Hexagonal', channelCount: 6 },
  { name: '6.1', channelCount: 7 },
  { name: '6.1 (back)', channelCount: 7 },
  { name: '6.1 (front)', channelCount: 7 },
  { name: '7.0', channelCount: 7 },
  { name: '7.0 (front)', channelCount: 7 },
  { name: '7.1', channelCount: 8 },
  { name: '7.1 (wide)', channelCount: 8 },
  { name: '7.1 (wide) (back)', channelCount: 8 },
  { name: 'Octagonal', channelCount: 8 }
]
isInput: false
isOutput: true
---------------
name: Webcam C930e Analog Stereo
id: alsa_input.usb-046d_Logitech_Webcam_C930e_1658212E-02.analog-stereo
formats: [
   2,  3, 4, 15, 16,
  11, 12, 7,  8
]
sampleRates: [ { min: 8000, max: 5644800 } ]
channelLayouts: [
  { name: 'Mono', channelCount: 1 },
  { name: 'Stereo', channelCount: 2 },
  { name: '2.1', channelCount: 3 },
  { name: '3.0', channelCount: 3 },
  { name: '3.0 (back)', channelCount: 3 },
  { name: '3.1', channelCount: 4 },
  { name: '4.0', channelCount: 4 },
  { name: 'Quad', channelCount: 4 },
  { name: 'Quad (side)', channelCount: 4 },
  { name: '4.1', channelCount: 5 },
  { name: '5.0 (back)', channelCount: 5 },
  { name: '5.0 (side)', channelCount: 5 },
  { name: '5.1', channelCount: 6 },
  { name: '5.1 (back)', channelCount: 6 },
  { name: '6.0 (side)', channelCount: 6 },
  { name: '6.0 (front)', channelCount: 6 },
  { name: 'Hexagonal', channelCount: 6 },
  { name: '6.1', channelCount: 7 },
  { name: '6.1 (back)', channelCount: 7 },
  { name: '6.1 (front)', channelCount: 7 },
  { name: '7.0', channelCount: 7 },
  { name: '7.0 (front)', channelCount: 7 },
  { name: '7.1', channelCount: 8 },
  { name: '7.1 (wide)', channelCount: 8 },
  { name: '7.1 (wide) (back)', channelCount: 8 },
  { name: 'Octagonal', channelCount: 8 }
]
isInput: true
isOutput: false
-------
default output: Built-in Audio Analog Stereo
default input: Webcam C930e Analog Stereo
API: PulseAudio
```

### Play white noise

```javascript
const { AudioServer } = require('audioworklet');
const audioServer = new AudioServer();

const SAMPLE_RATE = 48000;
const CHANNELS = 2;

(async () => {
  const device = audioServer.getDefaultOutputDevice();
  console.log('Opening stream');
  const outputStream = audioServer.initOutputStream(device.id, {
    format: AudioServer.F32LE,
    sampleRate: SAMPLE_RATE,
    channels: CHANNELS,
    name: "test",
  });

  const buffer = new Float32Array(SAMPLE_RATE * CHANNELS * 5); // 5 seconds buffer
  for (let sample = 0; sample < buffer.length / CHANNELS; sample++) {
    // fill with interleaved data
    buffer[sample * CHANNELS] = Math.random();
    buffer[(sample * CHANNELS) + 1] = Math.random();
  }
  outputStream.pushAudioChunk(undefined, buffer);
  // first argument is the chunk timestamp in the audio device clock domain, if undefined, will be contiguous to the last pushed chunk

  console.log('Starting stream');
  outputStream.start();

  setTimeout(() => {
    console.log('Stopping stream');
    outputStream.stop();
  }, 2000);
})()
```

## Development

### Download cubeb

```
git submodule update --init --recursive
git clone https://github.com/djg/cubeb-pulse-rs.git vendor/cubeb/src/cubeb-pulse-rs
git clone https://github.com/ChunMinChang/cubeb-coreaudio-rs vendor/cubeb/src/cubeb-coreaudio-rs
cd vendor/cubeb
```

### Requirements for source build

***Most regular installs will support prebuilds that are built with each release, this is required if you want to develop with.***

* Node version that support N-API 4 and up ([N-API Node Version Matrix](https://nodejs.org/docs/latest/api/n-api.html#n_api_n_api_version_matrix))
* [CMake](http://www.cmake.org/download/)
* A proper C/C++ compiler toolchain of the given platform
    * **Windows**:
        * [Windows build tools](https://www.npmjs.com/package/windows-build-tools) NPM package
    * **Unix/Posix**:
        * Clang or GCC
        * Ninja or Make (Ninja will be picked if both present)
        * Alsa and/or Pulseaudio libs (on ubuntu, it can be installed with `apt-get install -y libasound2-dev libpulse-dev`)

## Legal

This project is licensed under the MIT license.
