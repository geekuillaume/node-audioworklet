if (process.browser) {
	exports = module.exports = {};
} else {
	const {AudioServer, AudioStream} = require("bindings")("audioworklet");
	const {attachProcessFunctionFromWorker, AudioWorkletProcessor} = require('./worker');

	AudioServer.prototype.getDefaultOutputDevice = function() {
		return this.getDevices().outputDevices.find((device) => device.preferred.all);
	}
	AudioServer.prototype.getDefaultInputDevice = function() {
		return this.getDevices().inputDevices.find((device) => device.preferred.all);
	}

	AudioStream.prototype.attachProcessFunctionFromWorker = attachProcessFunctionFromWorker;

	exports = module.exports = {
		AudioServer,
		AudioStream,
		AudioWorkletProcessor,
	};
}
