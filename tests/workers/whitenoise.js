const {AudioWorkletProcessor} = require('../../');

class WhiteNoiseProcessor extends AudioWorkletProcessor {
  constructor() {
    super();
  }

  process(outputChannels) {
    for (let sample = 0; sample < outputChannels[0].length; sample++) {
      outputChannels[0][sample] = Math.random();
      outputChannels[1][sample] = Math.random();
    }
    console.log('coucou', outputChannels);

    return true;
  }
}

module.exports = WhiteNoiseProcessor;
