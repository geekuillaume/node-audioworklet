#include <napi.h>

#include "soundio.h"

Napi::Object Init(Napi::Env env, Napi::Object exports)
{
	SoundioWrap::Init(env, exports);

	return exports;
}

NODE_API_MODULE(audioworklet, Init)
