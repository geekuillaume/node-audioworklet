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
