// this is executed from the worker.js file and used to initiate the context for the audioworklet worker

const {
  workerData,
} = require('worker_threads');
const {RtAudio} = require('../');

const scriptPath = workerData.scriptPath;
const rtAudioPtr = workerData.rtAudioPtr;

const WorkerClass = require(scriptPath);

const worker = new WorkerClass();
RtAudio._setProcessFunctionFromExternal(rtAudioPtr, worker.process.bind(worker));



