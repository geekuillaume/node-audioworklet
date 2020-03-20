#include "rt_audio.h"

#include <cmath>
#include <future>

#include "rt_audio_converter.h"

int rt_callback(void *outputBuffer, void *inputBuffer, unsigned int nFrames, double streamTime, RtAudioStreamStatus status, void *userData)
{
	RtAudioWrap *wrap = (RtAudioWrap *)userData;
	size_t bufferSize = wrap->_frameSize * wrap->_outputChannels * wrap->_sampleSize;
	std::promise<int> processPromise;

	// Verify frame size
	if (nFrames != wrap->_frameSize)
	{
		return 0;
	}
	if (wrap->_processFramefn == nullptr) {
		return 0;
	}

	wrap->_processfnMutex.lock();

	memset(outputBuffer, 0, bufferSize);

	auto processStatus = wrap->_processFramefn.NonBlockingCall([outputBuffer, inputBuffer, bufferSize, &processPromise](Napi::Env env, Napi::Function callback) {
		Napi::Array inputs = Napi::Array::New(env);
		Napi::Array outputs = Napi::Array::New(env);

		if (inputBuffer != nullptr) {
			inputs = Napi::Array::New(env, 1);
			const Napi::Buffer<int8_t> buffer = Napi::Buffer<int8_t>::New(env, reinterpret_cast<int8_t *>(inputBuffer), bufferSize);
			inputs[uint32_t(0)] = buffer;
		}
		if (outputBuffer != nullptr) {
			outputs = Napi::Array::New(env, 1);
			const Napi::Buffer<int8_t> buffer = Napi::Buffer<int8_t>::New(env, reinterpret_cast<int8_t *>(outputBuffer), bufferSize);
			outputs[uint32_t(0)] = buffer;
		}

		Napi::Value retValue = callback.Call({
			inputs,
			outputs,
		});

		if (!retValue.IsBoolean()) {
			throw Napi::Error::New(env, "Return value of process function should be a boolean");
		}
		if (retValue.As<Napi::Boolean>().Value() == true) {
			processPromise.set_value(0);
		} else {
			processPromise.set_value(1);
		}
	});

	wrap->_processfnMutex.unlock();

	if ( processStatus != napi_ok )
	{
		return 0;
	} else {
		std::future<int> processFuture = processPromise.get_future();

		processFuture.wait();
		return processFuture.get();
	}

}

Napi::Object RtAudioWrap::Init(Napi::Env env, Napi::Object exports)
{
	// Define the class and get it's ctor function
	Napi::Function ctor_func =
		DefineClass(env, "RtAudio",
					{InstanceMethod("openStream", &RtAudioWrap::openStream),
					 InstanceMethod("closeStream", &RtAudioWrap::closeStream),
					 InstanceMethod("isStreamOpen", &RtAudioWrap::isStreamOpen),
					 InstanceMethod("start", &RtAudioWrap::start),
					 InstanceMethod("stop", &RtAudioWrap::stop),
					 InstanceMethod("isStreamRunning", &RtAudioWrap::isStreamRunning),
					 InstanceMethod("getApi", &RtAudioWrap::getApi),
					 InstanceMethod("getStreamLatency", &RtAudioWrap::getStreamLatency),
					 InstanceMethod("getStreamSampleRate", &RtAudioWrap::getStreamSampleRate),
					 InstanceMethod("getDevices", &RtAudioWrap::getDevices),
					 InstanceMethod("getDefaultInputDevice", &RtAudioWrap::getDefaultInputDevice),
					 InstanceMethod("getDefaultOutputDevice", &RtAudioWrap::getDefaultOutputDevice),
					 InstanceMethod("setProcessFunction", &RtAudioWrap::setProcessFunction),
					 InstanceMethod("_getExternal", &RtAudioWrap::_getExternal),
					 StaticMethod("_setProcessFunctionFromExternal", &RtAudioWrap::_setProcessFunctionFromExternal),
					});

	// Set the class's ctor function as a persistent object to keep it in memory
	constructor = Napi::Persistent(ctor_func);
	constructor.SuppressDestruct();

	// Export the ctor
	exports.Set("RtAudio", ctor_func);
	return exports;
}

