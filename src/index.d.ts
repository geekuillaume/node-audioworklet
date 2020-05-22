/// <reference types="node" />

import { Worker } from 'worker_threads';

declare interface SoundioDevicesResponse {
	outputDevices: SoundioDevice[];
	inputDevices: SoundioDevice[];
}

declare const enum SoundioAudioFormat {
	SoundIoFormatS8,
	SoundIoFormatU8,
	SoundIoFormatS16LE,
	SoundIoFormatS16BE,
	SoundIoFormatU16LE,
	SoundIoFormatU16BE,
	SoundIoFormatS32LE,
	SoundIoFormatS32BE,
	SoundIoFormatU32LE,
	SoundIoFormatU32BE,
	SoundIoFormatFloat32LE,
	SoundIoFormatFloat32BE,
	SoundIoFormatFloat64LE,
	SoundIoFormatFloat64BE,
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
	format?: SoundioAudioFormat;

	/** number of samples per second per channel */
	sampleRate?: number;

	/** number of samples per channel to pass to the process function, if not set, will be dynamicly set on each frame to minimize the number of calls to the process function */
	frameSize?: number;

	name?: string;
	process?: ((inputOrOutputChannels: TypedArray[]) => boolean);
	bufferDuration?: number;
}

export declare class Soundio {

	static SoundIoFormatS8: SoundioAudioFormat;
	static SoundIoFormatU8: SoundioAudioFormat;
	static SoundIoFormatS16LE: SoundioAudioFormat;
	static SoundIoFormatS16BE: SoundioAudioFormat;
	static SoundIoFormatU16LE: SoundioAudioFormat;
	static SoundIoFormatU16BE: SoundioAudioFormat;
	static SoundIoFormatS24LE: SoundioAudioFormat;
	static SoundIoFormatS24BE: SoundioAudioFormat;
	static SoundIoFormatU24LE: SoundioAudioFormat;
	static SoundIoFormatU24BE: SoundioAudioFormat;
	static SoundIoFormatS32LE: SoundioAudioFormat;
	static SoundIoFormatS32BE: SoundioAudioFormat;
	static SoundIoFormatU32LE: SoundioAudioFormat;
	static SoundIoFormatU32BE: SoundioAudioFormat;
	static SoundIoFormatFloat32LE: SoundioAudioFormat;
	static SoundIoFormatFloat32BE: SoundioAudioFormat;
	static SoundIoFormatFloat64LE: SoundioAudioFormat;
	static SoundIoFormatFloat64BE: SoundioAudioFormat;

	getApi(): string;
	getDevices(): SoundioDevicesResponse;
	getDefaultInputDevice(): SoundioDevice;
	getDefaultOutputDevice(): SoundioDevice;

	refreshDevices(): Promise<void>;
}

export declare class SoundioDevice {
	name: string;
	id: string;
	isInput: boolean;
	isOutput: boolean;
	formats: SoundioAudioFormat[];
	sampleRates: {
		min: number;
		max: number;
	}[];
	channelLayouts: {
		name: string;
		channelCount: number;
	}[];

	openOutputStream(params?: StreamParams): SoundioOutputStream;
	openInputStream(params?: StreamParams): SoundioInputStream;
}

export declare class SoundioOutputStream {
	start(): void;

	close(): void;
	isOpen(): boolean;

	setPause(paused: boolean): void;
	getLatency(): number;
	setVolume(volume: number): void;
	setProcessFunction(process: ((outputChannels: Buffer[]) => boolean)): void;
	clearBuffer(): void;

	/**
	 * Start the module specified by scriptPath in a new worker thread and use it as the process function
	 * The module should export a class extending AudioWorkletProcessor with the process method implemented
	 */
	attachProcessFunctionFromWorker(scriptPath: string): Worker;
}


export declare class SoundioInputStream {
	start(): void;

	close(): void;
	isOpen(): boolean;

	setPause(paused: boolean): void;
	getLatency(): number;
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
	abstract process(inputOrOutputChannels: TypedArray[]): boolean;
}
