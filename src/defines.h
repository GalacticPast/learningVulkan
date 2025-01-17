#pragma once
#include <stdint.h>

typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef int8_t  s8;
typedef int16_t s16;
typedef int32_t s32;
typedef int64_t s64;

typedef float  f32;
typedef double f64;

// boolean since C doesnt have its own
typedef int8_t b8;

#define true 1
#define false 0

// compiler predermined platform detection macros
// https://stackoverflow.com/questions/430424/are-there-any-macros-to-determine-if-my-code-is-being-compiled-to-windows
// https://stackoverflow.com/questions/4605842/how-to-identify-platform-compiler-from-preprocessor-macros

#ifdef __linux__
#define PLATFORM_LINUX
#endif

#define UNUSED __attribute__((unused))
#define INLINE __attribute__((always_inline))

#define CLAMP(value, min, max) (value <= min) ? min : ((value >= max) ? max : value);
