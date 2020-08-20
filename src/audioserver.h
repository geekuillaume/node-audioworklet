#pragma once

#include <napi.h>

#include <memory>
#include <queue>
#include <mutex>
#include <cmath>
#include <iostream>

#include "cubeb/cubeb.h"

#include "./audiostream.h"
#include "class_registry.h"

void device_collection_changed_handler(cubeb *context, void *user_ptr);

class AudioServerWrap : public Napi::ObjectWrap<AudioServerWrap>
{
public:
	static void Init(Napi::Env& env, Napi::Object exports, ClassRegistry *registry);

	AudioServerWrap(const Napi::CallbackInfo &info);
	~AudioServerWrap();

	Napi::Value getDevices(const Napi::CallbackInfo &info);
	Napi::Value getApi(const Napi::CallbackInfo& info);
	Napi::Value initInputStream(const Napi::CallbackInfo& info);
	Napi::Value initOutputStream(const Napi::CallbackInfo& info);
	Napi::Value outputLoopbackSupported(const Napi::CallbackInfo& info);

	friend void device_collection_changed_handler(cubeb *context, void *user_ptr);

	static Napi::Value setDebugLog(const Napi::CallbackInfo& info);

	cubeb	*_cubeb;

private:
	inline static Napi::FunctionReference constructor;
	Napi::Reference<Napi::Value> _ownRef;
	Napi::ThreadSafeFunction _deviceChangeHandler;
	ClassRegistry *registry;
};

