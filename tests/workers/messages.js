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
