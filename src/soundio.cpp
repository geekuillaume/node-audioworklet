#include "soundio.h"
#include "./soundio_refresh_device_work.h"

// #include <bits/stdc++.h>
// #include <sys/time.h>

void SoundioWrap::Init(Napi::Env &env, Napi::Object exports, ClassRegistry *registry)
{
	// Define the class and get it's ctor function
	Napi::Function ctor_func = DefineClass(env, "Soundio", {
			InstanceMethod("getApi", &SoundioWrap::getApi),
		//  InstanceMethod("getStreamSampleRate", &SoundioWrap::getStreamSampleRate),
			InstanceMethod("getDevices", &SoundioWrap::getDevices),
			InstanceMethod("getDefaultInputDevice", &SoundioWrap::getDefaultInputDevice),
			InstanceMethod("getDefaultOutputDevice", &SoundioWrap::getDefaultOutputDevice),
			InstanceMethod("refreshDevicesCb", &SoundioWrap::refreshDevicesCb),

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
	}, registry);

  // Set the class's ctor function as a persistent object to keep it in memory
  registry->SoundioConstructor = Napi::Persistent(ctor_func);
  registry->SoundioConstructor.SuppressDestruct();

	// Export the ctor
	exports.Set("Soundio", ctor_func);
}

SoundioWrap::SoundioWrap(
	const Napi::CallbackInfo &info
) :
	Napi::ObjectWrap<SoundioWrap>(info)
{
	registry = static_cast<ClassRegistry *>(info.Data());
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
}

SoundioWrap::~SoundioWrap()
{
	soundio_destroy(_soundio);
}

Napi::Value SoundioWrap::refreshDevicesCb(const Napi::CallbackInfo &info) {
	Napi::Function cb = info[0].As<Napi::Function>();

	RefreshDeviceWorker *wk = new RefreshDeviceWorker(cb, this);
	wk->Queue();
	return info.Env().Undefined();
}

bool SoundioWrap::_refreshDevices(const Napi::CallbackInfo &info)
{
	if (!hasBeenInitialized) {
		throw Napi::Error::New(info.Env(), "You need to refresh the devices first with refreshDevices()");
	}
	devicesInfoLock.lock();
	bool hasChanged = false;

	std::vector<SoundIoDevice *> presentOutputDevices = this->rawOutputDevices;
	std::vector<SoundIoDevice *> presentInputDevices = this->rawInputDevices;

	// first removing all devices that are not present anymore
	std::remove_if(_outputDevices.begin(), _outputDevices.end(), [&presentOutputDevices, &hasChanged](Napi::ObjectReference& wrapped) {
		SoundioDeviceWrap *device = Napi::ObjectWrap<SoundioDeviceWrap>::Unwrap(wrapped.Value());
		bool shouldBeRemoved = std::none_of(presentOutputDevices.begin(), presentOutputDevices.end(), [&device](SoundIoDevice *presentDevice) {
			return soundio_device_equal(device->_device, presentDevice);
		});
		if (shouldBeRemoved) {
			wrapped.Unref();
			hasChanged = true;
		}
		return shouldBeRemoved;
	});
	std::remove_if(_inputDevices.begin(), _inputDevices.end(), [&presentInputDevices, &hasChanged](Napi::ObjectReference& wrapped) {
		SoundioDeviceWrap *device = Napi::ObjectWrap<SoundioDeviceWrap>::Unwrap(wrapped.Value());
		bool shouldBeRemoved = std::none_of(presentInputDevices.begin(), presentInputDevices.end(), [&device](SoundIoDevice *presentDevice) {
			return soundio_device_equal(device->_device, presentDevice);
		});
		if (shouldBeRemoved) {
			wrapped.Unref();
			hasChanged = true;
		}
		return shouldBeRemoved;
	});

	// then adding devices that are new
	std::for_each(presentOutputDevices.begin(), presentOutputDevices.end(), [this, &info, &hasChanged](SoundIoDevice *presentDevice) {
		if (std::none_of(_outputDevices.begin(), _outputDevices.end(), [presentDevice](const Napi::ObjectReference& wrapped) {
			SoundioDeviceWrap *device = Napi::ObjectWrap<SoundioDeviceWrap>::Unwrap(wrapped.Value());
			return soundio_device_equal(device->_device, presentDevice);
		})) {
			auto newDevice = registry->SoundioDeviceConstructor.Value().New({
				info.This(),
				Napi::External<SoundIoDevice>::New(info.Env(), presentDevice),
				Napi::Boolean::New(info.Env(), true),
				Napi::Boolean::New(info.Env(), false)
			});
			hasChanged = true;
			_outputDevices.push_back(Napi::Persistent(newDevice));
		}
	});
	std::for_each(presentInputDevices.begin(), presentInputDevices.end(), [this, &info, &hasChanged](SoundIoDevice *presentDevice) {
		if (std::none_of(_inputDevices.begin(), _inputDevices.end(), [presentDevice](const Napi::ObjectReference& wrapped) {
			SoundioDeviceWrap *device = Napi::ObjectWrap<SoundioDeviceWrap>::Unwrap(wrapped.Value());
			return soundio_device_equal(device->_device, presentDevice);
		})) {
			auto newDevice = registry->SoundioDeviceConstructor.Value().New({
				info.This(),
				Napi::External<SoundIoDevice>::New(info.Env(), presentDevice),
				Napi::Boolean::New(info.Env(), false),
				Napi::Boolean::New(info.Env(), true)
			});
			hasChanged = true;
			_inputDevices.push_back(Napi::Persistent(newDevice));
		}
	});
	devicesInfoLock.unlock();

	return hasChanged;
}


