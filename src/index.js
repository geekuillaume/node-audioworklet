if (process.browser) {
	exports = module.exports = {};
} else {
	const {Soundio, SoundioOutstream, SoundioInstream} = require("bindings")("audioworklet");
	const {attachProcessFunctionFromWorker, AudioWorkletProcessor} = require('./worker');

	Soundio.prototype.refreshDevices = function () {
		return new Promise((r) => this.refreshDevicesCb(r));
	}

	SoundioOutstream.prototype.attachProcessFunctionFromWorker = attachProcessFunctionFromWorker('outstream');
	SoundioInstream.prototype.attachProcessFunctionFromWorker = attachProcessFunctionFromWorker('instream');

	exports = module.exports = {
		Soundio,
		SoundioOutstream,
		AudioWorkletProcessor,
	};
}
