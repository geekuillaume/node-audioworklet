// this is executed from the worker.js file and used to initiate the context for the audioworklet worker
const {
  workerData,
} = require('worker_threads');
let audioworklet;
try {
  audioworklet = require('audioworklet');
} catch (e) {
  audioworklet = require('../');
}

const scriptPath = workerData.scriptPath;
const streamPtr = workerData.streamPtr;

const workerModule = require(scriptPath);
let WorkerClass;
if (workerModule.default) {
  WorkerClass = workerModule.default;
} else {
  WorkerClass = workerModule;
}

const worker = new WorkerClass();
audioworklet.AudioStream._setProcessFunctionFromExternal(streamPtr, worker.process.bind(worker));



