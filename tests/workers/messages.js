const {AudioWorkletProcessor} = require('../../');

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

  process(inputData, outputData) {
    if (this.mute) {
      return true;
    }
    for (let i = 0; i < outputData[0].length; i++) {
      outputData[0][i] = Math.floor(Math.random() * 16000);
    }

    return true;
  }
}

module.exports = WhiteNoiseProcessorWithMessage;
