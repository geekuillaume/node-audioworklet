#include <future>

#include "audiostream.h"

long write_callback(cubeb_stream *stream, void *user_ptr, void const *input_buffer, void *output_buffer, long nframes)
{
	// printf("Needs to write %d frames\n", nframes);
	AudioStream *wrap = (AudioStream *)user_ptr;
	if (!wrap->_isStarted) {
		return 0;
	}
	std::vector<std::vector<uint8_t>> &_outstreamJsBuffer = wrap->_outstreamJsBuffer;
	cubeb_sample_format format = wrap->_params.format;
	int framesPerJsCall = wrap->_outstreamFrameSize;
	int bytesPerSample = bytesPerFormat(format);
	uint8_t interleavedBuffer[((framesPerJsCall > nframes) ? framesPerJsCall : nframes) * wrap->_params.channels * bytesPerSample];

	int frames_left = nframes;
	while (wrap->_processFramefn && CircularBufferGetDataSize(wrap->_audioBuffer) < nframes * bytesPerSample * wrap->_params.channels) {
		std::promise<bool> processPromise;
		std::future<bool> processFuture = processPromise.get_future();

		for (std::vector<std::vector<uint8_t>>::iterator it = _outstreamJsBuffer.begin(); it != _outstreamJsBuffer.end(); ++it) {
			memset(it->data(), 0, it->size());
		}
		wrap->_processfnMutex.lock();

		auto processStatus = wrap->_processFramefn.BlockingCall([format, &_outstreamJsBuffer, framesPerJsCall, &processPromise](Napi::Env env, Napi::Function callback) {
			size_t bytes_per_sample = bytesPerFormat(format);
			int bufferSize = framesPerJsCall * bytes_per_sample;

			Napi::Array outputs = Napi::Array::New(env);

			outputs = Napi::Array::New(env, _outstreamJsBuffer.size());
			for (size_t channel = 0; channel < _outstreamJsBuffer.size(); channel++)
			{
				Napi::ArrayBuffer buffer = Napi::ArrayBuffer::New(env, _outstreamJsBuffer[channel].data(), _outstreamJsBuffer[channel].size());

				if (format == CUBEB_SAMPLE_S16LE || format == CUBEB_SAMPLE_S16BE) {
					outputs[channel] = Napi::TypedArrayOf<uint16_t>::New(env, bufferSize / bytes_per_sample, buffer, 0);
				} else if (format == CUBEB_SAMPLE_FLOAT32LE || format == CUBEB_SAMPLE_FLOAT32BE) {
					outputs[channel] = Napi::TypedArrayOf<float>::New(env, bufferSize / bytes_per_sample, buffer, 0);
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
			wrap->_processfnMutex.unlock();
			break;
		} else {
			// struct timespec start, end;
			// clock_gettime(CLOCK_MONOTONIC, &start);
			processFuture.wait();
			wrap->_processfnMutex.unlock();
			// clock_gettime(CLOCK_MONOTONIC, &end);

			// double time_taken;
			// time_taken = (end.tv_sec - start.tv_sec) * 1e9;
			// time_taken = (time_taken + (end.tv_nsec - start.tv_nsec)) * 1e-9;
			// std::cout << "Time taken by process function is : " << std::fixed
			// 		<< time_taken << std::setprecision(9);
			// std::cout << " sec" << std::endl;
		}

		if (processFuture.get() == false) {
			return 0;
		}
		for (int channel = 0; channel < wrap->_params.channels; channel += 1) {
			uint8_t *channelData = _outstreamJsBuffer[channel].data();
			for (int frame = 0; frame < framesPerJsCall; frame++) {
				for (int sampleByte = 0; sampleByte < bytesPerSample; sampleByte++) {
					interleavedBuffer[(((frame * wrap->_params.channels) + channel) * bytesPerSample) + sampleByte] = channelData[(frame * bytesPerSample) + sampleByte];
				}
			}
		}

		CircularBufferPush(wrap->_audioBuffer, interleavedBuffer, framesPerJsCall * wrap->_params.channels * bytesPerSample);
	}

	CircularBufferPop(wrap->_audioBuffer, nframes * bytesPerSample * wrap->_params.channels, output_buffer);
  return nframes;
}

void AudioStream::Init(Napi::Env &env, Napi::Object exports, ClassRegistry *registry)
{
  Napi::Function ctor_func =
    DefineClass(env,
      "AudioStream",
      {
        // InstanceMethod("close", &AudioStream::close),
        InstanceMethod("isStarted", &AudioStream::isStarted),

        InstanceMethod("start", &AudioStream::start),
        InstanceMethod("stop", &AudioStream::stop),
        // InstanceMethod("clearBuffer", &AudioStream::clearBuffer),
        // InstanceMethod("getLatency", &AudioStream::getLatency),
        InstanceMethod("setProcessFunction", &AudioStream::setProcessFunction),
        // InstanceMethod("setVolume", &AudioStream::setVolume),

        InstanceMethod("_getExternal", &AudioStream::_getExternal),
        StaticMethod("_setProcessFunctionFromExternal", &AudioStream::_setProcessFunctionFromExternal),
      }, registry);

  // Set the class's ctor function as a persistent object to keep it in memory
  registry->AudioStreamConstructor = Napi::Persistent(ctor_func);
  registry->AudioStreamConstructor.SuppressDestruct();

	exports.Set("AudioStream", ctor_func);
}

int bytesPerFormat(cubeb_sample_format format) {
	if (format == CUBEB_SAMPLE_S16LE || format == CUBEB_SAMPLE_S16BE) {
		return 2;
	}
	if (format == CUBEB_SAMPLE_FLOAT32LE || format == CUBEB_SAMPLE_FLOAT32BE) {
		return 4;
	}
	return 2;
}

void state_callback(cubeb_stream * stm, void *user_ptr, cubeb_state state)
{
	AudioStream *wrap = (AudioStream *)user_ptr;
	if (wrap->_stateCallbackfn) {
		wrap->_stateCallbackfn.BlockingCall([wrap, state](Napi::Env env, Napi::Function callback) {
			callback.Call({
				Napi::External<AudioStream>::New(env, wrap),
				Napi::Number::New(env, state)
			});
		});
	}
}

void state_callback_js(const Napi::CallbackInfo& info)
{
	AudioStream *wrap = info[0].As<Napi::External<AudioStream>>().Data();
	cubeb_state state = (cubeb_state)info[1].As<Napi::Number>().Int32Value();
	if (state == CUBEB_STATE_DRAINED) {
		cubeb_stream_stop(wrap->_stream);
		wrap->_ownRef.Unref();
		wrap->_stateCallbackfn.Release();
		wrap->_isStarted = false;
		if (wrap->_processFramefn) {
			wrap->_processfnMutex.lock();
			wrap->_processFramefn.Release();
			wrap->_processfnMutex.unlock();
		}
	}
}


AudioStream::AudioStream(
	const Napi::CallbackInfo &info
) :
	Napi::ObjectWrap<AudioStream>(info),
	_processFramefn(),
	_outstreamFrameSize(48000 / 100),
	_processFctRef(),
	_isStarted(false)
{
	int err;
	_ownRef = Napi::Reference<Napi::Value>::New(info.This());
	_parentRef = Napi::Reference<Napi::Value>::New(info[0], 1);
  _cubebContext = info[1].As<Napi::External<cubeb>>().Data();
	cubeb_device_info *device = info[2].As<Napi::External<cubeb_device_info>>().Data();
  _isInput = info[3].As<Napi::Boolean>().Value();
	const char *name = "Node-AudioWorklet";

	Napi::Object opts = info[4].As<Napi::Object>();

	_params = {
		.format = CUBEB_SAMPLE_S16LE,
		.rate = 48000,
		.channels = 2,
		.layout = CUBEB_LAYOUT_UNDEFINED,
		.prefs = CUBEB_STREAM_PREF_NONE,
	};

	if (!opts.Get("format").IsNull() && !opts.Get("format").IsUndefined()) {
		_params.format = (cubeb_sample_format)opts.Get("format").As<Napi::Number>().Int32Value();
		if (device->format & _params.format == 0) {
			throw Napi::Error::New(info.Env(), "format not supported");
		}
	}
	if (!opts.Get("sampleRate").IsNull() && !opts.Get("sampleRate").IsUndefined()) {
		_params.rate = opts.Get("sampleRate").As<Napi::Number>().Uint32Value();
		if (_params.rate < device->min_rate || _params.rate > device->max_rate) {
			throw Napi::Error::New(info.Env(), "sample rate not supported");
		}
	}
	if (!opts.Get("channels").IsNull() && !opts.Get("channels").IsUndefined()) {
		_params.channels = opts.Get("channels").As<Napi::Number>().Uint32Value();
		if (_params.channels < 1 || _params.channels > device->max_channels) {
			throw Napi::Error::New(info.Env(), "channel count not supported");
		}
	}
	if (!opts.Get("name").IsNull() && !opts.Get("name").IsUndefined()) {
		name = opts.Get("name").As<Napi::String>().Utf8Value().c_str();
	}

	if (!opts.Get("process").IsNull() && !opts.Get("process").IsUndefined()) {
		_processFctRef = Napi::Reference<Napi::Value>::New(opts.Get("process"), 1);
	}
	if (!opts.Get("frameSize").IsNull() && !opts.Get("frameSize").IsUndefined()) {
		_outstreamFrameSize = opts.Get("frameSize").As<Napi::Number>();
	}

	err = cubeb_get_min_latency(_cubebContext, &_params, &_configuredLatencyFrames);
	if (err != CUBEB_OK) {
		throw Napi::Error::New(info.Env(), "Error while getting min latency");
	}

	if (!opts.Get("latencyFrames").IsNull() && !opts.Get("latencyFrames").IsUndefined()) {
		uint32_t latency = opts.Get("bufferDuration").As<Napi::Number>().Uint32Value();
		if (latency < _configuredLatencyFrames) {
			throw Napi::Error::New(info.Env(), "configured latency is too low");
		}
		_configuredLatencyFrames = opts.Get("bufferDuration").As<Napi::Number>().Uint32Value();
	}

	for (size_t i = 0; i < _params.channels; i++)
	{
		_outstreamJsBuffer.push_back(
			std::vector<uint8_t>(_outstreamFrameSize * bytesPerFormat(_params.format), uint8_t(0))
		);
	}

	if (_isInput) {
		err = cubeb_stream_init(
			_cubebContext, // context
			&_stream, // stream ptr
			name, // stream name
			device->devid, // input device
			&_params, // input device params
			NULL, // output device
			NULL, // output device params
			_configuredLatencyFrames, // latency in frames
			&write_callback, // write callback
			&state_callback, // state change callback
			this // user ptr
		);
	} else {
		err = cubeb_stream_init(
			_cubebContext, // context
			&_stream, // stream ptr
			name, // stream name
			NULL, // input device
			NULL, // input device params
			device->devid, // output device
			&_params, // output device params
			_configuredLatencyFrames, // latency in frames
			&write_callback, // write callback
			&state_callback, // state change callback
			this // user ptr
		);
	}

	if (err != CUBEB_OK) {
		throw Napi::Error::New(info.Env(), "Error while starting stream");
	}

	_audioBuffer = CircularBufferCreate(_configuredLatencyFrames * bytesPerFormat(_params.format) * _params.channels);
	CircularBufferReset(_audioBuffer);

}

AudioStream::~AudioStream()
{
	cubeb_stream_destroy(_stream);
	CircularBufferFree(_audioBuffer);
}

void AudioStream::stop(const Napi::CallbackInfo &info)
{
	if (!_isStarted) {
		return;
	}
	_isStarted = false;
}

Napi::Value AudioStream::isStarted(const Napi::CallbackInfo &info)
{
	return Napi::Boolean::New(info.Env(), _isStarted);
}

void AudioStream::start(const Napi::CallbackInfo &info)
{
	if (_stream == nullptr) {
		throw Napi::Error::New(info.Env(), "Stream closed");
	}
	if (_isStarted) {
		return;
	}
	_isStarted = true;
	_ownRef.Ref();

	if (_processFramefn) {
		_processFramefn.Acquire();
	} else if (!_processFctRef.IsEmpty()) {
	 	_processfnMutex.lock();
		_setProcessFunction(info.Env());
	 	_processfnMutex.unlock();
	}

	if (!_stateCallbackfn) {
		_stateCallbackfn = Napi::ThreadSafeFunction::New(
			info.Env(),
			Napi::Function::New(info.Env(), state_callback_js),
			"audioWorkletstateCallback",
			0,
			1,
			[this](Napi::Env) {
				// prevent a segfault on exit when the write thread want to access a non-existing threadsafefunction
				_stateCallbackfn = nullptr;
		});
	} else {
		_stateCallbackfn.Acquire();
	}

	int err = cubeb_stream_start(_stream);
	if (err) {
		throw Napi::Error::New(info.Env(), "Error on stream start");
	}
}

// void AudioStream::setPause(const Napi::CallbackInfo &info)
// {
// 	if (_outstream == nullptr) {
// 		throw Napi::Error::New(info.Env(), "Outstream closed");
// 	}

// 	int err = soundio_outstream_pause(_outstream, info[0].IsUndefined() || info[0].IsNull() ? true : info[0].As<Napi::Boolean>());
// 	if (err) {
// 		throw Napi::Error::New(info.Env(), soundio_strerror(err));
// 	}
// }

// void AudioStream::setVolume(const Napi::CallbackInfo &info)
// {
// 	if (_outstream == nullptr) {
// 		throw Napi::Error::New(info.Env(), "Outstream closed");
// 	}
// 	if (info[0].IsNull() || info[0].IsUndefined()) {
// 		throw Napi::Error::New(info.Env(), "First argument should be the volume");
// 	}
// 	double volume = info[0].As<Napi::Number>().DoubleValue();

// 	if (volume < 0 || volume > 1) {
// 		throw Napi::Error::New(info.Env(), "volume should be between 0 and 1");
// 	}
// 	soundio_outstream_set_volume(_outstream, volume);
// }

// Napi::Value AudioStream::getLatency(const Napi::CallbackInfo &info)
// {
// 	if (_outstream == nullptr) {
// 		throw Napi::Error::New(info.Env(), "Outstream closed");
// 	}
// 	try
// 	{
// 		return Napi::Number::New(info.Env(), _outstream->software_latency);
// 	}
// 	catch (std::exception &ex)
// 	{
// 		throw Napi::Error::New(info.Env(), ex.what());
// 	}
// }

void AudioStream::_setProcessFunction(const Napi::Env &env)
{
	_processFramefn = Napi::ThreadSafeFunction::New(
		env,
		_processFctRef.Value().As<Napi::Function>(),
		"soundioProcessFrameCallback",
		0,
		1,
		[this](Napi::Env) {
			// prevent a segfault on exit when the write thread want to access a non-existing threadsafefunction
			_processFramefn = nullptr;
	});
	// _processFctRef.Unref();
}

void AudioStream::setProcessFunction(const Napi::CallbackInfo& info)
{
	if (info[0].IsEmpty() || !info[0].IsFunction()) {
		throw Napi::Error::New(info.Env(), "First argument should be the process function");
	}
 	_processfnMutex.lock();

	_processFctRef = Napi::Reference<Napi::Value>::New(info[0], 1);
	if (_isStarted) {
		_setProcessFunction(info.Env());
	}

 	_processfnMutex.unlock();
}

Napi::Value AudioStream::_getExternal(const Napi::CallbackInfo& info)
{
	// we cannot pass an external between multiple nodejs thread so we use an array buffer to pass the pointer to thr SoundioOutstream instance
	// this is a big hack but I haven't found any other way of doing that
	// if thread are being created / deleted it could also lead to a segfault so be careful to call _setProcessFunctionFromExternal directly after calling _getExternal
	auto wrappedPointer = Napi::ArrayBuffer::New(info.Env(), sizeof(this));
	AudioStream** dataAddr = reinterpret_cast<AudioStream **>(wrappedPointer.Data());
	*dataAddr = this;
	return wrappedPointer;
}

void AudioStream::_setProcessFunctionFromExternal(const Napi::CallbackInfo& info)
{
	if (info.Length() != 2 || !info[0].IsArrayBuffer() || !info[1].IsFunction()) {
		throw Napi::Error::New(info.Env(), "Two arguments should be passed: External Rtaudio instance and process function");
	}

	AudioStream *wrap = *static_cast<AudioStream **>(info[0].As<Napi::ArrayBuffer>().Data());

 	wrap->_processfnMutex.lock();

	wrap->_processFctRef = Napi::Reference<Napi::Value>::New(info[1], 1);
	if (wrap->_isStarted) {
		wrap->_setProcessFunction(info.Env());
	}

	wrap->_processfnMutex.unlock();
}

