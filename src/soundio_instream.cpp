// #include <future>

// #include "soundio_instream.h"

// void read_callback(struct SoundIoInStream *instream, int frame_count_min, int frame_count_max)
// {
// 	SoundioInstreamWrap *wrap = (SoundioInstreamWrap *)instream->userdata;
// 	SoundIoFormat format = instream->format;

// 	int frames_left = frame_count_max;
// 	size_t outstreamFrameSize = wrap->_instreamFrameSize == 0 ? frame_count_max : wrap->_instreamFrameSize;

// 	size_t jsChannelBufferSize = outstreamFrameSize * instream->bytes_per_sample;
// 	std::vector<std::vector<uint8_t>> &jsBuffer = wrap->_instreamJsBuffer;

// 	for (std::vector<std::vector<uint8_t>>::iterator it = jsBuffer.begin(); it != jsBuffer.end(); ++it) {
// 		if (it->size() < jsChannelBufferSize) {
// 			it->resize(jsChannelBufferSize);
// 		}
// 	}

// 	struct SoundIoChannelArea *areas;
// 	double outLatency;
// 	int err;
// 	// wrap->_processfnMutex.lock();

// 	while (frames_left >= outstreamFrameSize)
// 	{
// 		std::promise<bool> processPromise;
// 		std::future<bool> processFuture = processPromise.get_future();

// 		for (std::vector<std::vector<uint8_t>>::iterator it = jsBuffer.begin(); it != jsBuffer.end(); ++it) {
// 			memset(it->data(), 0, it->size());
// 		}

// 		int frame_count = outstreamFrameSize;
// 		if ((err = soundio_instream_begin_read(instream, &areas, &frame_count))) {
//       break;
// 		}
// 		if (!frame_count) {
// 			break;
// 		}

// 		if (wrap->_processFramefn) {

//       for (int channel = 0; channel < instream->layout.channel_count; channel += 1) {
//         for (int frame = 0; frame < frame_count; frame++) {
//           memcpy(jsBuffer[channel].data() + (frame * instream->bytes_per_sample), // destination
//             areas[channel].ptr + (areas[channel].step * frame), // source: it can be interleaved or not so we need to take care of one frame after another
//             instream->bytes_per_sample);
//         }
//       }


// 			auto processStatus = wrap->_processFramefn.BlockingCall([format, &jsBuffer, frame_count, &processPromise](Napi::Env env, Napi::Function callback) {
// 				size_t bytes_per_sample = soundio_get_bytes_per_sample(format);
// 				int bufferSize = frame_count * bytes_per_sample;

// 				Napi::Array outputs = Napi::Array::New(env);

// 				outputs = Napi::Array::New(env, jsBuffer.size());
// 				for (size_t channel = 0; channel < jsBuffer.size(); channel++)
// 				{
// 					Napi::ArrayBuffer buffer = Napi::ArrayBuffer::New(env, jsBuffer[channel].data(), jsBuffer[channel].size());

// 					if (format == SoundIoFormatS8) {
// 						outputs[channel] = Napi::TypedArrayOf<int8_t>::New(env, bufferSize / bytes_per_sample, buffer, 0);
// 					} else if (format == SoundIoFormatU8) {
// 						outputs[channel] = Napi::TypedArrayOf<uint8_t>::New(env, bufferSize / bytes_per_sample, buffer, 0);
// 					} else if (format == SoundIoFormatS16LE || format == SoundIoFormatS16LE) {
// 						outputs[channel] = Napi::TypedArrayOf<int16_t>::New(env, bufferSize / bytes_per_sample, buffer, 0);
// 					} else if (format == SoundIoFormatU16LE || format == SoundIoFormatU16BE) {
// 						outputs[channel] = Napi::TypedArrayOf<uint16_t>::New(env, bufferSize / bytes_per_sample, buffer, 0);
// 					} else if (format == SoundIoFormatS32LE || format == SoundIoFormatS32BE) {
// 						outputs[channel] = Napi::TypedArrayOf<int32_t>::New(env, bufferSize / bytes_per_sample, buffer, 0);
// 					} else if (format == SoundIoFormatU32LE || format == SoundIoFormatU32BE) {
// 						outputs[channel] = Napi::TypedArrayOf<uint32_t>::New(env, bufferSize / bytes_per_sample, buffer, 0);
// 					} else if (format == SoundIoFormatFloat32LE || format == SoundIoFormatFloat32BE) {
// 						outputs[channel] = Napi::TypedArrayOf<float>::New(env, bufferSize / bytes_per_sample, buffer, 0);
// 					} else if (format == SoundIoFormatFloat64LE || format == SoundIoFormatFloat64BE) {
// 						outputs[channel] = Napi::TypedArrayOf<double>::New(env, bufferSize / bytes_per_sample, buffer, 0);
// 					}
// 				}

