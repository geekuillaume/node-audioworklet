#include "soundio.h"

#include <cmath>
#include <future>
#include <iostream>
#include <bits/stdc++.h>
#include <sys/time.h>

#include "soundio_converter.h"

void write_callback(struct SoundIoOutStream *outstream, int frame_count_min, int frame_count_max)
{
	SoundioWrap *wrap = (SoundioWrap *)outstream->userdata;
	SoundIoFormat format = outstream->format;

	int frames_left = frame_count_max;
	size_t jsChannelBufferSize = wrap->_outstreamFrameSize * outstream->bytes_per_sample;
	std::vector<uint8_t *> jsBuffer = wrap->_outstreamJsBuffer;

	struct SoundIoChannelArea *areas;
	double outLatency;
	int err;
	// wrap->_processfnMutex.lock();

	// std::cout << frame_count_min << " -> " << frame_count_max << '\n';
	while (frames_left >= wrap->_outstreamFrameSize)
	{
		std::promise<bool> processPromise;
		std::future<bool> processFuture = processPromise.get_future();

		for(void *channelBuffer : jsBuffer) {
			memset(channelBuffer, 0, jsChannelBufferSize);
		}

		int frame_count = wrap->_outstreamFrameSize;
		if ((err = soundio_outstream_begin_write(outstream, &areas, &frame_count))) {
			if (err != SoundIoErrorUnderflow) {
				// todo call an error callback here
				break;
			}
		}
		if (!frame_count)
			break;

		if (wrap->_processFramefn) {
			auto processStatus = wrap->_processFramefn.BlockingCall([format, jsBuffer, frame_count, &processPromise](Napi::Env env, Napi::Function callback) {
				size_t bytes_per_sample = soundio_get_bytes_per_sample(format);
				int bufferSize = frame_count * bytes_per_sample;

				Napi::Array outputs = Napi::Array::New(env);

				outputs = Napi::Array::New(env, jsBuffer.size());
				for (size_t channel = 0; channel < jsBuffer.size(); channel++)
				{
					Napi::ArrayBuffer buffer = Napi::ArrayBuffer::New(env, jsBuffer[channel], bufferSize);

					if (format == SoundIoFormatS8) {
						outputs[channel] = Napi::TypedArrayOf<int8_t>::New(env, bufferSize / bytes_per_sample, buffer, 0);
					} else if (format == SoundIoFormatU8) {
						outputs[channel] = Napi::TypedArrayOf<uint8_t>::New(env, bufferSize / bytes_per_sample, buffer, 0);
					} else if (format == SoundIoFormatS16LE || format == SoundIoFormatS16LE) {
						outputs[channel] = Napi::TypedArrayOf<int16_t>::New(env, bufferSize / bytes_per_sample, buffer, 0);
					} else if (format == SoundIoFormatU16LE || format == SoundIoFormatU16BE) {
						outputs[channel] = Napi::TypedArrayOf<uint16_t>::New(env, bufferSize / bytes_per_sample, buffer, 0);
					} else if (format == SoundIoFormatS32LE || format == SoundIoFormatS32BE) {
						outputs[channel] = Napi::TypedArrayOf<int32_t>::New(env, bufferSize / bytes_per_sample, buffer, 0);
					} else if (format == SoundIoFormatU32LE || format == SoundIoFormatU32BE) {
						outputs[channel] = Napi::TypedArrayOf<uint32_t>::New(env, bufferSize / bytes_per_sample, buffer, 0);
					} else if (format == SoundIoFormatFloat32LE || format == SoundIoFormatFloat32BE) {
						outputs[channel] = Napi::TypedArrayOf<float>::New(env, bufferSize / bytes_per_sample, buffer, 0);
					} else if (format == SoundIoFormatFloat64LE || format == SoundIoFormatFloat64BE) {
						outputs[channel] = Napi::TypedArrayOf<double>::New(env, bufferSize / bytes_per_sample, buffer, 0);
					}
				}

				Napi::Value retValue = callback.Call({
					outputs,
				});

				if (!retValue.IsBoolean()) {
					throw Napi::Error::New(env, "Return value of process function should be a boolean");
				}
				processPromise.set_value(retValue.As<Napi::Boolean>().Value());
			});

			if ( processStatus != napi_ok )
			{
				break;
			} else {
				// struct timespec start, end;
				// clock_gettime(CLOCK_MONOTONIC, &start);
				processFuture.wait();
				// clock_gettime(CLOCK_MONOTONIC, &end);

				// double time_taken;
				// time_taken = (end.tv_sec - start.tv_sec) * 1e9;
				// time_taken = (time_taken + (end.tv_nsec - start.tv_nsec)) * 1e-9;
				// std::cout << "Time taken by process function is : " << std::fixed
				// 		<< time_taken << std::setprecision(9);
				// std::cout << " sec" << std::endl;
			}

			for (int channel = 0; channel < outstream->layout.channel_count; channel += 1) {
				for (int frame = 0; frame < frame_count; frame++) {
					memcpy(areas[channel].ptr + (areas[channel].step * frame), // destination: it can be interleaved or not so we need to take care of one frame after another
						jsBuffer[channel] + (frame * outstream -> bytes_per_sample), // source
						outstream->bytes_per_sample);
				}
			}

			if ((err = soundio_outstream_end_write(outstream))) {
				// todo call an error callback here
				break;
			}
			if (processFuture.get() == false) {
				soundio_outstream_pause(outstream, true);
			}

			frames_left -= frame_count;

			if ((err = soundio_outstream_get_latency(outstream, &outLatency))) {
				// todo call an error callback here
				break;
			}

			if (outLatency > wrap->_configuredOutputBufferDuration) {
				break;
			}
		}
	}
}

