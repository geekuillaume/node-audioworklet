/// <reference types="node" />

import { Worker } from 'worker_threads';

declare interface AudioDevicesResponse {
	outputDevices: AudioDevice[];
	inputDevices: AudioDevice[];
}

declare const enum AudioFormat {
	S16LE = 0x0010, /**< 16-bit integers, Little Endian. */
	S16BE = 0x0020, /**< 16-bit integers, Big Endian. */
	F32LE = 0x1000, /**< 32-bit floating point, Little Endian. */
	F32BE = 0x2000  /**< 32-bit floating point, Big Endian. */
}

export type TypedArray = Int8Array |
  Uint8Array |
  Int16Array |
  Uint16Array |
  Int32Array |
  Uint32Array |
  Uint8ClampedArray |
  Float32Array |
  Float64Array;

interface StreamParams {
	channels?: number;
	format?: AudioFormat;

	/** number of samples per second per channel */
	sampleRate?: number;

	/** number of samples per channel to pass to the process function, if not set, will be dynamicly set on each frame to minimize the number of calls to the process function */
	frameSize?: number;

	name?: string;
	process?: ((inputOrOutputChannels: TypedArray[]) => boolean);

	/** size of the buffer in frames */
	latencyFrames?: number;
}

export declare class AudioServer {
	constructor({onDeviceChange}?: {onDeviceChange?: () => any});

	static S16LE: AudioFormat;
	static S16BE: AudioFormat;
	static F32LE: AudioFormat;
	static F32BE: AudioFormat;

	getApi(): string;
	getDevices(): AudioDevicesResponse;
	getDefaultOutputDevice(): AudioDevice;
	getDefaultInputDevice(): AudioDevice;
	outputLoopbackSupported(): boolean;
	initInputStream(deviceId: string, params?: StreamParams, outputDeviceLoopback?: boolean): AudioStream;
	initOutputStream(deviceId: string, params?: StreamParams): AudioStream;
}

export declare class AudioDevice {
	name: string;
	id: string;
	minRate: number;
	maxRate: number;
	defaultRate: number;
	maxChannels: number;
	minLatency: number;
	maxLatency: number;
	groupId: string;
	defaultFormat: AudioFormat;
	preferred: {
		multimedia: boolean;
		voice: boolean;
		notification: boolean;
		all: boolean;
	}
}

export declare class AudioStream {
	start(): void;
	stop(): void;

	isStarted(): boolean;

	getLatency(): number;
	setVolume(volume: number): void;
	setProcessFunction(process: ((outputChannels: Buffer[]) => boolean)): void;

	/**
	 * Start the module specified by scriptPath in a new worker thread and use it as the process function
	 * The module should export a class extending AudioWorkletProcessor with the process method implemented
	 */
	attachProcessFunctionFromWorker(scriptPath: string): Worker;
}

export declare abstract class AudioWorkletProcessor {
	constructor();

	port: MessagePort;
	getLatency: () => number;
	abstract process(inputOrOutputChannels: TypedArray[]): boolean;
}
