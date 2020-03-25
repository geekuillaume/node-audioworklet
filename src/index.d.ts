/// <reference types="node" />

import { Worker } from 'worker_threads';

// /** Audio API specifier arguments. */
declare const enum SoundioApi {
	/** Search for a working compiled API. */
	UNSPECIFIED,

	/** The Advanced Linux Sound Architecture API. */
	LINUX_ALSA,

	/** The Linux PulseAudio API. */
	LINUX_PULSE,

	/** The Linux Open Sound System API. */
	LINUX_OSS,

	/** The Jack Low-Latency Audio Server API. */
	UNIX_JACK,

	/** Macintosh OS-X Core Audio API. */
	MACOSX_CORE,

	/** The Microsoft WASAPI API. */
	WINDOWS_WASAPI,

	/** The Steinberg Audio Stream I/O API. */
	WINDOWS_ASIO,

	/** The Microsoft DirectSound API. */
	WINDOWS_DS,

	/** A compilable but non-functional API. */
	RTAUDIO_DUMMY
}

/** The public device information structure for returning queried values. */
declare interface SoundioDeviceInfo {
	/** Character string device identifier. */
	name: string;

	// /** Maximum output channels supported by device. */
	// outputChannels: number;

	// /** Maximum input channels supported by device. */
	// inputChannels: number;

	// /** Maximum simultaneous input/output channels supported by device. */
	// duplexChannels: number;

	// /** Is the device the default output device */
	// isDefaultOutput: number;

	// /** Is the device the default input device */
	// isDefaultInput: number;

	// /** Supported sample rates (queried from list of standard rates). */
	// sampleRates: Array<number>;

	// /** Preferred sample rate, e.g. for WASAPI the system sample rate. */
	// preferredSampleRate: number;

	// /** Bit mask of supported data formats. */
	// nativeFormats: number;
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

interface OutputStreamParams {
	format?: SoundioAudioFormat;
	sampleRate?: number;
	frameSize?: number;
	name?: string;
	deviceId?: number;
	process?: ((outputChannels: TypedArray[]) => boolean);
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

	/**
	 * Create an Soundio instance.
	 * @param api The audio API to use. (Default will be automatically selected)
	 */
	constructor(api?: SoundioApi);

	openOutputStream(params?: OutputStreamParams): void;
	closeOutputStream(): void;
	isOutputStreamOpen(): boolean;

	startOutputStream(): void;
	setOutputPause(paused: bool): void;

	getApi(): string;

	getStreamLatency(): number;

	/**
	 * Returns the list of available devices.
	 */
	getDevices(): Array<RtAudioDeviceInfo>;

	getDefaultInputDeviceIndex(): number;
	getDefaultOutputDeviceIndex(): number;

	setProcessFunction(process: ((outputChannels: Buffer[]) => boolean)): void;
	clearOutputBuffer();
	setOutputVolume(volume: number): void;

	/**
	 * Start the module specified by scriptPath in a new worker thread and use it as the process function
	 * The module should export a class extending AudioWorkletProcessor with the process method implemented
	 */
	attachProcessFunctionFromWorker(scriptPath: string): Worker;
}

export declare abstract class AudioWorkletProcessor {
	constructor();

	port: MessagePort;
	abstract process(inputData: Buffer[], outputData: Buffer[]): boolean;
}
