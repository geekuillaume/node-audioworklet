const DEFAULT_READ_INTERVAL = 100;

if (process.browser) {
	exports = module.exports = {};
} else {
	const {AudioServer, AudioStream} = require("bindings")("audioworklet");

	AudioServer.prototype.getDefaultOutputDevice = function() {
		return this.getDevices().outputDevices.find((device) => device.preferred.all);
	}
	AudioServer.prototype.getDefaultInputDevice = function() {
		return this.getDevices().inputDevices.find((device) => device.preferred.all);
	}

	AudioStream.prototype.registerReadHandler = function(handler, options = {}) {
		const format = this.getFormat();
		const TypedArrayBuilder = format === AudioServer.S16LE || format === AudioServer.S16BE ? Int16Array : Float32Array;
		const channels = this.getChannels();

		const interval = setInterval(() => {
			if (!this.isStarted()) {
				clearInterval(interval);
			}
			const availableFrames = this.getBufferSize();
			if (!availableFrames) {
				return;
			}
			const buffer = new TypedArrayBuilder(availableFrames * channels);
			this.readAudioChunk(undefined, buffer);
			handler(buffer);
		}, options.interval || DEFAULT_READ_INTERVAL);
	}

	exports = module.exports = {
		AudioServer,
		AudioStream,
	};
}
