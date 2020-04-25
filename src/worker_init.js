// this is executed from the worker.js file and used to initiate the context for the audioworklet worker

const {
  workerData,
} = require('worker_threads');
let SoundioOutstream;
try {
  SoundioOutstream = require('audioworklet').SoundioOutstream;
} catch (e) {
  SoundioOutstream = require('../').SoundioOutstream;
}

const scriptPath = workerData.scriptPath;
const outstreamPtr = workerData.outstreamPtr;

const workerModule = require(scriptPath);
let WorkerClass;
if (workerModule.default) {
  WorkerClass = workerModule.default;
} else {
  WorkerClass = workerModule;
}

const worker = new WorkerClass();
SoundioOutstream._setProcessFunctionFromExternal(outstreamPtr, worker.process.bind(worker));



