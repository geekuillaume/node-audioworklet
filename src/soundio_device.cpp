// #include "soundio_device.h"
// #include "soundio_outstream.h"
// #include "soundio_instream.h"

// void SoundioDeviceWrap::Init(Napi::Env &env, Napi::Object exports, ClassRegistry *registry)
// {
//   Napi::Function ctor_func =
//     DefineClass(env,
//       "SoundioDevice",
//       {
//         InstanceAccessor("name", &SoundioDeviceWrap::getName, nullptr, napi_enumerable),
//         InstanceAccessor("id", &SoundioDeviceWrap::getId, nullptr, napi_enumerable),
//         InstanceAccessor("formats", &SoundioDeviceWrap::getFormats, nullptr, napi_enumerable),
//         InstanceAccessor("sampleRates", &SoundioDeviceWrap::getSampleRates, nullptr, napi_enumerable),
//         InstanceAccessor("channelLayouts", &SoundioDeviceWrap::getChannelLayouts, nullptr, napi_enumerable),
//         InstanceAccessor("isInput", &SoundioDeviceWrap::getIsInput, nullptr, napi_enumerable),
//         InstanceAccessor("isOutput", &SoundioDeviceWrap::getIsOutput, nullptr, napi_enumerable),

//         InstanceMethod("openOutputStream", &SoundioDeviceWrap::openOutputStream),
//         InstanceMethod("openInputStream", &SoundioDeviceWrap::openInputStream),
//       }, registry);

//   // Set the class's ctor function as a persistent object to keep it in memory
//   registry->SoundioDeviceConstructor = Napi::Persistent(ctor_func);
//   registry->SoundioDeviceConstructor.SuppressDestruct();

//   exports.Set("SoundioDevice", ctor_func);
// }

// SoundioDeviceWrap::SoundioDeviceWrap(
// 	const Napi::CallbackInfo &info
// ) :
// 	Napi::ObjectWrap<SoundioDeviceWrap>(info)
// {
// 	registry = static_cast<ClassRegistry *>(info.Data());
//   _parentSoundioRef = Napi::Reference<Napi::Value>::New(info[0], 1); // this is used to prevent the GC to collect the parent soundio object while a device is still accessible
//   _device = info[1].As<Napi::External<SoundIoDevice>>().Data();

//   _isOutput = info[2].As<Napi::Boolean>();
//   _isInput = info[3].As<Napi::Boolean>();

//   soundio_device_ref(_device);
// }

// SoundioDeviceWrap::~SoundioDeviceWrap()
// {
//   soundio_device_unref(_device);
// }

// Napi::Value SoundioDeviceWrap::getName(const Napi::CallbackInfo &info)
// {
//   return Napi::String::New(info.Env(), _device->name);
// }

// Napi::Value SoundioDeviceWrap::getId(const Napi::CallbackInfo &info)
// {
//   return Napi::String::New(info.Env(), _device->id);
// }

// Napi::Value SoundioDeviceWrap::getFormats(const Napi::CallbackInfo &info)
// {
// 	Napi::Array formats = Napi::Array::New(info.Env(), _device->format_count);
// 	for (size_t i = 0; i < _device->format_count; i++)
// 	{
// 		formats[i] = Napi::Number::New(info.Env(), _device->formats[i]);
// 	}
//   return formats;
// }

// Napi::Value SoundioDeviceWrap::getSampleRates(const Napi::CallbackInfo &info)
// {
// 	Napi::Array sampleRates = Napi::Array::New(info.Env(), _device->sample_rate_count);
// 	for (size_t i = 0; i < _device->sample_rate_count; i++)
// 	{
// 		Napi::Object sampleRateRange = Napi::Object::New(info.Env());
// 		sampleRateRange.Set("min", _device->sample_rates[i].min);
// 		sampleRateRange.Set("max", _device->sample_rates[i].max);
// 		sampleRates[i] = sampleRateRange;
// 	}
//   return sampleRates;
// }

// Napi::Value SoundioDeviceWrap::getChannelLayouts(const Napi::CallbackInfo &info)
// {
// 	Napi::Array channelLayouts = Napi::Array::New(info.Env(), _device->layout_count);
// 	for (size_t i = 0; i < _device->layout_count; i++)
// 	{
// 		Napi::Object layout = Napi::Object::New(info.Env());
// 		layout.Set("name", _device->layouts[i].name);
// 		layout.Set("channelCount", _device->layouts[i].channel_count);
// 		channelLayouts[i] = layout;
// 	}
//   return channelLayouts;
// }

// Napi::Value SoundioDeviceWrap::getIsInput(const Napi::CallbackInfo &info)
// {
//   return Napi::Boolean::New(info.Env(), _isInput);
// }

// Napi::Value SoundioDeviceWrap::getIsOutput(const Napi::CallbackInfo &info)
// {
//   return Napi::Boolean::New(info.Env(), _isOutput);
// }

// Napi::Value SoundioDeviceWrap::openOutputStream(const Napi::CallbackInfo &info)
// {
//   if (!_isOutput) {
//     throw Napi::Error::New(info.Env(), "This is not an output device");
//   }
//   return registry->SoundioOutstreamConstructor.New({
//     info.This(),
//     Napi::External<SoundIoDevice>::New(info.Env(), _device),
//     info[0]
//   });
// }

// Napi::Value SoundioDeviceWrap::openInputStream(const Napi::CallbackInfo &info)
// {
//   if (!_isInput) {
//     throw Napi::Error::New(info.Env(), "This is not an input device");
//   }

//   return registry->SoundioInstreamConstructor.New({
//     info.This(),
//     Napi::External<SoundIoDevice>::New(info.Env(), _device),
//     info[0]
//   });
// }
