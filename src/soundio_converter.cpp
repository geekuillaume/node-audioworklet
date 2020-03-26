#include "soundio_converter.h"

Napi::Value SoundioConverter::ConvertDeviceInfo(Napi::Env env, SoundIoDevice *dev)
{
	Napi::Object devInfo = Napi::Object::New(env);

	// Set all properties in the object
	devInfo.Set("name", dev->name);
	devInfo.Set("id", dev->id);
	devInfo.Set("channels", dev->current_layout.channel_count);
	// devInfo.Set("sampleRates", ArrayFromVector(env, dev.sampleRates));

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
