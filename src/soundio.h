#pragma once

#include <soundio/soundio.h>
#include <napi.h>

#include <memory>
#include <queue>
#include <mutex>
#include <cmath>
#include <iostream>

#include "./soundio_instream.h"
#include "./soundio_outstream.h"
#include "./soundio_device.h"
#include "class_registry.h"

class SoundioWrap : public Napi::ObjectWrap<SoundioWrap>
{
public:
	static void Init(Napi::Env& env, Napi::Object exports, ClassRegistry *registry);

	SoundioWrap(const Napi::CallbackInfo &info);
	~SoundioWrap();

	Napi::Value getDevices(const Napi::CallbackInfo &info);
	Napi::Value getDefaultInputDevice(const Napi::CallbackInfo& info);
	Napi::Value getDefaultOutputDevice(const Napi::CallbackInfo& info);

	Napi::Value getApi(const Napi::CallbackInfo& info);
	Napi::Value refreshDevices(const Napi::CallbackInfo& info);

private:
	bool _refreshDevices(const Napi::CallbackInfo& info);

	inline static Napi::FunctionReference constructor;

	SoundIo	*_soundio;
	Napi::Reference<Napi::Value> _ownRef;
	std::vector<Napi::ObjectReference> _outputDevices;
	std::vector<Napi::ObjectReference> _inputDevices;
	ClassRegistry *registry;
};

