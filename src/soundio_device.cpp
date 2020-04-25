#include "soundio.h"

void SoundioDevice::Init(Napi::Env &env, Napi::Object exports)
{
  Napi::HandleScope scope(env);
  Napi::Function ctor =
    DefineClass(env,
      "SoundioDevice",
      {
        InstanceAccessor("name", &SoundioDevice::getName, nullptr, napi_enumerable),
        InstanceAccessor("id", &SoundioDevice::getId, nullptr, napi_enumerable),
        InstanceAccessor("formats", &SoundioDevice::getFormats, nullptr, napi_enumerable),
        InstanceAccessor("sampleRates", &SoundioDevice::getSampleRates, nullptr, napi_enumerable),
        InstanceAccessor("channelLayouts", &SoundioDevice::getChannelLayouts, nullptr, napi_enumerable),
        InstanceAccessor("isInput", &SoundioDevice::getIsInput, nullptr, napi_enumerable),
        InstanceAccessor("isOutput", &SoundioDevice::getIsOutput, nullptr, napi_enumerable),

        InstanceMethod("openOutputStream", &SoundioDevice::openOutputStream),
      });
  constructor = Napi::Persistent(ctor);
  constructor.SuppressDestruct();

  exports.Set("SoundioDevice", ctor);
}

SoundioDevice::SoundioDevice(
	const Napi::CallbackInfo &info
) :
	Napi::ObjectWrap<SoundioDevice>(info)
{
	_ownRef = Napi::Reference<Napi::Value>::New(info.This()); // this is used to prevent the GC to collect this object while a stream is running
  _device = info[0].As<Napi::External<SoundIoDevice>>().Data();

  _isOutput = info[1].As<Napi::Boolean>();
  _isInput = info[2].As<Napi::Boolean>();

  soundio_device_ref(_device);
}

SoundioDevice::~SoundioDevice()
{
  soundio_device_unref(_device);
}

Napi::Value SoundioDevice::getName(const Napi::CallbackInfo &info)
{
  return Napi::String::New(info.Env(), _device->name);
}

Napi::Value SoundioDevice::getId(const Napi::CallbackInfo &info)
{
  return Napi::String::New(info.Env(), _device->id);
}

Napi::Value SoundioDevice::getFormats(const Napi::CallbackInfo &info)
{
	Napi::Array formats = Napi::Array::New(info.Env(), _device->format_count);
	for (size_t i = 0; i < _device->format_count; i++)
	{
		formats[i] = Napi::Number::New(info.Env(), _device->formats[i]);
	}
  return formats;
}

Napi::Value SoundioDevice::getSampleRates(const Napi::CallbackInfo &info)
{
	Napi::Array sampleRates = Napi::Array::New(info.Env(), _device->sample_rate_count);
	for (size_t i = 0; i < _device->sample_rate_count; i++)
	{
		Napi::Object sampleRateRange = Napi::Object::New(info.Env());
		sampleRateRange.Set("min", _device->sample_rates[i].min);
		sampleRateRange.Set("max", _device->sample_rates[i].max);
		sampleRates[i] = sampleRateRange;
	}
  return sampleRates;
}

Napi::Value SoundioDevice::getChannelLayouts(const Napi::CallbackInfo &info)
{
	Napi::Array channelLayouts = Napi::Array::New(info.Env(), _device->layout_count);
	for (size_t i = 0; i < _device->layout_count; i++)
	{
		Napi::Object layout = Napi::Object::New(info.Env());
		layout.Set("name", _device->layouts[i].name);
		layout.Set("channelCount", _device->layouts[i].channel_count);
		channelLayouts[i] = layout;
	}
  return channelLayouts;
}

Napi::Value SoundioDevice::getIsInput(const Napi::CallbackInfo &info)
{
  return Napi::Boolean::New(info.Env(), _isInput);
}

Napi::Value SoundioDevice::getIsOutput(const Napi::CallbackInfo &info)
{
  return Napi::Boolean::New(info.Env(), _isOutput);
}

Napi::Value SoundioDevice::openOutputStream(const Napi::CallbackInfo &info)
{
	Napi::Object opts = info[0].IsUndefined() ? Napi::Object::New(info.Env()) : info[0].As<Napi::Object>();

  if (!_isOutput) {
    throw Napi::Error::New(info.Env(), "This is not an output device");
  }

  return SoundioOutstream::constructor.New({
    Napi::External<SoundIoDevice>::New(info.Env(), _device),
    info[0]
  });

	_ownRef.Ref();
}