Napi::Object SoundioWrap::Init(Napi::Env env, Napi::Object exports)
{
	// Define the class and get it's ctor function
	Napi::Function ctor_func = DefineClass(env, "Soundio", {
			InstanceMethod("openOutputStream", &SoundioWrap::openOutputStream),
			InstanceMethod("closeOutputStream", &SoundioWrap::closeOutputStream),
		 	InstanceMethod("isOutputStreamOpen", &SoundioWrap::isOutputStreamOpen),

			InstanceMethod("startOutputStream", &SoundioWrap::startOutputStream),
			InstanceMethod("setOutputPause", &SoundioWrap::setOutputPause),
			InstanceMethod("clearOutputBuffer", &SoundioWrap::clearOutputBuffer),

			InstanceMethod("getApi", &SoundioWrap::getApi),
			InstanceMethod("getStreamLatency", &SoundioWrap::getStreamLatency),
		//  InstanceMethod("getStreamSampleRate", &SoundioWrap::getStreamSampleRate),
			InstanceMethod("getDevices", &SoundioWrap::getDevices),
			InstanceMethod("getDefaultInputDeviceIndex", &SoundioWrap::getDefaultInputDeviceIndex),
			InstanceMethod("getDefaultOutputDeviceIndex", &SoundioWrap::getDefaultOutputDeviceIndex),
			InstanceMethod("setProcessFunction", &SoundioWrap::setProcessFunction),
			InstanceMethod("setOutputVolume", &SoundioWrap::setOutputVolume),

			InstanceMethod("_getExternal", &SoundioWrap::_getExternal),
			StaticMethod("_setProcessFunctionFromExternal", &SoundioWrap::_setProcessFunctionFromExternal),

			StaticValue("SoundIoFormatS8", Napi::Number::New(env, SoundIoFormatS8)),        ///< Signed 8 bit
			StaticValue("SoundIoFormatU8", Napi::Number::New(env, SoundIoFormatU8)),        ///< Unsigned 8 bit
			StaticValue("SoundIoFormatS16LE", Napi::Number::New(env, SoundIoFormatS16LE)),     ///< Signed 16 bit Little Endian
			StaticValue("SoundIoFormatS16BE", Napi::Number::New(env, SoundIoFormatS16BE)),     ///< Signed 16 bit Big Endian
			StaticValue("SoundIoFormatU16LE", Napi::Number::New(env, SoundIoFormatU16LE)),     ///< Unsigned 16 bit Little Endian
			StaticValue("SoundIoFormatU16BE", Napi::Number::New(env, SoundIoFormatU16BE)),     ///< Unsigned 16 bit Big Endian
			StaticValue("SoundIoFormatS32LE", Napi::Number::New(env, SoundIoFormatS32LE)),     ///< Signed 32 bit Little Endian
			StaticValue("SoundIoFormatS32BE", Napi::Number::New(env, SoundIoFormatS32BE)),     ///< Signed 32 bit Big Endian
			StaticValue("SoundIoFormatU32LE", Napi::Number::New(env, SoundIoFormatU32LE)),     ///< Unsigned 32 bit Little Endian
			StaticValue("SoundIoFormatU32BE", Napi::Number::New(env, SoundIoFormatU32BE)),     ///< Unsigned 32 bit Big Endian
			StaticValue("SoundIoFormatFloat32LE", Napi::Number::New(env, SoundIoFormatFloat32LE)), ///< Float 32 bit Little Endian, Range -1.0 to 1.0
			StaticValue("SoundIoFormatFloat32BE", Napi::Number::New(env, SoundIoFormatFloat32BE)), ///< Float 32 bit Big Endian, Range -1.0 to 1.0
			StaticValue("SoundIoFormatFloat64LE", Napi::Number::New(env, SoundIoFormatFloat64LE)), ///< Float 64 bit Little Endian, Range -1.0 to 1.0
			StaticValue("SoundIoFormatFloat64BE", Napi::Number::New(env, SoundIoFormatFloat64BE)), ///< Float 64 bit Big Endian, Range -1.0 to 1.0
	});

	// Set the class's ctor function as a persistent object to keep it in memory
	constructor = Napi::Persistent(ctor_func);
	constructor.SuppressDestruct();

	// Export the ctor
	exports.Set("Soundio", ctor_func);
	return exports;
}

