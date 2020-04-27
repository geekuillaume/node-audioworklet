#pragma once

#include <soundio/soundio.h>
#include <napi.h>
#include "class_registry.h"

class SoundioDeviceWrap : public Napi::ObjectWrap<SoundioDeviceWrap>
{
	public:
		static void Init(Napi::Env& env, Napi::Object exports, ClassRegistry *registry);

		SoundioDeviceWrap(const Napi::CallbackInfo &info);
		~SoundioDeviceWrap();

		Napi::Value getName(const Napi::CallbackInfo& info);
		Napi::Value getId(const Napi::CallbackInfo& info);
		Napi::Value getFormats(const Napi::CallbackInfo& info);
		Napi::Value getSampleRates(const Napi::CallbackInfo& info);
		Napi::Value getChannelLayouts(const Napi::CallbackInfo& info);

		Napi::Value getIsOutput(const Napi::CallbackInfo& info);
		Napi::Value getIsInput(const Napi::CallbackInfo& info);

		Napi::Value openOutputStream(const Napi::CallbackInfo &info);
		Napi::Value openInputStream(const Napi::CallbackInfo &info);

		SoundIoDevice *_device;
	private:
		bool _isInput;
		bool _isOutput;
		Napi::Reference<Napi::Value> _parentSoundioRef;
		ClassRegistry *registry;
};

