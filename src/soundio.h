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
	Napi::Value refreshDevicesCb(const Napi::CallbackInfo& info);

	std::mutex devicesInfoLock;
	SoundIo	*_soundio;

	std::vector<SoundIoDevice *> rawInputDevices;
	std::vector<SoundIoDevice *> rawOutputDevices;

	SoundIoDevice *defaultOutputDevice;
	SoundIoDevice *defaultInputDevice;

	bool _refreshDevices(const Napi::CallbackInfo& info);
	// this is used to prevent accessing to devices before initializing them with refreshDevices()
	bool hasBeenInitialized = false;
private:

	inline static Napi::FunctionReference constructor;

	Napi::Reference<Napi::Value> _ownRef;
	std::vector<Napi::ObjectReference> _outputDevices;
	std::vector<Napi::ObjectReference> _inputDevices;
	ClassRegistry *registry;
};

