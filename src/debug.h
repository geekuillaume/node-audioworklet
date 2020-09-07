#pragma once
#include <iostream>


#ifndef NDEBUG
#define DEBUG_AUDIOWORKLET 1
#else
#define DEBUG_AUDIOWORKLET 0
#endif

#define debug_print(fmt, ...) \
        do { if (DEBUG_AUDIOWORKLET) fprintf(stderr, "%s:%d:%s(): " fmt, __FILE__, \
                                __LINE__, __func__, ##__VA_ARGS__); } while (0)
