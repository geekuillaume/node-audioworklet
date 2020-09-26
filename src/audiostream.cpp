#include <future>
#include <assert.h>
#include <chrono>
#include <inttypes.h>

#include "audiostream.h"
#include "utils.h"
#include "debug.h"

long data_callback(cubeb_stream *stream, void *user_ptr, void const *input_buffer, void *output_buffer, long nframes)
{
	AudioStream *wrap = (AudioStream *)user_ptr;
	if (!wrap->_isStarted) {
		return 0;
	}
	if (wrap->_isInput) {
		wrap->_audioBuffer.get()->enqueueAudioBuffer(wrap->_timestamp, (uint8_t *)input_buffer, nframes);
	} else {
		int readFrames = wrap->_audioBuffer.get()->dequeueAudioBuffer(wrap->_timestamp, (uint8_t *)output_buffer, nframes);
	}
	wrap->_timestamp += nframes;
	return nframes;
}

void AudioStream::Init(Napi::Env &env, Napi::Object exports, ClassRegistry *registry)
{
  Napi::Function ctor_func =
    DefineClass(env,
      "AudioStream",
      {
        InstanceMethod("isStarted", &AudioStream::isStarted),

        InstanceMethod("start", &AudioStream::start),
        InstanceMethod("stop", &AudioStream::stop),

        InstanceMethod("getLatency", &AudioStream::getLatency),
        InstanceMethod("getPosition", &AudioStream::getPosition),
        InstanceMethod("setVolume", &AudioStream::setVolume),

        InstanceMethod("pushAudioChunk", &AudioStream::pushAudioChunk),
        InstanceMethod("readAudioChunk", &AudioStream::readAudioChunk),

        InstanceMethod("getBufferSize", &AudioStream::getBufferSize),
        InstanceMethod("getFormat", &AudioStream::getFormat),
        InstanceMethod("getChannels", &AudioStream::getChannels),
      }, registry);

  // Set the class's ctor function as a persistent object to keep it in memory
  registry->AudioStreamConstructor = Napi::Persistent(ctor_func);
  registry->AudioStreamConstructor.SuppressDestruct();

	exports.Set("AudioStream", ctor_func);
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
	if ((state == CUBEB_STATE_DRAINED || state == CUBEB_STATE_STOPPED) && wrap->_stream) {
		debug_print("Stream drained, cleaning\n");
		cubeb_stream_stop(wrap->_stream);
		wrap->_ownRef.Unref();
		wrap->_parentRef.Unref();
		wrap->_stateCallbackfn.Release();
		wrap->_isStarted = false;
		cubeb_stream_destroy(wrap->_stream);
		wrap->_stream = nullptr;
	}
}


