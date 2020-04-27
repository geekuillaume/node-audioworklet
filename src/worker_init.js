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
const streamType = workerData.streamType;

const workerModule = require(scriptPath);
let WorkerClass;
if (workerModule.default) {
  WorkerClass = workerModule.default;
} else {
  WorkerClass = workerModule;
}

const worker = new WorkerClass();
if (streamType === 'instream') {
  audioworklet.SoundioInstream._setProcessFunctionFromExternal(streamPtr, worker.process.bind(worker));
} else if (streamType === 'outstream') {
  audioworklet.SoundioOutstream._setProcessFunctionFromExternal(streamPtr, worker.process.bind(worker));
} else {
  throw new Error('Invalid stream type');
}