SoundioWrap::SoundioWrap(
	const Napi::CallbackInfo &info
) :
	Napi::ObjectWrap<SoundioWrap>(info),
	_outstream(nullptr),
	_processFramefn(nullptr),
	_outstreamFrameSize(480),
	_configuredOutputBufferDuration(0.1)
{
	_ownRef = Napi::Reference<Napi::Value>::New(info.This()); // this is used to prevent the GC to collect this object while a stream is running
	int err;

	_soundio = soundio_create();
	if (!_soundio) {
		throw Napi::Error::New(info.Env(), "Error while creating soundio instance");
	}

	err = soundio_connect(_soundio);
	if (err) {
		throw Napi::Error::New(info.Env(), soundio_strerror(err));
	}
	soundio_flush_events(_soundio);
}

SoundioWrap::~SoundioWrap()
{
	if (_outstream) {
		soundio_outstream_destroy(_outstream);
	}
	soundio_destroy(_soundio);
}

Napi::Value SoundioWrap::getDevices(const Napi::CallbackInfo &info)
{
	Napi::Array outputDevicesArray;
	Napi::Array inputDevicesArray;
	std::vector<SoundIoDevice *> outputDevices;
	std::vector<SoundIoDevice *> inputDevices;

	unsigned int outputDeviceCount = soundio_output_device_count(_soundio);
	unsigned int inputDeviceCount = soundio_input_device_count(_soundio);

	for (int i = 0; i < outputDeviceCount; i += 1) {
		struct SoundIoDevice *device = soundio_get_output_device(_soundio, i);
		if (!device->probe_error) {
			outputDevices.push_back(device);
		}
	}
	for (int i = 0; i < inputDeviceCount; i += 1) {
		struct SoundIoDevice *device = soundio_get_input_device(_soundio, i);
		if (!device->probe_error) {
			inputDevices.push_back(device);
		}
	}

	// Allocate the devices array
	outputDevicesArray = Napi::Array::New(info.Env(), outputDevices.size());
	inputDevicesArray = Napi::Array::New(info.Env(), inputDevices.size());

	// Convert the devices to objects
	for (unsigned int i = 0; i < outputDevices.size(); i++)
	{
		outputDevicesArray[i] = SoundioConverter::ConvertDeviceInfo(info.Env(), outputDevices[i]);
		soundio_device_unref(outputDevices[i]);
	}
	for (unsigned int i = 0; i < inputDevices.size(); i++)
	{
		inputDevicesArray[i] = SoundioConverter::ConvertDeviceInfo(info.Env(), inputDevices[i]);
		soundio_device_unref(inputDevices[i]);
	}

	Napi::Object devices = Napi::Object::New(info.Env());
	devices.Set("outputDevices", outputDevicesArray);
	devices.Set("inputDevices", inputDevicesArray);

	return devices;
}