AudioStream::AudioStream(
	const Napi::CallbackInfo &info
) :
	Napi::ObjectWrap<AudioStream>(info),
	_isStarted(false),
	_logProcessTime(false),
	_timestamp(0),
	_lastChunkTimestamp(0)
{
	int err;
	_ownRef = Napi::Reference<Napi::Value>::New(info.This());
	_parentRef = Napi::Reference<Napi::Value>::New(info[0], 1);
  _cubebContext = info[1].As<Napi::External<cubeb>>().Data();
	cubeb_device_info *device = info[2].As<Napi::External<cubeb_device_info>>().Data();
  _isInput = info[3].As<Napi::Boolean>().Value();
	const char *name = "Node-AudioWorklet";

	Napi::Object opts = info[4].IsUndefined() ? Napi::Object::New(info.Env()) : info[4].As<Napi::Object>();

	_params.format = CUBEB_SAMPLE_S16LE;
	_params.rate = 48000;
	_params.channels = 2;
	_params.layout = CUBEB_LAYOUT_UNDEFINED;
	_params.prefs = CUBEB_STREAM_PREF_NONE;

	if (device->type == CUBEB_DEVICE_TYPE_OUTPUT && _isInput) {
		_params.prefs = CUBEB_STREAM_PREF_LOOPBACK;
	}

	if (!opts.Get("disableSwitching").IsNull() && !opts.Get("disableSwitching").IsUndefined()) {
		if (opts.Get("disableSwitching").As<Napi::Boolean>().Value()) {
			_params.prefs = (cubeb_stream_prefs)(_params.prefs & CUBEB_STREAM_PREF_DISABLE_DEVICE_SWITCHING);
		}
	}

	if (!opts.Get("format").IsNull() && !opts.Get("format").IsUndefined()) {
		_params.format = (cubeb_sample_format)opts.Get("format").As<Napi::Number>().Int32Value();
		if (device->format & _params.format == 0) {
			throw Napi::Error::New(info.Env(), "format not supported");
		}
	}
	if (!opts.Get("sampleRate").IsNull() && !opts.Get("sampleRate").IsUndefined()) {
		_params.rate = opts.Get("sampleRate").As<Napi::Number>().Uint32Value();
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

	err = cubeb_get_min_latency(_cubebContext, &_params, &_configuredLatencyFrames);
	if (err != CUBEB_OK) {
		throw Napi::Error::New(info.Env(), "Error while getting min latency");
	}

	if (!opts.Get("latencyFrames").IsNull() && !opts.Get("latencyFrames").IsUndefined()) {
		uint32_t latency = opts.Get("latencyFrames").As<Napi::Number>().Uint32Value();
		if (latency < _configuredLatencyFrames) {
			throw Napi::Error::New(info.Env(), "configured latency is too low");
		}
		_configuredLatencyFrames = opts.Get("latencyFrames").As<Napi::Number>().Uint32Value();
	}
	info.This().As<Napi::Object>().Set("configuredLatencyFrames", Napi::Number::New(info.Env(), _configuredLatencyFrames));

	if (opts.Get("logProcessTime").IsBoolean()) {
		_logProcessTime = opts.Get("logProcessTime").As<Napi::Boolean>();
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
			&data_callback, // write callback
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
			&data_callback, // write callback
			&state_callback, // state change callback
			this // user ptr
		);
	}

	if (err != CUBEB_OK) {
		if (err == CUBEB_ERROR_NOT_SUPPORTED) {
			Napi::Error::New(info.Env(), "Options not supported").ThrowAsJavaScriptException();
		}
		Napi::Error::New(info.Env(), "Error while starting stream").ThrowAsJavaScriptException();;
	}

	_audioBuffer.reset(new AudioRingBuffer(10 * _params.rate, cubeb_sample_size(_params.format) * _params.channels)); // 10 seconds max buffer
}

AudioStream::~AudioStream()
{
	debug_print("Audio stream destroyed\n");
	if (_stream != nullptr) {
		cubeb_stream_destroy(_stream);
	}
}

