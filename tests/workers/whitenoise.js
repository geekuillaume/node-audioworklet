const {AudioWorkletProcessor} = require('../../');

class WhiteNoiseProcessor extends AudioWorkletProcessor {
  constructor() {
    super();
  }

  process(inputData, outputData) {
    for (let i = 0; i < outputData[0].length; i++) {
      outputData[0][i] = Math.floor(Math.random() * 16000);
    }

    return true;
  }
}

module.exports = WhiteNoiseProcessor;
