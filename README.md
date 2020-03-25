# Node Audioworklet

Node Audioworklet is a NodeJS lib exposing a set of function to output audio on an audio card. It's close to the AudioWorklet interface of WebAudio. It uses [libsoundio](http://libsound.io/) to support a wide variety of OS and configuration: Linux (native ALSA, JACK, PulseAudio), Macintosh OS X (CoreAudio and JACK), and Windows (WASAPI). It only handle raw PCM frames that should be set into a Buffer for every frame. You can set the buffer size to precisely time the sound you are emitting.

AudioWorklet also can use another file started in a [NodeJS Worker Thread](https://nodejs.org/api/worker_threads.html) as the process function to isolate the thread and better manage the memory to prevent stop-the-world Garbage Collection from stopping the thread and cause audio artifacts. You need Node v10 or higher to use this feature.

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
const { Soundio } = require('../');

const soundio = new Soundio();
console.log(soundio.getDevices())

console.log('default output:', soundio.getDefaultOutputDeviceIndex());
console.log('default input:', soundio.getDefaultInputDeviceIndex());
console.log('API:', soundio.getApi());
```

Will output:

```
{
  outputDevices: [
    {
      name: 'PCM2704 16-bit stereo audio DAC Digital Stereo (IEC958)',
      id: 'alsa_output.usb-Audioengine_Audioengine_2_-00.iec958-stereo',
      channels: 2
    },
    {
      name: 'Built-in Audio Analog Stereo',
      id: 'alsa_output.pci-0000_00_1f.3.analog-stereo',
      channels: 2
    }
  ],
  inputDevices: [
    {
      name: 'Monitor of PCM2704 16-bit stereo audio DAC Digital Stereo (IEC958)',
      id: 'alsa_output.usb-Audioengine_Audioengine_2_-00.iec958-stereo.monitor',
      channels: 2
    },
    {
      name: 'Webcam C930e Analog Stereo',
      id: 'alsa_input.usb-046d_Logitech_Webcam_C930e_1658212E-02.analog-stereo',
      channels: 2
    },
    {
      name: 'Monitor of Built-in Audio Analog Stereo',
      id: 'alsa_output.pci-0000_00_1f.3.analog-stereo.monitor',
      channels: 2
    },
    {
      name: 'Built-in Audio Analog Stereo',
      id: 'alsa_input.pci-0000_00_1f.3.analog-stereo',
      channels: 2
    }
  ]
}
default output: 1
default input: 3
API: PulseAudio
```

### Play white noise

```javascript
const { Soundio } = require('../');
const soundio = new Soundio();

const processFrame = (outputChannels) => {
  // outputChannels is an array of TypedArray, one per channel, in this example, SoundIoFormatFloat32LE is used so it will be an array of Float32Array
  for (let sample = 0; sample < outputChannels[0].length; sample++) {
    outputChannels[0][sample] = Math.random();
    outputChannels[1][sample] = Math.random();
  }
  // if the function returns false, the stream is paused after this sample
  return true;
}

soundio.openOutputStream({
  format: Soundio.SoundIoFormatFloat32LE, // can be 8,16,32 signed/unsigned int in Big or Little endian and also Float64
  sampleRate: 48000, // in sample per seconds
  name: "Test", // used by some audio backend to show more info about the stream
  frameSize: 480, // size of the output buffer passed to the process function, low means responsive audio stream, high means less CPU usage
  bufferDuration: 0.1, // in seconds, how long in the future to emit samples for, high means more delay but better CPU usage
  process: processFrame,
});
soundio.startOutputStream();
```

### Use another file as AudioWorklet

```javascript
const path = require('path');
const { Soundio } = require('audioworklet');
const soundio = new Soundio();

soundio.openOutputStream();

soundio.attachProcessFunctionFromWorker(path.resolve(__dirname, './workers/whitenoise.js'));
soundio.startOutputStream();

setTimeout(() => {
  console.log('exiting');
  process.exit(0);
}, 1000);
```

And in `./workers/whitenoise.js`:

```javascript
const {AudioWorkletProcessor} = require('../../');

class WhiteNoiseProcessor extends AudioWorkletProcessor {
  constructor() {
    super();
  }

  process(outputChannels) {
    outputChannels.forEach((channel) => {
      for (let sample = 0; sample < channel.length; sample++) {
        channel[sample] = Math.random();
      }
    })

    return true;
  }
}

module.exports = WhiteNoiseProcessor;
```

### Communicate with an AudioWorklet

```javascript
const path = require('path');
const { Soundio } = require('audioworklet');
const soundio = new Soundio();

soundio.openOutputStream();

const worklet = soundio.attachProcessFunctionFromWorker(path.resolve(__dirname, './workers/messages.js'));
soundio.startOutputStream();

setTimeout(() => {
  console.log('Muting worklet');
  worklet.postMessage({
    mute: true,
  });
}, 1000);

setTimeout(() => {
  console.log('exiting');
  process.exit(0);
}, 2000);
```

And in `workers/messages.js`:

```javascript
const {AudioWorkletProcessor} = require('audioworklet');

class WhiteNoiseProcessorWithMessage extends AudioWorkletProcessor {
  constructor() {
    super();
    this.mute = false;

    this.port.onmessage = this.handleMessage.bind(this);
  }

  handleMessage(message) {
    console.log('Receiving mute status', message.data.mute);
    this.mute = message.data.mute;
  }

  process(outputChannels) {
    if (this.mute) {
      return true;
    }
    outputChannels.forEach((channel) => {
      for (let sample = 0; sample < channel.length; sample++) {
        channel[sample] = Math.random();
      }
    })

    return true;
  }
}

module.exports = WhiteNoiseProcessorWithMessage;
```

The `this.port` property is a `MessagePort` and also handle passing a second argument `transferList` to prevent a data copy of ArrayBuffers. Look at the [documentation](https://nodejs.org/api/worker_threads.html#worker_threads_port_postmessage_value_transferlist) for more information.

## Development

### Download libsoundio

```
git submodule update --init --recursive
cd vendor/libsoundio
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

It is based on the [Audify](https://github.com/almogh52/audify) project from Almogh52 also released under MIT license.
