// this is executed from the worker.js file and used to initiate the context for the audioworklet worker

const {
  workerData,
} = require('worker_threads');
const {Soundio} = require('../');

const scriptPath = workerData.scriptPath;
const rtAudioPtr = workerData.rtAudioPtr;

const workerModule = require(scriptPath);
let WorkerClass;
if (workerModule.default) {
  WorkerClass = workerModule.default;
} else {
  WorkerClass = workerModule;
}

const worker = new WorkerClass();
Soundio._setProcessFunctionFromExternal(rtAudioPtr, worker.process.bind(worker));