// 				Napi::Value retValue = callback.Call({
// 					outputs,
// 				});

// 				if (!retValue.IsBoolean()) {
// 					throw Napi::Error::New(env, "Return value of process function should be a boolean");
// 				}
// 				processPromise.set_value(retValue.As<Napi::Boolean>().Value());
// 			});

// 			if ( processStatus != napi_ok )
// 			{
// 				break;
// 			} else {
// 				// struct timespec start, end;
// 				// clock_gettime(CLOCK_MONOTONIC, &start);
// 				processFuture.wait();
// 				// clock_gettime(CLOCK_MONOTONIC, &end);

// 				// double time_taken;
// 				// time_taken = (end.tv_sec - start.tv_sec) * 1e9;
// 				// time_taken = (time_taken + (end.tv_nsec - start.tv_nsec)) * 1e-9;
// 				// std::cout << "Time taken by process function is : " << std::fixed
// 				// 		<< time_taken << std::setprecision(9);
// 				// std::cout << " sec" << std::endl;
// 			}

// 			if (processFuture.get() == false) {
// 				soundio_instream_pause(instream, true);
// 			}
// 		}

// 		if ((err = soundio_instream_end_read(instream))) {
// 			// todo call an error callback here
// 			break;
// 		}

// 		frames_left -= frame_count;
// 	}
// }

// void SoundioInstreamWrap::Init(Napi::Env &env, Napi::Object exports, ClassRegistry *registry)
// {
//   Napi::Function ctor_func =
//     DefineClass(env,
//       "SoundioInstream",
//       {
//         InstanceMethod("close", &SoundioInstreamWrap::close),
//         InstanceMethod("isOpen", &SoundioInstreamWrap::isOpen),

//         InstanceMethod("start", &SoundioInstreamWrap::start),
//         InstanceMethod("setPause", &SoundioInstreamWrap::setPause),
//         InstanceMethod("getLatency", &SoundioInstreamWrap::getLatency),
//         InstanceMethod("setProcessFunction", &SoundioInstreamWrap::setProcessFunction),

//         InstanceMethod("_getExternal", &SoundioInstreamWrap::_getExternal),
//         StaticMethod("_setProcessFunctionFromExternal", &SoundioInstreamWrap::_setProcessFunctionFromExternal),
//       }, registry);

//   // Set the class's ctor function as a persistent object to keep it in memory
//   registry->SoundioInstreamConstructor = Napi::Persistent(ctor_func);
//   registry->SoundioInstreamConstructor.SuppressDestruct();

// 	exports.Set("SoundioInstream", ctor_func);
// }

// SoundioInstreamWrap::SoundioInstreamWrap(
// 	const Napi::CallbackInfo &info
// ) :
// 	Napi::ObjectWrap<SoundioInstreamWrap>(info),
// 	_processFramefn(nullptr),
// 	_instreamFrameSize(0),
// 	_configuredInputBufferDuration(0.1),
// 	_isStarted(false),
// 	_processFctRef()
// {
// 	registry = static_cast<ClassRegistry *>(info.Data());
// 	_ownRef = Napi::Reference<Napi::Value>::New(info.This()); // this is used to prevent the GC to collect this object while a stream is running
// 	_parentDeviceRef = Napi::Reference<Napi::Value>::New(info[0], 1);
//   _device = info[1].As<Napi::External<SoundIoDevice>>().Data();

