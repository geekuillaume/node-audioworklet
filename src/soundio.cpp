#include "soundio.h"

// #include <bits/stdc++.h>
// #include <sys/time.h>

Napi::Object SoundioWrap::Init(Napi::Env env, Napi::Object exports)
{
	// Define the class and get it's ctor function
	Napi::Function ctor_func = DefineClass(env, "Soundio", {
			InstanceMethod("getApi", &SoundioWrap::getApi),
		//  InstanceMethod("getStreamSampleRate", &SoundioWrap::getStreamSampleRate),
			InstanceMethod("getDevices", &SoundioWrap::getDevices),
			InstanceMethod("getDefaultInputDevice", &SoundioWrap::getDefaultInputDevice),
			InstanceMethod("getDefaultOutputDevice", &SoundioWrap::getDefaultOutputDevice),

			StaticValue("SoundIoFormatS8", Napi::Number::New(env, SoundIoFormatS8)),        ///< Signed 8 bit
			StaticValue("SoundIoFormatU8", Napi::Number::New(env, SoundIoFormatU8)),        ///< Unsigned 8 bit
			StaticValue("SoundIoFormatS16LE", Napi::Number::New(env, SoundIoFormatS16LE)),     ///< Signed 16 bit Little Endian
			StaticValue("SoundIoFormatS16BE", Napi::Number::New(env, SoundIoFormatS16BE)),     ///< Signed 16 bit Big Endian
			StaticValue("SoundIoFormatU16LE", Napi::Number::New(env, SoundIoFormatU16LE)),     ///< Unsigned 16 bit Little Endian
			StaticValue("SoundIoFormatU16BE", Napi::Number::New(env, SoundIoFormatU16BE)),     ///< Unsigned 16 bit Big Endian
			StaticValue("SoundIoFormatS32LE", Napi::Number::New(env, SoundIoFormatS32LE)),     ///< Signed 32 bit Little Endian
			StaticValue("SoundIoFormatS32BE", Napi::Number::New(env, SoundIoFormatS32BE)),     ///< Signed 32 bit Big Endian
			StaticValue("SoundIoFormatU32LE", Napi::Number::New(env, SoundIoFormatU32LE)),     ///< Unsigned 32 bit Little Endian
			StaticValue("SoundIoFormatU32BE", Napi::Number::New(env, SoundIoFormatU32BE)),     ///< Unsigned 32 bit Big Endian
			StaticValue("SoundIoFormatFloat32LE", Napi::Number::New(env, SoundIoFormatFloat32LE)), ///< Float 32 bit Little Endian, Range -1.0 to 1.0
			StaticValue("SoundIoFormatFloat32BE", Napi::Number::New(env, SoundIoFormatFloat32BE)), ///< Float 32 bit Big Endian, Range -1.0 to 1.0
			StaticValue("SoundIoFormatFloat64LE", Napi::Number::New(env, SoundIoFormatFloat64LE)), ///< Float 64 bit Little Endian, Range -1.0 to 1.0
			StaticValue("SoundIoFormatFloat64BE", Napi::Number::New(env, SoundIoFormatFloat64BE)), ///< Float 64 bit Big Endian, Range -1.0 to 1.0
	});

	// Set the class's ctor function as a persistent object to keep it in memory
	constructor = Napi::Persistent(ctor_func);
	constructor.SuppressDestruct();

	// Export the ctor
	exports.Set("Soundio", ctor_func);
	return exports;
}

SoundioWrap::SoundioWrap(
	const Napi::CallbackInfo &info
) :
	Napi::ObjectWrap<SoundioWrap>(info)
{
	_ownRef = Napi::Reference<Napi::Value>::New(info.This()); // this is used to prevent the GC to collect this object while a stream is running
	int err;

	_soundio = soundio_create();
	if (!_soundio) {
		throw Napi::Error::New(info.Env(), "Error while creating soundio instance");
	}

	err = soundio_connect(_soundio);
	if (err) {
		throw Napi::Error::New(info.Env(), soundio_strerror(err));
	}
	soundio_flush_events(_soundio);
}

SoundioWrap::~SoundioWrap()
{
	soundio_destroy(_soundio);
}

Napi::Value SoundioWrap::getDevices(const Napi::CallbackInfo &info)
{
	Napi::Array outputDevicesArray;
	Napi::Array inputDevicesArray;
	std::vector<SoundIoDevice *> outputDevices;
	std::vector<SoundIoDevice *> inputDevices;

	unsigned int outputDeviceCount = soundio_output_device_count(_soundio);
	unsigned int inputDeviceCount = soundio_input_device_count(_soundio);

	for (int i = 0; i < outputDeviceCount; i += 1) {
		struct SoundIoDevice *device = soundio_get_output_device(_soundio, i);
		if (!device->probe_error) {
			outputDevices.push_back(device);
		}
	}
	for (int i = 0; i < inputDeviceCount; i += 1) {
		struct SoundIoDevice *device = soundio_get_input_device(_soundio, i);
		if (!device->probe_error) {
			inputDevices.push_back(device);
		}
	}

	// Allocate the devices array
	outputDevicesArray = Napi::Array::New(info.Env(), outputDevices.size());
	inputDevicesArray = Napi::Array::New(info.Env(), inputDevices.size());

	// Convert the devices to objects
	for (unsigned int i = 0; i < outputDevices.size(); i++)
	{
		outputDevicesArray[i] = SoundioDevice::constructor.New({ Napi::External<SoundIoDevice>::New(info.Env(), outputDevices[i]), Napi::Boolean::New(info.Env(), true), Napi::Boolean::New(info.Env(), false) });
		soundio_device_unref(outputDevices[i]);
	}
	for (unsigned int i = 0; i < inputDevices.size(); i++)
	{
		inputDevicesArray[i] = SoundioDevice::constructor.New({ Napi::External<SoundIoDevice>::New(info.Env(), inputDevices[i]), Napi::Boolean::New(info.Env(), false), Napi::Boolean::New(info.Env(), true) });
		soundio_device_unref(inputDevices[i]);
	}

	Napi::Object devices = Napi::Object::New(info.Env());
	devices.Set("outputDevices", outputDevicesArray);
	devices.Set("inputDevices", inputDevicesArray);

	return devices;
}

Napi::Value SoundioWrap::getDefaultOutputDevice(const Napi::CallbackInfo &info)
{
	int defaultDeviceIndex = soundio_default_output_device_index(_soundio);
	struct SoundIoDevice *device = soundio_get_output_device(_soundio, defaultDeviceIndex);

	return SoundioDevice::constructor.New({ Napi::External<SoundIoDevice>::New(info.Env(), device), Napi::Boolean::New(info.Env(), true), Napi::Boolean::New(info.Env(), false) });;
}

Napi::Value SoundioWrap::getDefaultInputDevice(const Napi::CallbackInfo &info)
{
	int defaultDeviceIndex = soundio_default_input_device_index(_soundio);
	struct SoundIoDevice *device = soundio_get_input_device(_soundio, defaultDeviceIndex);

	return SoundioDevice::constructor.New({ Napi::External<SoundIoDevice>::New(info.Env(), device), Napi::Boolean::New(info.Env(), false), Napi::Boolean::New(info.Env(), true) });;
}

Napi::Value SoundioWrap::getApi(const Napi::CallbackInfo &info)
{
	return Napi::String::New(info.Env(), soundio_backend_name(_soundio->current_backend));
}
