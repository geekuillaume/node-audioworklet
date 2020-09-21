const {AudioWorkletProcessor} = require('../../src');

const currentPitch = 440;
const sampleRate = 48000;
let processPosition = 0;

class WhiteNoiseProcessorWithMessage extends AudioWorkletProcessor {
  constructor() {
    super();
    setInterval(() => {
      this.port.postMessage({
        processPosition,
      });
    })
  }

  process(outputChannels, position) {
    const radPerSecond = Math.PI * 2 * currentPitch;

    for (let sample = 0; sample < outputChannels[0].length; sample++) {
      const sinSample = Math.sin(radPerSecond * (processPosition / sampleRate));
      outputChannels[0][sample] = sinSample;
      outputChannels[1][sample] = sinSample;
      processPosition += 1;
    }
    return true;
  }
}

module.exports = WhiteNoiseProcessorWithMessage;
