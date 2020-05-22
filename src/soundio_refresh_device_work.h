#pragma once

#include <napi.h>

#include <chrono>
#include <thread>
#include <soundio/soundio.h>

#include "./soundio.h"

using namespace Napi;

// The refresh device operation is made in a different thread to prevent a dead lock situation
// when flushing events with soundio will block the main JS thread but will also wait a read_callback or write_callback to finish
// but this callback is waiting for the main JS thread
class RefreshDeviceWorker : public AsyncWorker {
    public:
        RefreshDeviceWorker(Function& callback, SoundioWrap* soundioWrap)
        : AsyncWorker(callback), soundioWrap(soundioWrap) {}

        ~RefreshDeviceWorker() {}

    // This code will be executed on the worker thread
    void Execute() override {
        soundioWrap->devicesInfoLock.lock();
        soundio_flush_events(soundioWrap->_soundio);

        // TODO we should unref device here to prevent memory leaks
        soundioWrap->rawOutputDevices.clear();
        soundioWrap->rawInputDevices.clear();

        unsigned int outputDeviceCount = soundio_output_device_count(soundioWrap->_soundio);
        unsigned int inputDeviceCount = soundio_input_device_count(soundioWrap->_soundio);

        for (int i = 0; i < outputDeviceCount; i += 1) {
          struct SoundIoDevice *device = soundio_get_output_device(soundioWrap->_soundio, i);
          if (!device->probe_error) {
            soundioWrap->rawOutputDevices.push_back(device);
          }
        }
        for (int i = 0; i < inputDeviceCount; i += 1) {
          struct SoundIoDevice *device = soundio_get_input_device(soundioWrap->_soundio, i);
          if (!device->probe_error) {
            soundioWrap->rawInputDevices.push_back(device);
          }
        }

        int defaultOutputDeviceIndex = soundio_default_output_device_index(soundioWrap->_soundio);
        if (defaultOutputDeviceIndex == -1) {
          soundioWrap->defaultOutputDevice = nullptr;
        } else {
          soundioWrap->defaultOutputDevice = soundio_get_output_device(soundioWrap->_soundio, defaultOutputDeviceIndex);
        }

        int defaultInputDeviceIndex = soundio_default_input_device_index(soundioWrap->_soundio);
        if (defaultInputDeviceIndex == -1) {
          soundioWrap->defaultInputDevice = nullptr;
        } else {
          soundioWrap->defaultInputDevice = soundio_get_input_device(soundioWrap->_soundio, defaultOutputDeviceIndex);
        }

        soundioWrap->hasBeenInitialized = true;
        soundioWrap->devicesInfoLock.unlock();
    }

    void OnOK() override {
        HandleScope scope(Env());
        // bool hasChanged = soundioWrap->_refreshDevices();
        Callback().Call({ Env().Null() });
    }

    private:
        SoundioWrap *soundioWrap;
};
