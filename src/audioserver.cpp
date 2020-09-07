#include <cstdarg>
#include "audioserver.h"
#include "debug.h"

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
			InstanceMethod("outputLoopbackSupported", &AudioServerWrap::outputLoopbackSupported),

			StaticMethod("setDebugLog", &AudioServerWrap::setDebugLog),

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
	Napi::Object opts = info[0].As<Napi::Object>();
	int err;

	if (!opts.IsUndefined() && !opts.Get("onDeviceChange").IsUndefined()) {
		_deviceChangeHandler = Napi::ThreadSafeFunction::New(
			info.Env(),
			opts.Get("onDeviceChange").As<Napi::Function>(),
			"audioworkletDeviceChangeHandler",
			0,
			1,
			[this](Napi::Env) {
				_deviceChangeHandler = nullptr;
		});
		// let the process exit if there is only this thread safe function running
		_deviceChangeHandler.Unref(info.Env());
	}

	err = cubeb_init(&_cubeb, "AudioWorklet", NULL);
	if (err != CUBEB_OK) {
		throw Napi::Error::New(info.Env(), "Error while creating instance");
	}
	cubeb_register_device_collection_changed(_cubeb, (cubeb_device_type)(CUBEB_DEVICE_TYPE_INPUT | CUBEB_DEVICE_TYPE_OUTPUT), device_collection_changed_handler, this);
}

AudioServerWrap::~AudioServerWrap()
{
	debug_print("Audio server destroyed\n");
	cubeb_destroy(_cubeb);
}

void print_log(const char * msg, ...)
{
  va_list args;
  va_start(args, msg);
  vprintf(msg, args);
  va_end(args);
}

Napi::Value AudioServerWrap::setDebugLog(const Napi::CallbackInfo &info) {
	if (info[0].IsNull() || info[0].IsUndefined() || info[0].As<Napi::Boolean>().Value() == true) {
		cubeb_set_log_callback(CUBEB_LOG_VERBOSE, &print_log);
	}
	return info.Env().Undefined();
}


Napi::Object tranformCubebDevice(Napi::Env env, cubeb_device_info *cubebDevice) {
	Napi::Object device = Napi::Object::New(env);
	device.Set("id", Napi::String::New(env, cubebDevice->device_id));
	device.Set("name", Napi::String::New(env, cubebDevice->friendly_name ? cubebDevice->friendly_name : "unknown"));

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
	device.Set("state", Napi::String::New(env, cubebDevice->state == CUBEB_DEVICE_STATE_DISABLED ? "disabled" : cubebDevice->state == CUBEB_DEVICE_STATE_UNPLUGGED ? "unplugged" : "enabled"));
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
			if (devices.device[i].type != CUBEB_DEVICE_TYPE_OUTPUT) {
				throw Napi::Error::New(info.Env(), "This is an input device but required an output stream");
			}
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
	cubeb_device_collection devices;
	int err;

	bool isLoopback = !info[2].IsUndefined() && !info[2].IsNull() ? info[2].As<Napi::Boolean>().Value() : false;

	err = cubeb_enumerate_devices(_cubeb, isLoopback ? CUBEB_DEVICE_TYPE_OUTPUT : CUBEB_DEVICE_TYPE_INPUT, &devices);
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
				Napi::Boolean::New(info.Env(), true),
				info[1]
			});
			cubeb_device_collection_destroy(_cubeb, &devices);
			return ret;
		}
	}
	cubeb_device_collection_destroy(_cubeb, &devices);
	throw Napi::Error::New(info.Env(), "Unknown device id");
};

void device_collection_changed_handler(cubeb *context, void *user_ptr)
{
	AudioServerWrap *wrap = (AudioServerWrap *)user_ptr;

	if (wrap->_deviceChangeHandler) {
		wrap->_deviceChangeHandler.BlockingCall();
	}
}

Napi::Value AudioServerWrap::outputLoopbackSupported(const Napi::CallbackInfo& info)
{
	bool isSupported = !strcmp(cubeb_get_backend_id(_cubeb), "wasapi");
	return Napi::Boolean::New(info.Env(), isSupported);
}
