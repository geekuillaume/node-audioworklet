if (process.browser) {
	exports = module.exports = {};
} else {
	const {Soundio, SoundioOutstream} = require("bindings")("audioworklet");
	const {attachProcessFunctionFromWorker, AudioWorkletProcessor} = require('./worker');

	SoundioOutstream.prototype.attachProcessFunctionFromWorker = attachProcessFunctionFromWorker;

	exports = module.exports = {
		Soundio,
		SoundioOutstream,
		AudioWorkletProcessor,
	};
}
