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

  process(outputBuffer) {
    if (this.mute) {
      return true;
    }
    for (let channel = 0; channel < outputBuffer.length; channel++) {
      const channelBuffer = new Int16Array(outputBuffer[channel].buffer);
      for (let sample = 0; sample < channelBuffer.length; sample++) {
        channelBuffer[sample] = Math.floor(((Math.random() * 2) - 1) * (1<<15));
      }
    }

    return true;
  }
}

module.exports = WhiteNoiseProcessorWithMessage;
