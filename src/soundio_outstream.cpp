#include <future>

#include "soundio_outstream.h"

void write_callback(struct SoundIoOutStream *outstream, int frame_count_min, int frame_count_max)
{
	// std::cout << frame_count_min << " -> " << frame_count_max << '\n';
	SoundioOutstreamWrap *wrap = (SoundioOutstreamWrap *)outstream->userdata;
	SoundIoFormat format = outstream->format;

	int frames_left = frame_count_max;
	size_t outstreamFrameSize = wrap->_outstreamFrameSize == 0 ? frame_count_max : wrap->_outstreamFrameSize;

	size_t jsChannelBufferSize = outstreamFrameSize * outstream->bytes_per_sample;
	std::vector<std::vector<uint8_t>> &jsBuffer = wrap->_outstreamJsBuffer;

	for (std::vector<std::vector<uint8_t>>::iterator it = jsBuffer.begin(); it != jsBuffer.end(); ++it) {
		if (it->size() < jsChannelBufferSize) {
			it->resize(jsChannelBufferSize);
		}
	}

	struct SoundIoChannelArea *areas;
	double outLatency;
	int err;

	while (frames_left >= outstreamFrameSize)
	{
		std::promise<bool> processPromise;
		std::future<bool> processFuture = processPromise.get_future();

		for (std::vector<std::vector<uint8_t>>::iterator it = jsBuffer.begin(); it != jsBuffer.end(); ++it) {
			memset(it->data(), 0, it->size());
		}

		int frame_count = outstreamFrameSize;
		if ((err = soundio_outstream_begin_write(outstream, &areas, &frame_count))) {
			if (err != SoundIoErrorUnderflow) {
				// todo call an error callback here
				break;
			}
		}
		if (!frame_count)
			break;

		if (wrap->_processFramefn) {
			wrap->_processfnMutex.lock();

			auto processStatus = wrap->_processFramefn.BlockingCall([format, &jsBuffer, frame_count, &processPromise](Napi::Env env, Napi::Function callback) {
				size_t bytes_per_sample = soundio_get_bytes_per_sample(format);
				int bufferSize = frame_count * bytes_per_sample;

				Napi::Array outputs = Napi::Array::New(env);

				outputs = Napi::Array::New(env, jsBuffer.size());
				for (size_t channel = 0; channel < jsBuffer.size(); channel++)
				{
					Napi::ArrayBuffer buffer = Napi::ArrayBuffer::New(env, jsBuffer[channel].data(), jsBuffer[channel].size());

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
				soundio_outstream_pause(outstream, true);
			}
		}

		for (int channel = 0; channel < outstream->layout.channel_count; channel += 1) {
			for (int frame = 0; frame < frame_count; frame++) {
				memcpy(areas[channel].ptr + (areas[channel].step * frame), // destination: it can be interleaved or not so we need to take care of one frame after another
					jsBuffer[channel].data() + (frame * outstream -> bytes_per_sample), // source
					outstream->bytes_per_sample);
			}
		}

		if ((err = soundio_outstream_end_write(outstream))) {
			// todo call an error callback here
			break;
		}

		frames_left -= frame_count;
	}
}

void SoundioOutstreamWrap::Init(Napi::Env &env, Napi::Object exports, ClassRegistry *registry)
{
  Napi::Function ctor_func =
    DefineClass(env,
      "SoundioOutstream",
      {
        InstanceMethod("close", &SoundioOutstreamWrap::close),
        InstanceMethod("isOpen", &SoundioOutstreamWrap::isOpen),

        InstanceMethod("start", &SoundioOutstreamWrap::start),
        InstanceMethod("setPause", &SoundioOutstreamWrap::setPause),
        InstanceMethod("clearBuffer", &SoundioOutstreamWrap::clearBuffer),
        InstanceMethod("getLatency", &SoundioOutstreamWrap::getLatency),
        InstanceMethod("setProcessFunction", &SoundioOutstreamWrap::setProcessFunction),
        InstanceMethod("setVolume", &SoundioOutstreamWrap::setVolume),

        InstanceMethod("_getExternal", &SoundioOutstreamWrap::_getExternal),
        StaticMethod("_setProcessFunctionFromExternal", &SoundioOutstreamWrap::_setProcessFunctionFromExternal),
      }, registry);

  // Set the class's ctor function as a persistent object to keep it in memory
  registry->SoundioOutstreamConstructor = Napi::Persistent(ctor_func);
  registry->SoundioOutstreamConstructor.SuppressDestruct();

	exports.Set("SoundioOutstream", ctor_func);
}

SoundioOutstreamWrap::SoundioOutstreamWrap(
	const Napi::CallbackInfo &info
) :
	Napi::ObjectWrap<SoundioOutstreamWrap>(info),
	_processFramefn(),
	_outstreamFrameSize(0),
	_configuredOutputBufferDuration(0.1),
	_processFctRef()
{
	registry = static_cast<ClassRegistry *>(info.Data());
	_ownRef = Napi::Reference<Napi::Value>::New(info.This());
	_parentDeviceRef = Napi::Reference<Napi::Value>::New(info[0], 1);
  _device = info[1].As<Napi::External<SoundIoDevice>>().Data();

  SoundIoOutStream *outstream = soundio_outstream_create(_device);
	Napi::Object opts = info[2].IsUndefined() ? Napi::Object::New(info.Env()) : info[2].As<Napi::Object>();

	if (!opts.Get("format").IsNull() && !opts.Get("format").IsUndefined()) {
		outstream->format = (SoundIoFormat)opts.Get("format").As<Napi::Number>().Int32Value();
		if (!soundio_device_supports_format(_device, outstream->format)) {
			throw Napi::Error::New(info.Env(), "format not supported");
		}
	}
	if (!opts.Get("sampleRate").IsNull() && !opts.Get("sampleRate").IsUndefined()) {
		outstream->sample_rate = opts.Get("sampleRate").As<Napi::Number>();
		if (!soundio_device_supports_sample_rate(_device, outstream->sample_rate)) {
			throw Napi::Error::New(info.Env(), "sample rate not supported");
		}
	}
	if (!opts.Get("name").IsNull() && !opts.Get("name").IsUndefined()) {
		outstream->name = opts.Get("name").As<Napi::String>().Utf8Value().c_str();
	}

	if (!opts.Get("process").IsNull() && !opts.Get("process").IsUndefined()) {
		_processFctRef = Napi::Reference<Napi::Value>::New(opts.Get("process"), 1);
	}
	if (!opts.Get("frameSize").IsNull() && !opts.Get("frameSize").IsUndefined()) {
		_outstreamFrameSize = opts.Get("frameSize").As<Napi::Number>();
	}
	if (!opts.Get("bufferDuration").IsNull() && !opts.Get("bufferDuration").IsUndefined()) {
		_configuredOutputBufferDuration = opts.Get("bufferDuration").As<Napi::Number>().DoubleValue();
	}

	outstream->userdata = this;
	outstream->write_callback = write_callback;
	outstream->software_latency = _configuredOutputBufferDuration;

  int err;
	if ((err = soundio_outstream_open(outstream))) {
			soundio_outstream_destroy(outstream);
			throw Napi::Error::New(info.Env(), soundio_strerror(err));
	}

	for (size_t i = 0; i < outstream->layout.channel_count; i++)
	{
		_outstreamJsBuffer.push_back(std::vector<uint8_t>(_outstreamFrameSize * outstream->bytes_per_sample, uint8_t(0)));
	}

	_outstream = outstream;
}

SoundioOutstreamWrap::~SoundioOutstreamWrap()
{
  soundio_outstream_destroy(_outstream);
}

void SoundioOutstreamWrap::close(const Napi::CallbackInfo &info)
{
	if (_outstream == nullptr) {
		throw Napi::Error::New(info.Env(), "The outstream is closed");
	}
	soundio_outstream_destroy(_outstream);
	if (_processFramefn) {
		_processFramefn.Release();
	}
  _outstream = nullptr;
	_ownRef.Unref();
}

void SoundioOutstreamWrap::clearBuffer(const Napi::CallbackInfo &info)
{
	if (_outstream == nullptr) {
		throw Napi::Error::New(info.Env(), "The outstream is closed");
	}

	soundio_outstream_clear_buffer(_outstream);
}

Napi::Value SoundioOutstreamWrap::isOpen(const Napi::CallbackInfo &info)
{
	return Napi::Boolean::New(info.Env(), _outstream != nullptr);
}

void SoundioOutstreamWrap::start(const Napi::CallbackInfo &info)
{
	if (_outstream == nullptr) {
		throw Napi::Error::New(info.Env(), "Instream closed");
	}

	_isStarted = true;
	_ownRef.Ref();

	if (!_processFctRef.IsEmpty()) {
	 	_processfnMutex.lock();
		_setProcessFunction(info.Env());
	 	_processfnMutex.unlock();
	}


	int err = soundio_outstream_start(_outstream);
	if (err) {
		throw Napi::Error::New(info.Env(), soundio_strerror(err));
	}
}

void SoundioOutstreamWrap::setPause(const Napi::CallbackInfo &info)
{
	if (_outstream == nullptr) {
		throw Napi::Error::New(info.Env(), "Outstream closed");
	}

	int err = soundio_outstream_pause(_outstream, info[0].IsUndefined() || info[0].IsNull() ? true : info[0].As<Napi::Boolean>());
	if (err) {
		throw Napi::Error::New(info.Env(), soundio_strerror(err));
	}
}

void SoundioOutstreamWrap::setVolume(const Napi::CallbackInfo &info)
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

Napi::Value SoundioOutstreamWrap::getLatency(const Napi::CallbackInfo &info)
{
	if (_outstream == nullptr) {
		throw Napi::Error::New(info.Env(), "Outstream closed");
	}
	try
	{
		return Napi::Number::New(info.Env(), _outstream->software_latency);
	}
	catch (std::exception &ex)
	{
		throw Napi::Error::New(info.Env(), ex.what());
	}
}

void SoundioOutstreamWrap::_setProcessFunction(const Napi::Env &env)
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
	_processFctRef.Unref();
}

void SoundioOutstreamWrap::setProcessFunction(const Napi::CallbackInfo& info)
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

Napi::Value SoundioOutstreamWrap::_getExternal(const Napi::CallbackInfo& info)
{
	// we cannot pass an external between multiple nodejs thread so we use an array buffer to pass the pointer to thr SoundioOutstream instance
	// this is a big hack but I haven't found any other way of doing that
	// if thread are being created / deleted it could also lead to a segfault so be careful to call _setProcessFunctionFromExternal directly after calling _getExternal
	auto wrappedPointer = Napi::ArrayBuffer::New(info.Env(), sizeof(this));
	SoundioOutstreamWrap** dataAddr = reinterpret_cast<SoundioOutstreamWrap **>(wrappedPointer.Data());
	*dataAddr = this;
	return wrappedPointer;
}

void SoundioOutstreamWrap::_setProcessFunctionFromExternal(const Napi::CallbackInfo& info)
{
	if (info.Length() != 2 || !info[0].IsArrayBuffer() || !info[1].IsFunction()) {
		throw Napi::Error::New(info.Env(), "Two arguments should be passed: External Rtaudio instance and process function");
	}

	SoundioOutstreamWrap *wrap = *static_cast<SoundioOutstreamWrap **>(info[0].As<Napi::ArrayBuffer>().Data());

 	wrap->_processfnMutex.lock();

	wrap->_processFctRef = Napi::Reference<Napi::Value>::New(info[1], 1);
	if (wrap->_isStarted) {
		wrap->_setProcessFunction(info.Env());
	}

	wrap->_processfnMutex.unlock();
}

