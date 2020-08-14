#include "audioserver.h"

// #include <bits/stdc++.h>
// #include <sys/time.h>

void AudioServerWrap::Init(Napi::Env &env, Napi::Object exports, ClassRegistry *registry)
{
	// Define the class and get it's ctor function
	Napi::Function ctor_func = DefineClass(env, "AudioServer", {
			InstanceMethod("getApi", &AudioServerWrap::getApi),
			InstanceMethod("getDevices", &AudioServerWrap::getDevices),
			InstanceMethod("initInputStream", &AudioServerWrap::initInputStream),
			InstanceMethod("initOutputStream", &AudioServerWrap::initOutputStream),

			StaticValue("S16LE", Napi::Number::New(env, CUBEB_SAMPLE_S16LE)),
			StaticValue("S16BE", Napi::Number::New(env, CUBEB_SAMPLE_S16BE)),
			StaticValue("F32LE", Napi::Number::New(env, CUBEB_SAMPLE_FLOAT32LE)),
			StaticValue("F32BE", Napi::Number::New(env, CUBEB_SAMPLE_FLOAT32BE)),
	}, registry);

  // Set the class's ctor function as a persistent object to keep it in memory
  registry->AudioServerConstructor = Napi::Persistent(ctor_func);
  registry->AudioServerConstructor.SuppressDestruct();

	// Export the ctor
	exports.Set("AudioServer", ctor_func);
}

AudioServerWrap::AudioServerWrap(
	const Napi::CallbackInfo &info
) :
	Napi::ObjectWrap<AudioServerWrap>(info)
{
	registry = static_cast<ClassRegistry *>(info.Data());
	_ownRef = Napi::Reference<Napi::Value>::New(info.This()); // this is used to prevent the GC to collect this object while a stream is running
	int err;

	err = cubeb_init(&_cubeb, "AudioWorklet", NULL);
	if (err != CUBEB_OK) {
		throw Napi::Error::New(info.Env(), "Error while creating instance");
	}
}

AudioServerWrap::~AudioServerWrap()
{
	cubeb_destroy(_cubeb);
}

Napi::Object tranformCubebDevice(Napi::Env env, cubeb_device_info *cubebDevice) {
	Napi::Object device = Napi::Object::New(env);
	device.Set("id", Napi::String::New(env, cubebDevice->device_id));
	device.Set("name", Napi::String::New(env, cubebDevice->friendly_name));

	Napi::Object preferred = Napi::Object::New(env);
	preferred.Set("multimedia", Napi::Boolean::New(env, cubebDevice->preferred & CUBEB_DEVICE_PREF_MULTIMEDIA));
	preferred.Set("voice", Napi::Boolean::New(env, cubebDevice->preferred & CUBEB_DEVICE_PREF_VOICE));
	preferred.Set("notification", Napi::Boolean::New(env, cubebDevice->preferred & CUBEB_DEVICE_PREF_NOTIFICATION));
	preferred.Set("all", Napi::Boolean::New(env, cubebDevice->preferred & CUBEB_DEVICE_PREF_ALL));
	device.Set("preferred", preferred);

	Napi::Object supportedFormats = Napi::Object::New(env);
	supportedFormats.Set("S16LE", Napi::Boolean::New(env, cubebDevice->format & CUBEB_DEVICE_FMT_S16LE));
	supportedFormats.Set("S16BE", Napi::Boolean::New(env, cubebDevice->format & CUBEB_DEVICE_FMT_S16BE));
	supportedFormats.Set("F32LE", Napi::Boolean::New(env, cubebDevice->format & CUBEB_DEVICE_FMT_F32LE));
	supportedFormats.Set("F32BE", Napi::Boolean::New(env, cubebDevice->format & CUBEB_DEVICE_FMT_F32BE));
	device.Set("supportedFormats", supportedFormats);

	device.Set("minRate", Napi::Number::New(env, cubebDevice->min_rate));
	device.Set("maxRate", Napi::Number::New(env, cubebDevice->max_rate));
	device.Set("defaultRate", Napi::Number::New(env, cubebDevice->default_rate));
	device.Set("maxChannels", Napi::Number::New(env, cubebDevice->max_channels));
	device.Set("minLatency", Napi::Number::New(env, cubebDevice->latency_lo));
	device.Set("maxLatency", Napi::Number::New(env, cubebDevice->latency_hi));
	device.Set("groupId", Napi::String::New(env, cubebDevice->group_id ? cubebDevice->group_id : ""));
	device.Set("defaultFormat", Napi::Number::New(env, cubebDevice->default_format));
	return device;
}

Napi::Value AudioServerWrap::getDevices(const Napi::CallbackInfo &info)
{
	cubeb_device_collection inputDevices;
	cubeb_device_collection outputDevices;
	int err;

	err = cubeb_enumerate_devices(_cubeb, CUBEB_DEVICE_TYPE_OUTPUT, &outputDevices);
	if (err != CUBEB_OK) {
		throw Napi::Error::New(info.Env(), "Error while enumerating devices");
	}
	err = cubeb_enumerate_devices(_cubeb, CUBEB_DEVICE_TYPE_INPUT, &inputDevices);
	if (err != CUBEB_OK) {
		throw Napi::Error::New(info.Env(), "Error while enumerating devices");
	}

	auto inputDevicesArray = Napi::Array::New(info.Env(), inputDevices.count);
	auto outputDevicesArray = Napi::Array::New(info.Env(), outputDevices.count);

	// Convert the devices to objects
	for (unsigned int i = 0; i < outputDevices.count; i++)
	{
		outputDevicesArray[i] = tranformCubebDevice(info.Env(), &outputDevices.device[i]);
	}

	for (unsigned int i = 0; i < inputDevices.count; i++)
	{
		inputDevicesArray[i] = tranformCubebDevice(info.Env(), &inputDevices.device[i]);
	}

	Napi::Object devices = Napi::Object::New(info.Env());
	devices.Set("outputDevices", outputDevicesArray);
	devices.Set("inputDevices", inputDevicesArray);

	cubeb_device_collection_destroy(_cubeb, &inputDevices);
	cubeb_device_collection_destroy(_cubeb, &outputDevices);

	return devices;
}

Napi::Value AudioServerWrap::getApi(const Napi::CallbackInfo &info)
{
	return Napi::String::New(info.Env(), cubeb_get_backend_id(_cubeb));
}

Napi::Value AudioServerWrap::initOutputStream(const Napi::CallbackInfo& info)
{
	cubeb_device_collection devices;
	int err;

	err = cubeb_enumerate_devices(_cubeb, CUBEB_DEVICE_TYPE_OUTPUT, &devices);
	if (err != CUBEB_OK) {
		throw Napi::Error::New(info.Env(), "Error while enumerating devices");
	}

	for (unsigned int i = 0; i < devices.count; i++)
	{
		if (info[0].As<Napi::String>().Utf8Value().compare(devices.device[i].device_id) == 0) {
			Napi::Value ret = registry->AudioStreamConstructor.New({
				info.This(),
				Napi::External<cubeb>::New(info.Env(), _cubeb),
				Napi::External<cubeb_device_info>::New(info.Env(), &devices.device[i]),
				Napi::Boolean::New(info.Env(), false),
				info[1]
			});
			cubeb_device_collection_destroy(_cubeb, &devices);
			return ret;
		}
	}
	cubeb_device_collection_destroy(_cubeb, &devices);
	throw Napi::Error::New(info.Env(), "Unknown device id");
};

Napi::Value AudioServerWrap::initInputStream(const Napi::CallbackInfo& info)
{
	return registry->AudioStreamConstructor.New({
    info.This(),
    info[0]
  });
};