void AudioStream::stop(const Napi::CallbackInfo &info)
{
	if (!_isStarted) {
		return;
	}
	_isStarted = false;
	if (_isInput) {
		cubeb_stream_stop(_stream);
	}
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

void AudioStream::setVolume(const Napi::CallbackInfo &info)
{
	if (_stream == nullptr) {
		throw Napi::Error::New(info.Env(), "Stream closed");
	}
	if (info[0].IsNull() || info[0].IsUndefined()) {
		throw Napi::Error::New(info.Env(), "First argument should be the volume");
	}
	cubeb_stream *stream = _stream;
	double volume = info[0].As<Napi::Number>().DoubleValue();

	if (volume < 0 || volume > 1) {
		throw Napi::Error::New(info.Env(), "volume should be between 0 and 1");
	}

	cubeb_stream_set_volume(stream, volume);
}

Napi::Value AudioStream::getLatency(const Napi::CallbackInfo &info)
{
	if (_stream == nullptr) {
		throw Napi::Error::New(info.Env(), "Stream closed");
	}
	uint32_t latencyInFrames;
	int err;

	if (_isInput) {
		err = cubeb_stream_get_input_latency(_stream, &latencyInFrames);
	} else {
		err = cubeb_stream_get_latency(_stream, &latencyInFrames);
	}

	if (err != CUBEB_OK) {
		throw Napi::Error::New(info.Env(), "Error while getting latency");
	}
	return Napi::Number::New(info.Env(), latencyInFrames);
}

Napi::Value AudioStream::getPosition(const Napi::CallbackInfo &info)
{
	if (_stream == nullptr) {
		throw Napi::Error::New(info.Env(), "Stream closed");
	}
	uint64_t positionInFrames;
	int err;

	err = cubeb_stream_get_position(_stream, &positionInFrames);

	if (err != CUBEB_OK) {
		throw Napi::Error::New(info.Env(), "Error while getting position");
	}
	return Napi::Number::New(info.Env(), positionInFrames);
}

Napi::Value AudioStream::pushAudioChunk(const Napi::CallbackInfo &info)
{
	if (_isInput) {
		throw Napi::Error::New(info.Env(), "This method can only be called on a output stream");
	}
	if (!info[0].IsUndefined() && !info[0].IsNumber()) {
		throw Napi::Error::New(info.Env(), "First argument should be the timestamp corresponding to the first sample in the buffer");
	}
	if (!info[1].IsTypedArray()) {
		throw Napi::Error::New(info.Env(), "Second argument should be the audio chunk TypedArray");
	}

	uint64_t timestamp;
	if (info[0].IsUndefined()) {
		timestamp = _lastChunkTimestamp;
	} else {
		timestamp = info[0].As<Napi::Number>().Int64Value();
	}
	if (timestamp < _lastChunkTimestamp && _lastChunkTimestamp != 0) {
		throw Napi::Error::New(info.Env(), "Cannot push audio chunk at a position lower than a previously pushed audio chunk");
	}

	Napi::TypedArray chunk = info[1].As<Napi::TypedArray>();
	if (chunk.TypedArrayType() != format_to_typedarray_type(_params.format)) {
		throw Napi::Error::New(info.Env(), "TypeArray type is not corresponding to the configured stream format");
	}
	Napi::ArrayBuffer buffer = chunk.ArrayBuffer();

	int writtenFrames = _audioBuffer.get()->enqueueAudioBuffer(timestamp, (uint8_t *)buffer.Data() + chunk.ByteOffset(), chunk.ElementLength() / _params.channels);
	_lastChunkTimestamp += writtenFrames;

	return Napi::Number::New(info.Env(), writtenFrames);
}

Napi::Value AudioStream::readAudioChunk(const Napi::CallbackInfo &info)
{
	if (!_isInput) {
		throw Napi::Error::New(info.Env(), "This method can only be called on an input stream");
	}
	if (!info[0].IsUndefined() && !info[0].IsNumber()) {
		throw Napi::Error::New(info.Env(), "First argument should be the timestamp corresponding to the first sample to read from the buffer");
	}
	if (!info[1].IsTypedArray()) {
		throw Napi::Error::New(info.Env(), "Second argument should be the destination TypedArray");
	}

	uint64_t timestamp;
	if (info[0].IsUndefined()) {
		timestamp = _lastChunkTimestamp;
	} else {
		timestamp = info[0].As<Napi::Number>().Int64Value();
	}
	if (timestamp < _lastChunkTimestamp && _lastChunkTimestamp != 0) {
		throw Napi::Error::New(info.Env(), "Cannot push audio chunk at a position lower than a previously pushed audio chunk");
	}

	Napi::TypedArray chunk = info[1].As<Napi::TypedArray>();
	if (chunk.TypedArrayType() != format_to_typedarray_type(_params.format)) {
		throw Napi::Error::New(info.Env(), "TypeArray type is not corresponding to the configured stream format");
	}
	Napi::ArrayBuffer buffer = chunk.ArrayBuffer();

	int readFrames = _audioBuffer.get()->dequeueAudioBuffer(timestamp, (uint8_t *)buffer.Data() + chunk.ByteOffset(), chunk.ElementLength() / _params.channels);
	_lastChunkTimestamp += readFrames;

	return Napi::Number::New(info.Env(), readFrames);
}

Napi::Value AudioStream::getBufferSize(const Napi::CallbackInfo &info) {
	int size;

	if (_isInput) {
		size = _audioBuffer.get()->available_read();
	} else {
		size = _audioBuffer.get()->available_write();
	}

	return Napi::Number::New(info.Env(), size);
}

Napi::Value AudioStream::getFormat(const Napi::CallbackInfo &info) {
	return Napi::Number::New(info.Env(), _params.format);
}

Napi::Value AudioStream::getChannels(const Napi::CallbackInfo &info) {
	return Napi::Number::New(info.Env(), _params.channels);
}

