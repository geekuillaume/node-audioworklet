#pragma once

#include <soundio/soundio.h>
#include <napi.h>
#include <iostream>
#include "class_registry.h"

class SoundioOutstreamWrap : public Napi::ObjectWrap<SoundioOutstreamWrap>
{
public:
	static void Init(Napi::Env& env, Napi::Object exports, ClassRegistry *registry);

	SoundioOutstreamWrap(const Napi::CallbackInfo &info);
	~SoundioOutstreamWrap();

	void close(const Napi::CallbackInfo &info);
	Napi::Value isOpen(const Napi::CallbackInfo& info);
	void clearBuffer(const Napi::CallbackInfo& info);
	void start(const Napi::CallbackInfo &info);
	void setPause(const Napi::CallbackInfo &info);
	void setVolume(const Napi::CallbackInfo &info);
	Napi::Value getLatency(const Napi::CallbackInfo& info);
	void setProcessFunction(const Napi::CallbackInfo& info);
	friend void write_callback(struct SoundIoOutStream *outstream, int frame_count_min, int frame_count_max);
	Napi::Value _getExternal(const Napi::CallbackInfo& info);
	static void _setProcessFunctionFromExternal(const Napi::CallbackInfo& info);

private:
	SoundIoDevice *_device;
	SoundIoOutStream *_outstream;
	void _setProcessFunction(const Napi::Env &env);

	double _configuredOutputBufferDuration;
	unsigned int 	_outstreamFrameSize;
	std::vector<std::vector<uint8_t>>	_outstreamJsBuffer;

	bool _isStarted;
	std::mutex _processfnMutex;
	Napi::Reference<Napi::Value> _processFctRef;
	Napi::ThreadSafeFunction _processFramefn;
	Napi::Reference<Napi::Value> _ownRef;
	Napi::Reference<Napi::Value> _parentDeviceRef;
	ClassRegistry *registry;
};

