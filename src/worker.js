const { Worker, parentPort } = require('worker_threads');
const path = require('path');

exports.attachProcessFunctionFromWorker = function (workerPath) {
  const streamPtr = this._getExternal();

  const worker = new Worker(path.resolve(__dirname, './worker_init.js'), {
    workerData: {
      scriptPath: workerPath,
      streamPtr,
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