RtAudioWrap::RtAudioWrap(
	const Napi::CallbackInfo &info
) :
	Napi::ObjectWrap<RtAudioWrap>(info),
	_frameSize(0),
	_inputChannels(0)
{
	RtAudio::Api api = info.Length() == 0 ? RtAudio::Api::UNSPECIFIED : (RtAudio::Api)(int)info[0].As<Napi::Number>();

	try
	{
		// Init the RtAudio object with the wanted api
		_rtAudio = std::make_shared<RtAudio>(api);
	}
	catch (std::exception &ex)
	{
		throw Napi::Error::New(info.Env(), ex.what());
	}
}

RtAudioWrap::~RtAudioWrap()
{
	_rtAudio->closeStream();
}

Napi::Value RtAudioWrap::getDevices(const Napi::CallbackInfo &info)
{
	Napi::Array devicesArray;
	std::vector<RtAudio::DeviceInfo> devices;

	// Determine the number of devices available
	unsigned int deviceCount = _rtAudio->getDeviceCount();

	// Scan through devices for various capabilities
	RtAudio::DeviceInfo device;
	for (unsigned int i = 0; i < deviceCount; i++)
	{
		// Get the device's info
		device = _rtAudio->getDeviceInfo(i);

		// If the device is probed
		if (device.probed)
		{
			devices.push_back(device);
		}
	}

	// Allocate the devices array
	devicesArray = Napi::Array::New(info.Env(), devices.size());

	// Convert the devices to objects
	for (unsigned int i = 0; i < devices.size(); i++)
	{
		devicesArray[i] = RtAudioConverter::ConvertDeviceInfo(info.Env(), devices[i]);
	}

	return devicesArray;
}

Napi::Value RtAudioWrap::getDefaultInputDevice(const Napi::CallbackInfo &info)
{
	return Napi::Number::New(info.Env(), _rtAudio->getDefaultInputDevice());
}

Napi::Value RtAudioWrap::getDefaultOutputDevice(const Napi::CallbackInfo &info)
{
	return Napi::Number::New(info.Env(), _rtAudio->getDefaultOutputDevice());
}

void RtAudioWrap::openStream(const Napi::CallbackInfo &info)
{
	RtAudio::StreamParameters outputParams = info[0].IsNull() || info[0].IsUndefined() ? RtAudio::StreamParameters() : RtAudioConverter::ConvertStreamParameters(info[0].As<Napi::Object>());
	RtAudio::StreamParameters inputParams = info[1].IsNull() || info[1].IsUndefined() ? RtAudio::StreamParameters() : RtAudioConverter::ConvertStreamParameters(info[1].As<Napi::Object>());
	RtAudioFormat format = (int)info[2].As<Napi::Number>();
	unsigned int sampleRate = info[3].As<Napi::Number>();
	unsigned int frameSize = info[4].As<Napi::Number>();
	std::string streamName = info[5].As<Napi::String>();
	Napi::Function processFrameCallback = info[6].IsNull() || info[6].IsUndefined() ? Napi::Function() : info[6].As<Napi::Function>();
	RtAudioStreamFlags flags = info.Length() < 8 ? 0 : info[7].As<Napi::Number>();

	RtAudio::StreamOptions options;

	// Set SINT24 as invalid
	if (format == RTAUDIO_SINT24)
	{
		throw Napi::Error::New(info.Env(), "24-bit signed integer is not available!");
	}

	_processfnMutex.lock();

	// If there is already a frame output threadsafe-function, release it
	if (_processFramefn != nullptr)
	{
		_processFramefn.Release();
		_processFramefn = nullptr;
	}

	// Save frame size
	_frameSize = frameSize;

	// Save input and output channels
	_inputChannels = inputParams.nChannels;
	_outputChannels = outputParams.nChannels;

	// Save the sample size by the format
	_sampleSize = getSampleSizeForFormat(format);

	// Save the format
	_format = format;

	// Set stream options
	options.flags = flags;
	options.streamName = streamName;

	try
	{
		// Open the stream
		_rtAudio->openStream(
			info[0].IsNull() || info[0].IsUndefined() ? nullptr : &outputParams,
			info[1].IsNull() || info[1].IsUndefined() ? nullptr : &inputParams,
			format,
			sampleRate,
			&frameSize,
			rt_callback,
			this,
			&options
		);
	}
	catch (std::exception &ex)
	{
		throw Napi::Error::New(info.Env(), ex.what());
	}

	if (!processFrameCallback.IsEmpty()) {
		_processFramefn = Napi::ThreadSafeFunction::New(info.Env(), processFrameCallback, "processFrameCallback", 0, 1, [this](Napi::Env) {});
	}

	_processfnMutex.unlock();
}

