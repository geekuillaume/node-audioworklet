#include "soundio_converter.h"

Napi::Value SoundioConverter::ConvertDeviceInfo(Napi::Env env, SoundIoDevice *dev)
{
	Napi::Object devInfo = Napi::Object::New(env);

	// Set all properties in the object
	devInfo.Set("name", dev->name);
	devInfo.Set("id", dev->id);

	Napi::Array formats = Napi::Array::New(env, dev->format_count);
	for (size_t i = 0; i < dev->format_count; i++)
	{
		formats[i] = Napi::Number::New(env, dev->formats[i]);
	}
	devInfo.Set("formats", formats);

	Napi::Array sampleRates = Napi::Array::New(env, dev->sample_rate_count);
	for (size_t i = 0; i < dev->sample_rate_count; i++)
	{
		Napi::Object sampleRateRange = Napi::Object::New(env);
		sampleRateRange.Set("min", dev->sample_rates[i].min);
		sampleRateRange.Set("max", dev->sample_rates[i].max);
		sampleRates[i] = sampleRateRange;
	}
	devInfo.Set("sampleRates", sampleRates);

	Napi::Array channelLayouts = Napi::Array::New(env, dev->layout_count);
	for (size_t i = 0; i < dev->layout_count; i++)
	{
		Napi::Object layout = Napi::Object::New(env);
		layout.Set("name", dev->layouts[i].name);
		layout.Set("channelCount", dev->layouts[i].channel_count);
		channelLayouts[i] = layout;
	}
	devInfo.Set("channelLayouts", channelLayouts);

	return devInfo;
}

// RtAudio::StreamParameters RtAudioConverter::ConvertStreamParameters(Napi::Object obj)
// {
// 	RtAudio::StreamParameters params;

// 	// Set default device id
// 	if (!obj.Has("deviceId")) {
// 		obj.Set("deviceId", 0);
// 	}

// 	// Set default first channel
// 	if (!obj.Has("firstChannel")) {
// 		obj.Set("firstChannel", 0);
// 	}

// 	// Check for valid device id
// 	if (!obj.Get("deviceId").IsNumber())
// 	{
// 		throw Napi::Error::New(obj.Env(), "The 'device id' stream parameter is missing or invalid!");
// 	}

// 	// Check for valid no. of channels
// 	if (!obj.Has("nChannels") || !obj.Get("nChannels").IsNumber())
// 	{
// 		throw Napi::Error::New(obj.Env(), "The 'no. of channels' stream parameter is missing or invalid!");
// 	}

// 	// Check for valid first channel
// 	if (!obj.Get("firstChannel").IsNumber())
// 	{
// 		throw Napi::Error::New(obj.Env(), "The 'first channel' stream parameter is missing or invalid!");
// 	}

// 	// Get the properties from the object
// 	params.deviceId = obj.Get("deviceId").As<Napi::Number>();
// 	params.nChannels = obj.Get("nChannels").As<Napi::Number>();
// 	params.firstChannel = obj.Get("firstChannel").As<Napi::Number>();

// 	return params;
// }
