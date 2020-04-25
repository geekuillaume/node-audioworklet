#include <napi.h>

#include "soundio.h"
#include <iostream>

Napi::Object Init(Napi::Env env, Napi::Object exports)
{
	SoundioWrap::Init(env, exports);
	SoundioDevice::Init(env, exports);
	SoundioOutstream::Init(env, exports);
	SoundioInstream::Init(env, exports);

	return exports;
}

NODE_API_MODULE(audioworklet, Init)