Napi::Value SoundioWrap::getDefaultInputDeviceIndex(const Napi::CallbackInfo &info)
{
	return Napi::Number::New(info.Env(), soundio_default_input_device_index(_soundio));
}

Napi::Value SoundioWrap::getDefaultOutputDeviceIndex(const Napi::CallbackInfo &info)
{
	return Napi::Number::New(info.Env(), soundio_default_output_device_index(_soundio));
}

void SoundioWrap::openOutputStream(const Napi::CallbackInfo &info)
{
	Napi::Object opts = info[0].IsUndefined() ? Napi::Object::New(info.Env()) : info[0].As<Napi::Object>();

	if (_outstream != nullptr) {
		throw Napi::Error::New(info.Env(), "An outstream is already started");
	}

	int err;
	unsigned int deviceId = opts.Get("deviceId").IsNull() || opts.Get("deviceId").IsUndefined() ? soundio_default_output_device_index(_soundio) : opts.Get("deviceId").As<Napi::Number>();

	if (deviceId >= soundio_output_device_count(_soundio)) {
		throw Napi::Error::New(info.Env(), "output device not found");
	}

	SoundIoDevice *device = soundio_get_output_device(_soundio, deviceId);
	SoundIoOutStream *outstream = soundio_outstream_create(device);

	if (!opts.Get("format").IsNull() && !opts.Get("format").IsUndefined()) {
		outstream->format = (SoundIoFormat)opts.Get("format").As<Napi::Number>().Int32Value();
		if (!soundio_device_supports_format(device, outstream->format)) {
			throw Napi::Error::New(info.Env(), "format not supported");
		}
	}
	if (!opts.Get("sampleRate").IsNull() && !opts.Get("sampleRate").IsUndefined()) {
		outstream->sample_rate = opts.Get("sampleRate").As<Napi::Number>();
		if (!soundio_device_supports_sample_rate(device, outstream->sample_rate)) {
			throw Napi::Error::New(info.Env(), "sample rate not supported");
		}
	}
	if (!opts.Get("name").IsNull() && !opts.Get("name").IsUndefined()) {
		outstream->name = opts.Get("name").As<Napi::String>().Utf8Value().c_str();
	}
	outstream->userdata = this;
	outstream->write_callback = write_callback;

	if (!opts.Get("process").IsNull() && !opts.Get("process").IsUndefined()) {
		_setProcessFunction(info.Env(), opts.Get("process").As<Napi::Function>());
	}
	if (!opts.Get("frameSize").IsNull() && !opts.Get("frameSize").IsUndefined()) {
		_outstreamFrameSize = opts.Get("frameSize").As<Napi::Number>();
	}
	if (!opts.Get("bufferDuration").IsNull() && !opts.Get("bufferDuration").IsUndefined()) {
		_configuredOutputBufferDuration = opts.Get("bufferDuration").As<Napi::Number>().DoubleValue();
	}

	if ((err = soundio_outstream_open(outstream))) {
			soundio_outstream_destroy(outstream);
			soundio_device_unref(device);
			throw Napi::Error::New(info.Env(), soundio_strerror(err));
	}

	for (size_t i = 0; i < outstream->layout.channel_count; i++)
	{
		_outstreamJsBuffer.push_back(new uint8_t[_outstreamFrameSize * outstream->bytes_per_sample]);
	}

	_outstream = outstream;
	soundio_device_unref(device);
	_ownRef.Ref();
}

void SoundioWrap::closeOutputStream(const Napi::CallbackInfo &info)
{
	if (_outstream == nullptr) {
		throw Napi::Error::New(info.Env(), "The outstream is already stopped");
	}

	soundio_outstream_destroy(_outstream);
	_outstream = nullptr;
	if (_processFramefn) {
		_processFramefn.Release();
		_processFramefn = nullptr;
	}
	_ownRef.Unref();
}

void SoundioWrap::clearOutputBuffer(const Napi::CallbackInfo &info)
{
	if (_outstream == nullptr) {
		throw Napi::Error::New(info.Env(), "The outstream is not started");
	}

	soundio_outstream_clear_buffer(_outstream);
}


