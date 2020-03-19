#include <napi.h>

#include "rt_audio.h"

Napi::Object Init(Napi::Env env, Napi::Object exports)
{
	RtAudioWrap::Init(env, exports);

	return exports;
}

NODE_API_MODULE(audioworklet, Init)
