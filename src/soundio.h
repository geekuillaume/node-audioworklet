#pragma once

#include <soundio/soundio.h>
#include <memory>
#include <napi.h>
#include <queue>
#include <mutex>

class SoundioWrap : public Napi::ObjectWrap<SoundioWrap>
{
public:
	static Napi::Object Init(Napi::Env env, Napi::Object exports);
	SoundioWrap(const Napi::CallbackInfo &info);
	~SoundioWrap();

	Napi::Value getDevices(const Napi::CallbackInfo &info);
	Napi::Value getDefaultInputDeviceIndex(const Napi::CallbackInfo& info);
	Napi::Value getDefaultOutputDeviceIndex(const Napi::CallbackInfo& info);

	void openOutputStream(const Napi::CallbackInfo &info);
	void closeOutputStream(const Napi::CallbackInfo &info);
	Napi::Value isOutputStreamOpen(const Napi::CallbackInfo& info);
	void clearOutputBuffer(const Napi::CallbackInfo& info);

	void startOutputStream(const Napi::CallbackInfo &info);
	void setOutputPause(const Napi::CallbackInfo &info);
	void setOutputVolume(const Napi::CallbackInfo &info);

	Napi::Value getApi(const Napi::CallbackInfo& info);
	Napi::Value getStreamLatency(const Napi::CallbackInfo& info);
	// Napi::Value getStreamSampleRate(const Napi::CallbackInfo& info);
	void setProcessFunction(const Napi::CallbackInfo& info);

	friend void write_callback(struct SoundIoOutStream *outstream, int frame_count_min, int frame_count_max);

	Napi::Value _getExternal(const Napi::CallbackInfo& info);
	static void _setProcessFunctionFromExternal(const Napi::CallbackInfo& info);

private:
	void _setProcessFunction(Napi::Env env, Napi::Function processFn);
	inline static Napi::FunctionReference constructor;

	SoundIo	*_soundio;
	SoundIoOutStream *_outstream;
	SoundIoInStream *_instream;

	unsigned int 	_outstreamFrameSize;
	double _configuredOutputBufferDuration;
	std::vector<uint8_t *>	_outstreamJsBuffer;

	std::mutex _processfnMutex;
	Napi::ThreadSafeFunction _processFramefn;
	Napi::Reference<Napi::Value> _ownRef;

	// unsigned int getSampleSizeForFormat(RtAudioFormat format);
};
