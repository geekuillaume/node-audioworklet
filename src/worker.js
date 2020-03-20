const { Worker, parentPort } = require('worker_threads');
const path = require('path');

exports.attachProcessFunctionFromWorker = function (workerPath) {
  const rtaudioWrapExternal = this._getExternal();

  const worker = new Worker(path.resolve(__dirname, './worker_init.js'), {
    workerData: {
      scriptPath: workerPath,
      rtAudioPtr: rtaudioWrapExternal,
    },
  });

  return worker;
}

class AudioWorkletProcessor {
  constructor() {
    this.port = parentPort;
  }
}

exports.AudioWorkletProcessor = AudioWorkletProcessor;
