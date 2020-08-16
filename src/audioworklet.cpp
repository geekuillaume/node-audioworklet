#include <napi.h>

#include "audioserver.h"
#include "audiostream.h"
#include "class_registry.h"
#include <iostream>

#if defined( _WIN32)
	#ifndef WIN32_LEAN_AND_MEAN
		#define WIN32_LEAN_AND_MEAN
	#endif
	#include <objbase.h>
	#include <windows.h>
#else
	#include <unistd.h>
#endif

Napi::Object Init(Napi::Env env, Napi::Object exports)
{
  Napi::HandleScope scope(env);

	auto registry = new ClassRegistry();

	AudioServerWrap::Init(env, exports, registry);
	AudioStream::Init(env, exports, registry);
	// SoundioDeviceWrap::Init(env, exports, registry);
	// SoundioOutstreamWrap::Init(env, exports, registry);
	// SoundioInstreamWrap::Init(env, exports, registry);

	#if defined( _WIN32)
		CoInitializeEx(nullptr, COINIT_MULTITHREADED);
	#endif

	return exports;
}

NODE_API_MODULE(audioworklet, Init)