Napi::Value SoundioWrap::isOutputStreamOpen(const Napi::CallbackInfo &info)
{
	return Napi::Boolean::New(info.Env(), _outstream != nullptr);
}

void SoundioWrap::startOutputStream(const Napi::CallbackInfo &info)
{
	if (_outstream == nullptr) {
		throw Napi::Error::New(info.Env(), "Outstream closed");
	}

	int err = soundio_outstream_start(_outstream);
	if (err) {
		throw Napi::Error::New(info.Env(), soundio_strerror(err));
	}
}

void SoundioWrap::setOutputPause(const Napi::CallbackInfo &info)
{
	if (_outstream == nullptr) {
		throw Napi::Error::New(info.Env(), "Outstream closed");
	}

	int err = soundio_outstream_pause(_outstream, info[0].IsUndefined() || info[0].IsNull() ? true : info[0].As<Napi::Boolean>());
	if (err) {
		throw Napi::Error::New(info.Env(), soundio_strerror(err));
	}
}

void SoundioWrap::setOutputVolume(const Napi::CallbackInfo &info)
{
	if (_outstream == nullptr) {
		throw Napi::Error::New(info.Env(), "Outstream closed");
	}
	if (info[0].IsNull() || info[0].IsUndefined()) {
		throw Napi::Error::New(info.Env(), "First argument should be the volume");
	}
	double volume = info[0].As<Napi::Number>().DoubleValue();

	if (volume < 0 || volume > 1) {
		throw Napi::Error::New(info.Env(), "volume should be between 0 and 1");
	}
	soundio_outstream_set_volume(_outstream, volume);
}

Napi::Value SoundioWrap::getApi(const Napi::CallbackInfo &info)
{
	return Napi::String::New(info.Env(), soundio_backend_name(_soundio->current_backend));
}

Napi::Value SoundioWrap::getStreamLatency(const Napi::CallbackInfo &info)
{
	try
	{
		return Napi::Number::New(info.Env(), _outstream->software_latency);
	}
	catch (std::exception &ex)
	{
		throw Napi::Error::New(info.Env(), ex.what());
	}
}

void SoundioWrap::_setProcessFunction(Napi::Env env, Napi::Function processFn)
{
	_processfnMutex.lock();

	_processFramefn = Napi::ThreadSafeFunction::New(env, processFn, "processFrameCallback", 0, 1, [this](Napi::Env) {
		// prevent a segfault on exit when the write thread want to access a non-existing threadsafefunction
		_processFramefn = nullptr;
	});

	_processfnMutex.unlock();
}


void SoundioWrap::setProcessFunction(const Napi::CallbackInfo& info)
{
	if (info[0].IsEmpty() || !info[0].IsFunction()) {
		throw Napi::Error::New(info.Env(), "First argument should be the process function");
	}
	_setProcessFunction(info.Env(), info[0].As<Napi::Function>());
}

Napi::Value SoundioWrap::_getExternal(const Napi::CallbackInfo& info)
{
	// we cannot pass an external between multiple nodejs thread so we use an array buffer to pass the pointer to thr SoundioWrap instance
	// this is a big hack but I haven't found any other way of doing that
	// if thread are being created / deleted it could also lead to a segfault so be careful to call _setProcessFunctionFromExternal directly after calling _getExternal
	auto wrappedPointer = Napi::ArrayBuffer::New(info.Env(), sizeof(this));
	SoundioWrap** dataAddr = reinterpret_cast<SoundioWrap **>(wrappedPointer.Data());
	*dataAddr = this;
	return wrappedPointer;
}

void SoundioWrap::_setProcessFunctionFromExternal(const Napi::CallbackInfo& info)
{
	if (info.Length() != 2 || !info[0].IsArrayBuffer() || !info[1].IsFunction()) {
		throw Napi::Error::New(info.Env(), "Two arguments should be passed: External Rtaudio instance and process function");
	}

	SoundioWrap *wrap = *reinterpret_cast<SoundioWrap **>(info[0].As<Napi::ArrayBuffer>().Data());

	wrap->_setProcessFunction(info.Env(), info[1].As<Napi::Function>());
}