//   SoundIoInStream *instream = soundio_instream_create(_device);
// 	Napi::Object opts = info[2].IsUndefined() ? Napi::Object::New(info.Env()) : info[2].As<Napi::Object>();

// 	if (!opts.Get("format").IsNull() && !opts.Get("format").IsUndefined()) {
// 		instream->format = (SoundIoFormat)opts.Get("format").As<Napi::Number>().Int32Value();
// 		if (!soundio_device_supports_format(_device, instream->format)) {
// 			throw Napi::Error::New(info.Env(), "format not supported");
// 		}
// 	} else {
//     if (soundio_device_supports_format(_device, SoundIoFormatFloat32NE)) {
// 			instream->format = SoundIoFormatFloat32NE;
// 		} else {
//       instream->format = _device->formats[0];
//     }
//   }
// 	if (!opts.Get("sampleRate").IsNull() && !opts.Get("sampleRate").IsUndefined()) {
// 		instream->sample_rate = opts.Get("sampleRate").As<Napi::Number>();
// 		if (!soundio_device_supports_sample_rate(_device, instream->sample_rate)) {
// 			throw Napi::Error::New(info.Env(), "sample rate not supported");
// 		}
// 	}
// 	if (!opts.Get("name").IsNull() && !opts.Get("name").IsUndefined()) {
// 		instream->name = opts.Get("name").As<Napi::String>().Utf8Value().c_str();
// 	}

// 	if (!opts.Get("process").IsNull() && !opts.Get("process").IsUndefined()) {
// 		_processFctRef = Napi::Reference<Napi::Value>::New(opts.Get("process"), 1);
// 	}
// 	if (!opts.Get("frameSize").IsNull() && !opts.Get("frameSize").IsUndefined()) {
// 		_instreamFrameSize = opts.Get("frameSize").As<Napi::Number>();
// 	}
// 	if (!opts.Get("bufferDuration").IsNull() && !opts.Get("bufferDuration").IsUndefined()) {
// 		_configuredInputBufferDuration = opts.Get("bufferDuration").As<Napi::Number>().DoubleValue();
// 	}
// 	instream->userdata = this;
// 	instream->read_callback = read_callback;
// 	instream->software_latency = _configuredInputBufferDuration;

//   int err;
// 	if ((err = soundio_instream_open(instream))) {
// 			soundio_instream_destroy(instream);
// 			throw Napi::Error::New(info.Env(), soundio_strerror(err));
// 	}

// 	for (size_t i = 0; i < instream->layout.channel_count; i++)
// 	{
// 		_instreamJsBuffer.push_back(std::vector<uint8_t>(_instreamFrameSize * instream->bytes_per_sample, uint8_t(0)));
// 	}

// 	_instream = instream;
// }

// SoundioInstreamWrap::~SoundioInstreamWrap()
// {
//   soundio_instream_destroy(_instream);
// }

// void SoundioInstreamWrap::close(const Napi::CallbackInfo &info)
// {
// 	if (_instream == nullptr) {
// 		throw Napi::Error::New(info.Env(), "The outstream is closed");
// 	}
// 	soundio_instream_destroy(_instream);
// 	if (_processFramefn) {
// 		_processFramefn.Release();
// 	}
//   _instream = nullptr;
// 	_ownRef.Unref();
// }

// Napi::Value SoundioInstreamWrap::isOpen(const Napi::CallbackInfo &info)
// {
// 	return Napi::Boolean::New(info.Env(), _instream != nullptr);
// }

// void SoundioInstreamWrap::start(const Napi::CallbackInfo &info)
// {
// 	if (_instream == nullptr) {
// 		throw Napi::Error::New(info.Env(), "Instream closed");
// 	}

