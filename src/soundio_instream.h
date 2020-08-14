// #pragma once

// #include <soundio/soundio.h>
// #include <napi.h>
// #include "class_registry.h"

// class SoundioInstreamWrap : public Napi::ObjectWrap<SoundioInstreamWrap>
// {
// public:
// 	static void Init(Napi::Env& env, Napi::Object exports, ClassRegistry *registry);

// 	SoundioInstreamWrap(const Napi::CallbackInfo &info);
// 	~SoundioInstreamWrap();

// 	void close(const Napi::CallbackInfo &info);
// 	Napi::Value isOpen(const Napi::CallbackInfo& info);
// 	void start(const Napi::CallbackInfo &info);
// 	void setPause(const Napi::CallbackInfo &info);
// 	Napi::Value getLatency(const Napi::CallbackInfo& info);
// 	void setProcessFunction(const Napi::CallbackInfo& info);
// 	friend void read_callback(struct SoundIoInStream *instream, int frame_count_min, int frame_count_max);
// 	Napi::Value _getExternal(const Napi::CallbackInfo& info);
// 	static void _setProcessFunctionFromExternal(const Napi::CallbackInfo& info);

// private:
// 	SoundIoDevice *_device;
// 	SoundIoInStream *_instream;
// 	void _setProcessFunction(const Napi::Env &env);

// 	double _configuredInputBufferDuration;
// 	unsigned int 	_instreamFrameSize;
// 	std::vector<std::vector<uint8_t>>	_instreamJsBuffer;

// 	bool _isStarted;
// 	std::mutex _processfnMutex;
// 	Napi::Reference<Napi::Value> _processFctRef;
// 	Napi::ThreadSafeFunction _processFramefn;
// 	Napi::Reference<Napi::Value> _ownRef;
// 	Napi::Reference<Napi::Value> _parentDeviceRef;

// 	ClassRegistry *registry;
// };
