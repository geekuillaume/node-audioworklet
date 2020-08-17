const {AudioWorkletProcessor} = require('../../');

class WhiteNoiseProcessor extends AudioWorkletProcessor {
  constructor() {
    super();
    setInterval(() => {
      console.log('Latency from worker:', this.getLatency());
    }, 1000);
  }

  process(outputChannels) {
    for (let sample = 0; sample < outputChannels[0].length; sample++) {
      outputChannels[0][sample] = Math.random();
      outputChannels[1][sample] = Math.random();
    }
    return true;
  }
}

module.exports = WhiteNoiseProcessor;
