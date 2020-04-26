#include <napi.h>

#include "soundio.h"
#include <iostream>

Napi::Object Init(Napi::Env env, Napi::Object exports)
{
	SoundioWrap::Init(env, exports);
	SoundioDeviceWrap::Init(env, exports);
	SoundioOutstreamWrap::Init(env, exports);
	SoundioInstreamWrap::Init(env, exports);

	return exports;
}

NODE_API_MODULE(audioworklet, Init)
