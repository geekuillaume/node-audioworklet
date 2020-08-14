if (process.browser) {
	exports = module.exports = {};
} else {
	const {AudioServer, AudioStream} = require("bindings")("audioworklet");
	const {attachProcessFunctionFromWorker, AudioWorkletProcessor} = require('./worker');

	exports = module.exports = {
		AudioServer,
		AudioStream,
		AudioWorkletProcessor,
	};
}
