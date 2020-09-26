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

	name?: string;

	/** size of the buffer in frames */
	latencyFrames?: number;

	/** prevent device switch by OS on disconnect */
	disableSwitching?: boolean;

	logProcessTime?: boolean;
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

	static setDebugLog(enabled?: boolean): void;
}

export type AudioDeviceState = "enabled" | "unplugged" | "disabled";

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
	state: AudioDeviceState;
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
	getPosition(): number;
	setVolume(volume: number): void;

	/** Returns the number of pushed frames, could be lower than number of frames in the audio chunk if the buffer is full */
	pushAudioChunk(timestamp?: number, interleavedAudioChunk: TypedArray): number;
	/** Returns the number of read frames, could be lower than the destination size if the buffer is empty */
	readAudioChunk(timestamp?: number, destination: TypedArray): number;

	/** For an input stream returns the number of frames available to read, for an output stream returns the available size to write in frames in the buffer */
	getBufferSize(): number;
	getFormat(): AudioFormat;
	getChannels(): number;

	registerReadHandler(readHandler: (audioChunk: TypedArray) => void, options?: {interval: number}): void;

	configuredLatencyFrames: number;
}

export declare abstract class AudioWorkletProcessor {
	constructor();

	port: MessagePort;
	getLatency: () => number;
	abstract process(inputOrOutputChannels: TypedArray[]): boolean;
}
