if (process.browser) {
	exports = module.exports = {};
} else {
	const {Soundio} = require("bindings")("audioworklet");
	const {attachProcessFunctionFromWorker, AudioWorkletProcessor} = require('./worker');

	Soundio.prototype.attachProcessFunctionFromWorker = attachProcessFunctionFromWorker;

	exports = module.exports = {
		Soundio,
		AudioWorkletProcessor,
	};
}
