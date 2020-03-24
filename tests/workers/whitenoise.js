const {AudioWorkletProcessor} = require('../../');

class WhiteNoiseProcessor extends AudioWorkletProcessor {
  constructor() {
    super();
  }

  process(outputBuffer) {
    for (let channel = 0; channel < outputBuffer.length; channel++) {
      const channelBuffer = new Int16Array(outputBuffer[channel].buffer);
      for (let sample = 0; sample < channelBuffer.length; sample++) {
        channelBuffer[sample] = Math.floor(((Math.random() * 2) - 1) * (1<<15));
      }
    }

    return true;
  }
}

module.exports = WhiteNoiseProcessor;
