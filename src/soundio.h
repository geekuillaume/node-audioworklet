#pragma once

#include <soundio/soundio.h>
#include <memory>
#include <napi.h>
#include <queue>
#include <mutex>
#include <cmath>
#include <future>
#include <iostream>

class SoundioWrap : public Napi::ObjectWrap<SoundioWrap>
{
public:
	static Napi::Object Init(Napi::Env env, Napi::Object exports);
	SoundioWrap(const Napi::CallbackInfo &info);
	~SoundioWrap();

	Napi::Value getDevices(const Napi::CallbackInfo &info);
	Napi::Value getDefaultInputDevice(const Napi::CallbackInfo& info);
	Napi::Value getDefaultOutputDevice(const Napi::CallbackInfo& info);

	Napi::Value getApi(const Napi::CallbackInfo& info);

private:
	inline static Napi::FunctionReference constructor;

	SoundIo	*_soundio;
	Napi::Reference<Napi::Value> _ownRef;
};

class SoundioDevice : public Napi::ObjectWrap<SoundioDevice>
{
	public:
		SoundioDevice(const Napi::CallbackInfo &info);
		~SoundioDevice();
		inline static Napi::FunctionReference constructor;
		static void Init(Napi::Env& env, Napi::Object exports);

		Napi::Value getName(const Napi::CallbackInfo& info);
		Napi::Value getId(const Napi::CallbackInfo& info);
		Napi::Value getFormats(const Napi::CallbackInfo& info);
		Napi::Value getSampleRates(const Napi::CallbackInfo& info);
		Napi::Value getChannelLayouts(const Napi::CallbackInfo& info);

		Napi::Value getIsOutput(const Napi::CallbackInfo& info);
		Napi::Value getIsInput(const Napi::CallbackInfo& info);

		Napi::Value openOutputStream(const Napi::CallbackInfo &info);

	private:
		Napi::Reference<Napi::Value> _ownRef;
		SoundIoDevice *_device;
		bool _isInput;
		bool _isOutput;
};

class SoundioOutstream : public Napi::ObjectWrap<SoundioOutstream>
{
public:
	SoundioOutstream(const Napi::CallbackInfo &info);
	~SoundioOutstream();
	inline static Napi::FunctionReference constructor;
	static void Init(Napi::Env& env, Napi::Object exports);

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
	void _setProcessFunction(Napi::Env env, Napi::Function processFn);

	double _configuredOutputBufferDuration;
	unsigned int 	_outstreamFrameSize;
	std::vector<std::vector<uint8_t>>	_outstreamJsBuffer;

	std::mutex _processfnMutex;
	Napi::ThreadSafeFunction _processFramefn;
	Napi::Reference<Napi::Value> _ownRef;
};
