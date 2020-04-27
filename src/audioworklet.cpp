#include <napi.h>

#include "soundio.h"
#include "class_registry.h"
#include <iostream>

Napi::Object Init(Napi::Env env, Napi::Object exports)
{
  Napi::HandleScope scope(env);

	auto registry = new ClassRegistry();

	SoundioWrap::Init(env, exports, registry);
	SoundioDeviceWrap::Init(env, exports, registry);
	SoundioOutstreamWrap::Init(env, exports, registry);
	SoundioInstreamWrap::Init(env, exports, registry);

	return exports;
}

NODE_API_MODULE(audioworklet, Init)
