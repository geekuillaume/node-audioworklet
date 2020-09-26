#pragma once

#include <napi.h>
#include <assert.h>
#include <stdint.h>
#include "cubeb/cubeb.h"

size_t cubeb_sample_size(cubeb_sample_format format)
{
  switch (format) {
    case CUBEB_SAMPLE_S16LE:
    case CUBEB_SAMPLE_S16BE:
      return sizeof(int16_t);
    case CUBEB_SAMPLE_FLOAT32LE:
    case CUBEB_SAMPLE_FLOAT32BE:
      return sizeof(float);
    default:
      // should never happen as all cases are handled above.
      assert(false);
      return 0;
  }
}

napi_typedarray_type format_to_typedarray_type(cubeb_sample_format format)
{
  switch (format) {
    case CUBEB_SAMPLE_S16LE:
    case CUBEB_SAMPLE_S16BE:
      return napi_int16_array;
    case CUBEB_SAMPLE_FLOAT32LE:
    case CUBEB_SAMPLE_FLOAT32BE:
      return napi_float32_array;
    default:
      // should never happen as all cases are handled above.
      assert(false);
      return napi_int16_array;
  }

}