void RtAudioWrap::closeStream(const Napi::CallbackInfo &info)
{
	_rtAudio->closeStream();
}

Napi::Value RtAudioWrap::isStreamOpen(const Napi::CallbackInfo &info)
{
	return Napi::Boolean::New(info.Env(), _rtAudio->isStreamOpen());
}

void RtAudioWrap::start(const Napi::CallbackInfo &info)
{
	try
	{
		// Start the stream
		_rtAudio->startStream();
	}
	catch (std::exception &ex)
	{
		throw Napi::Error::New(info.Env(), ex.what());
	}
}

void RtAudioWrap::stop(const Napi::CallbackInfo &info)
{
	try
	{
		// Stop the stream
		_rtAudio->stopStream();
	}
	catch (std::exception &ex)
	{
		throw Napi::Error::New(info.Env(), ex.what());
	}
}

Napi::Value RtAudioWrap::isStreamRunning(const Napi::CallbackInfo &info)
{
	return Napi::Boolean::New(info.Env(), _rtAudio->isStreamRunning());
}

Napi::Value RtAudioWrap::getApi(const Napi::CallbackInfo &info)
{
	return Napi::String::New(info.Env(), _rtAudio->getApiDisplayName(_rtAudio->getCurrentApi()));
}

Napi::Value RtAudioWrap::getStreamLatency(const Napi::CallbackInfo &info)
{
	try
	{
		return Napi::Number::New(info.Env(), _rtAudio->getStreamLatency());
	}
	catch (std::exception &ex)
	{
		throw Napi::Error::New(info.Env(), ex.what());
	}
}

Napi::Value RtAudioWrap::getStreamSampleRate(const Napi::CallbackInfo &info)
{
	try
	{
		return Napi::Number::New(info.Env(), _rtAudio->getStreamSampleRate());
	}
	catch (std::exception &ex)
	{
		throw Napi::Error::New(info.Env(), ex.what());
	}
}

unsigned int RtAudioWrap::getSampleSizeForFormat(RtAudioFormat format)
{
	switch (format)
	{
	case RTAUDIO_SINT8:
		return 1;

	case RTAUDIO_SINT16:
		return 2;

	case RTAUDIO_SINT24:
		return 3;

	case RTAUDIO_SINT32:
	case RTAUDIO_FLOAT32:
		return 4;

	case RTAUDIO_FLOAT64:
		return 8;

	default:
		return 0;
	}
}

void RtAudioWrap::setProcessFunction(const Napi::CallbackInfo& info)
{
	if (info[0].IsEmpty() || !info[0].IsFunction()) {
		throw Napi::Error::New(info.Env(), "First argument should be the process function");
	}

	_processfnMutex.lock();

	_processFramefn = Napi::ThreadSafeFunction::New(info.Env(), info[0].As<Napi::Function>(), "processFrameCallback", 0, 1, [this](Napi::Env) {});

	_processfnMutex.unlock();
}

Napi::Value RtAudioWrap::_getExternal(const Napi::CallbackInfo& info)
{
	// we cannot pass an external between multiple nodejs thread so we use an array buffer to pass the pointer to thr RtAudioWrap instance
	// this is a big hack but I haven't found any other way of doing that
	// if thread are being created / deleted it could also lead to a segfault so be careful to call _setProcessFunctionFromExternal directly after calling _getExternal
	auto wrappedPointer = Napi::ArrayBuffer::New(info.Env(), sizeof(this));
	RtAudioWrap** dataAddr = reinterpret_cast<RtAudioWrap **>(wrappedPointer.Data());
	*dataAddr = this;
	return wrappedPointer;
}

void RtAudioWrap::_setProcessFunctionFromExternal(const Napi::CallbackInfo& info)
{
	if (info.Length() != 2 || !info[0].IsArrayBuffer() || !info[1].IsFunction()) {
		throw Napi::Error::New(info.Env(), "Two arguments should be passed: External Rtaudio instance and process function");
	}

	RtAudioWrap *wrap = *reinterpret_cast<RtAudioWrap **>(info[0].As<Napi::ArrayBuffer>().Data());

	wrap->_processfnMutex.lock();

	wrap->_processFramefn = Napi::ThreadSafeFunction::New(info.Env(), info[1].As<Napi::Function>(), "processFrameCallback", 0, 1);

	wrap->_processfnMutex.unlock();
}
