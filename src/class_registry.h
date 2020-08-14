#pragma once
#include <napi.h>

// This is used to store reference for the constructor of each class we use
// in this project so that we can instatiate new instance from C++ code.
// We cannot use static properties because this code is made to run in multiple
// context sharing the same memory (worker_threads) so we pass an instance of this
// object to each Init function of each class to register themself and pass a pointer
// to this class to their constructor function
// This should be simplified in NAPI v6 with these https://nodejs.org/api/n-api.html#n_api_environment_life_cycle_apis
class ClassRegistry {
  public:
    Napi::FunctionReference AudioServerConstructor;
    Napi::FunctionReference AudioStreamConstructor;
};
