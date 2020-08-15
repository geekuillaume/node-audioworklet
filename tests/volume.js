const { AudioServer } = require('../');
const audioServer = new AudioServer();

let streamStatus = true;

let currentSample = 0;
const pitch = 440;

const processFrame = (outputChannels) => {
  const radPerSecond = Math.PI * 2 * pitch;

  for (let sample = 0; sample < outputChannels[0].length; sample++) {
    const sinSample = Math.sin(radPerSecond * (currentSample / 48000));
    outputChannels[0][sample] = sinSample;
    outputChannels[1][sample] = sinSample;
    currentSample += 1;
  }

  return streamStatus;
}

const main = async () => {
  const device = audioServer.getDefaultOutputDevice();
  console.log('Opening stream');
  const outputStream = audioServer.initOutputStream(device.id, {
    format: AudioServer.F32LE,
    process: processFrame,
  });

  console.log('Starting stream');
  outputStream.start();

  const triggerVolumeDown = () => {
    console.log('Volume down');
    outputStream.setVolume(0.5);
    setTimeout(() => {
      console.log('Volume up');
      outputStream.setVolume(1);
      setTimeout(triggerVolumeDown, 1000);
    }, 1000);
  }

  setTimeout(triggerVolumeDown, 500);

  setTimeout(() => {
    console.log('Stopping stream');
    streamStatus = false;
    setTimeout(() => {
      process.exit(0);
    }, 1000);
  }, 10000);
}
main();
