const DEFAULT_READ_INTERVAL = 100;
const DEFAULT_WRITE_INTERVAL = 100;

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

	AudioStream.prototype.registerWriteHandler = function(handler, options = {}) {
		const format = this.getFormat();
		const rate = this.getRate();
		const TypedArrayBuilder = format === AudioServer.S16LE || format === AudioServer.S16BE ? Int16Array : Float32Array;
		const channels = this.getChannels();

		const targetBufferSize = options.targetBufferSize || rate * 0.5; // 500 ms
		const buffer = new TypedArrayBuilder((options.iterationBufferSize || targetBufferSize / 10) * channels);

		const interval = setInterval(() => {
			if (!this.isStarted()) {
				clearInterval(interval);
			}
			while (this.getBufferSize() < targetBufferSize) {
				const result = handler(buffer);
				if (!result && result !== undefined) {
					this.stop();
					return;
				}
				this.pushAudioChunk(undefined, buffer);
			}
		}, options.interval || DEFAULT_WRITE_INTERVAL);
	}

	exports = module.exports = {
		AudioServer,
		AudioStream,
	};
}
