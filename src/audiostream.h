#pragma once

#include <napi.h>
#include <iostream>
#include "cubeb/cubeb.h"
#include "class_registry.h"
extern "C"
{
	#include "CircularBuffer.h"
}


int bytesPerFormat(cubeb_sample_format format);

class AudioStream : public Napi::ObjectWrap<AudioStream>
{
public:
	static void Init(Napi::Env& env, Napi::Object exports, ClassRegistry *registry);

	AudioStream(const Napi::CallbackInfo &info);
	~AudioStream();

	Napi::Value isStarted(const Napi::CallbackInfo& info);
	void start(const Napi::CallbackInfo &info);
	void stop(const Napi::CallbackInfo &info);
	void setVolume(const Napi::CallbackInfo &info);
	Napi::Value getLatency(const Napi::CallbackInfo& info);
	void setProcessFunction(const Napi::CallbackInfo& info);

	friend long data_callback(cubeb_stream *stream, void *user_ptr, void const *input_buffer, void *output_buffer, long nframes);
	friend void state_callback(cubeb_stream * stm, void * user, cubeb_state state);
	friend void state_callback_js(const Napi::CallbackInfo& info);

	Napi::Value _getExternal(const Napi::CallbackInfo& info);
	static void _setProcessFunctionFromExternal(const Napi::CallbackInfo& info);
	static Napi::Value _getLatencyFromExternal(const Napi::CallbackInfo& info);

private:
	cubeb_stream *_stream;
	cubeb *_cubebContext;
	void _setProcessFunction(const Napi::Env &env);

	uint32_t _configuredLatencyFrames;
	uint32_t 	_streamFrameSize;
	std::vector<std::vector<uint8_t>>	_streamJsBuffer;
	std::vector<int8_t> _interleavedBuffer;
	Napi::Reference<Napi::Array> _channelsDataRef;

	bool _isStarted;
	std::mutex _processfnMutex;
	Napi::Reference<Napi::Value> _processFctRef;
	Napi::ThreadSafeFunction _processFramefn;
	Napi::ThreadSafeFunction _stateCallbackfn;
	Napi::Reference<Napi::Value> _ownRef;
	Napi::Reference<Napi::Value> _parentRef;
	bool _isInput;
	CircularBuffer _audioBuffer;
	cubeb_stream_params _params;
};