Napi::Value SoundioWrap::getDevices(const Napi::CallbackInfo &info)
{
	_refreshDevices(info);

	// Converting to Napi::Array
	auto outputDevicesArray = Napi::Array::New(info.Env(), _outputDevices.size());
	auto inputDevicesArray = Napi::Array::New(info.Env(), _inputDevices.size());

	// Convert the devices to objects
	for (unsigned int i = 0; i < _outputDevices.size(); i++)
	{
		outputDevicesArray[i] = _outputDevices[i].Value();
	}
	for (unsigned int i = 0; i < _inputDevices.size(); i++)
	{
		inputDevicesArray[i] = _inputDevices[i].Value();
	}

	Napi::Object devices = Napi::Object::New(info.Env());
	devices.Set("outputDevices", outputDevicesArray);
	devices.Set("inputDevices", inputDevicesArray);

	return devices;
}

Napi::Value SoundioWrap::getDefaultOutputDevice(const Napi::CallbackInfo &info)
{
	_refreshDevices(info);

	if (defaultOutputDevice == nullptr) {
		return info.Env().Undefined();
	}

	for(std::vector<int>::size_type i = 0; i != _outputDevices.size(); i++) {
		SoundioDeviceWrap *wrappedDevice = Napi::ObjectWrap<SoundioDeviceWrap>::Unwrap(_outputDevices[i].Value());
		if (soundio_device_equal(defaultOutputDevice, wrappedDevice->_device)) {
			return _outputDevices[i].Value();
		}
	}

	return info.Env().Undefined();
}

Napi::Value SoundioWrap::getDefaultInputDevice(const Napi::CallbackInfo &info)
{
	_refreshDevices(info);

	if (defaultInputDevice == nullptr) {
		return info.Env().Undefined();
	}

	for(std::vector<int>::size_type i = 0; i != _inputDevices.size(); i++) {
		SoundioDeviceWrap *wrappedDevice = Napi::ObjectWrap<SoundioDeviceWrap>::Unwrap(_inputDevices[i].Value());
		if (soundio_device_equal(defaultInputDevice, wrappedDevice->_device)) {
			return _inputDevices[i].Value();
		}
	}

	return info.Env().Undefined();
}

Napi::Value SoundioWrap::getApi(const Napi::CallbackInfo &info)
{
	return Napi::String::New(info.Env(), soundio_backend_name(_soundio->current_backend));
}