// 	_isStarted = true;
// 	_ownRef.Ref();

// 	if (!_processFctRef.IsEmpty()) {
// 		_setProcessFunction(info.Env());
// 	}

// 	int err = soundio_instream_start(_instream);
// 	if (err) {
// 		throw Napi::Error::New(info.Env(), soundio_strerror(err));
// 	}
// }

// void SoundioInstreamWrap::setPause(const Napi::CallbackInfo &info)
// {
// 	if (_instream == nullptr) {
// 		throw Napi::Error::New(info.Env(), "Instream closed");
// 	}

// 	int err = soundio_instream_pause(_instream, info[0].IsUndefined() || info[0].IsNull() ? true : info[0].As<Napi::Boolean>());
// 	if (err) {
// 		throw Napi::Error::New(info.Env(), soundio_strerror(err));
// 	}
// }

// Napi::Value SoundioInstreamWrap::getLatency(const Napi::CallbackInfo &info)
// {
// 	if (_instream == nullptr) {
// 		throw Napi::Error::New(info.Env(), "Instream closed");
// 	}
// 	try
// 	{
// 		return Napi::Number::New(info.Env(), _instream->software_latency);
// 	}
// 	catch (std::exception &ex)
// 	{
// 		throw Napi::Error::New(info.Env(), ex.what());
// 	}
// }

// void SoundioInstreamWrap::_setProcessFunction(const Napi::Env &env)
// {
//  	_processfnMutex.lock();

// 	_processFramefn = Napi::ThreadSafeFunction::New(env, _processFctRef.Value().As<Napi::Function>(), "soundioProcessFrameCallback", 0, 1, [this](Napi::Env) {
// 		// prevent a segfault on exit when the write thread want to access a non-existing threadsafefunction
// 		_processFramefn = nullptr;
// 	});
// 	_processFctRef.Unref();

// 	_processfnMutex.unlock();
// }

// void SoundioInstreamWrap::setProcessFunction(const Napi::CallbackInfo& info)
// {
// 	if (info[0].IsEmpty() || !info[0].IsFunction()) {
// 		throw Napi::Error::New(info.Env(), "First argument should be the process function");
// 	}
// 	_processFctRef = Napi::Reference<Napi::Value>::New(info[0], 1);
// 	if (_isStarted) {
// 		_setProcessFunction(info.Env());
// 	}
// }

// Napi::Value SoundioInstreamWrap::_getExternal(const Napi::CallbackInfo& info)
// {
// 	// we cannot pass an external between multiple nodejs thread so we use an array buffer to pass the pointer to thr SoundioInstream instance
// 	// this is a big hack but I haven't found any other way of doing that
// 	// if thread are being created / deleted it could also lead to a segfault so be careful to call _setProcessFunctionFromExternal directly after calling _getExternal
// 	auto wrappedPointer = Napi::ArrayBuffer::New(info.Env(), sizeof(this));
// 	SoundioInstreamWrap** dataAddr = reinterpret_cast<SoundioInstreamWrap **>(wrappedPointer.Data());
// 	*dataAddr = this;
// 	return wrappedPointer;
// }

// void SoundioInstreamWrap::_setProcessFunctionFromExternal(const Napi::CallbackInfo& info)
// {
// 	if (info.Length() != 2 || !info[0].IsArrayBuffer() || !info[1].IsFunction()) {
// 		throw Napi::Error::New(info.Env(), "Two arguments should be passed: External Rtaudio instance and process function");
// 	}

// 	SoundioInstreamWrap *wrap = *static_cast<SoundioInstreamWrap **>(info[0].As<Napi::ArrayBuffer>().Data());

// 	wrap->_processFctRef = Napi::Reference<Napi::Value>::New(info[1], 1);
// 	if (wrap->_isStarted) {
// 		wrap->_setProcessFunction(info.Env());
// 	}
// }
