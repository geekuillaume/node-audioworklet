# Node Audioworklet

Node Audioworklet is a NodeJS lib exposing a set of function to output audio on an audio card. It's close to the AudioWorklet interface of WebAudio. It uses [cubeb](https://github.com/kinetiknz/cubeb) to support a wide variety of OS and configuration ([details](https://github.com/kinetiknz/cubeb/wiki/Backend-Support)). It only handle raw PCM frames that should be set into a Buffer for every frame. You can set the buffer size to precisely time the sound you are emitting.

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
const { AudioServer } = require('audioworklet');

const audioServer = new AudioServer();

const logDevice = (device) => {
  console.log('---------------');
  for (let prop in device) {
    console.log(`${prop}:`, device[prop]);
  }
}

(async () => {
  await soundio.refreshDevices();

  soundio.getDevices().outputDevices.forEach(logDevice);
  soundio.getDevices().inputDevices.forEach(logDevice);

  console.log('-------')

  console.log('default output:', soundio.getDefaultOutputDevice().name);
  console.log('default input:', soundio.getDefaultInputDevice().name);
  console.log('API:', soundio.getApi());
})()
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

const processFrame = (outputChannels) => {
  for (let sample = 0; sample < outputChannels[0].length; sample++) {
    outputChannels[0][sample] = Math.random();
    outputChannels[1][sample] = Math.random();
  }

  return true; // returning false will stop the stream
}
(async () => {
  const device = audioServer.getDefaultOutputDevice();
  console.log('Opening stream');
  const outputStream = audioServer.initOutputStream(device.id, {
    format: AudioServer.F32LE,
    sampleRate: 48000,
    name: "test",
    process: processFrame,
  });

  console.log('Starting stream');
  outputStream.start();

  setTimeout(() => {
    console.log('Stopping stream');
    outputStream.stop();
  }, 2000);
})()
```

### Use another file as AudioWorklet

```javascript
const path = require('path');
const { AudioServer } = require('audioworklet');
const audioServer = new AudioServer();

(async () => {
  const device = audioServer.getDefaultOutputDevice();
  const outputStream = audioServer.initOutputStream(device.id, {
    format: AudioServer.F32LE,
  });

  outputStream.attachProcessFunctionFromWorker(path.resolve(__dirname, './workers/whitenoise.js'));
  outputStream.start();

  setTimeout(() => {
    console.log('exiting');
    process.exit(0);
  }, 1000);
})()

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
const { AudioServer } = require('audioworklet');
const audioServer = new AudioServer();

(async () => {
  const device = audioServer.getDefaultOutputDevice();
  const outputStream = audioServer.initOutputStream(device.id, {
    format: AudioServer.F32LE,
  });

  const worklet = outputStream.attachProcessFunctionFromWorker(path.resolve(__dirname, './workers/messages.js'));
  outputStream.start();

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
})()
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

### Download cubeb

```
git submodule update --init --recursive
git clone https://github.com/djg/cubeb-pulse-rs.git vendor/cubeb/src/cubeb-pulse-rs
git clone https://github.com/ChunMinChang/cubeb-coreaudio-rs vendor/cubeb/src/cubeb-coreaudio-rs
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
