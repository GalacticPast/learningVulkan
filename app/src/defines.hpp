#pragma once
#include <cstdint>

// Unsigned int types.
typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

// Signed int types.
typedef int8_t  s8;
typedef int16_t s16;
typedef int32_t s32;
typedef int64_t s64;

// Floating point types
typedef float  f32;
typedef double f64;

// Properly define static assertions.
#if defined(__clang__) || defined(__gcc__)
#define STATIC_ASSERT _Static_assert
#else
#define STATIC_ASSERT static_assert
#endif

/**
 * @brief Any id set to this should be considered invalid,
 * and not actually pointing to a real object.
 */
#define INVALID_ID 4294967295U
#define INVALID_ID_64 18446744073709551615LLU

// Platform detection
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__)
#define DPLATFORM_WINDOWS 1
#ifndef _WIN64
#error "64-bit is required on Windows!"
#endif
#elif defined(__linux__) || defined(__gnu_linux__)
// Linux OS
#define DPLATFORM_LINUX 1
#if defined(__ANDROID__)
#define DPLATFORM_ANDROID 1
#endif
#elif defined(__unix__)
// Catch anything not caught by the above.
#define DPLATFORM_UNIX 1
#elif defined(_POSIX_VERSION)
// Posix
#define DPLATFORM_POSIX 1
#elif __APPLE__
// Apple platforms
#define DPLATFORM_APPLE 1
#include <TargetConditionals.h>
#if TARGET_IPHONE_SIMULATOR
// iOS Simulator
#define DPLATFORM_IOS 1
#define DPLATFORM_IOS_SIMULATOR 1
#elif TARGET_OS_IPHONE
#define DPLATFORM_IOS 1
// iOS device
#elif TARGET_OS_MAC
// Other kinds of Mac OS
#else
#error "Unknown Apple platform"
#endif
#else
#error "Unknown platform!"
#endif

#ifdef DEXPORT
// Exports
#ifdef _MSC_VER
#define DAPI __declspec(dllexport)
#else
#define DAPI __attribute__((visibility("default")))
#endif
#else
// Imports
#ifdef _MSC_VER
#define DAPI __declspec(dllimport)
#else
#define DAPI
#endif
#endif

#define DCLAMP(value, min, max) (value <= min) ? min : (value >= max) ? max : value;
#define DMAX(a, b) (a < b) ? b : a;
#define DMIN(a, b) (a > b) ? b : a;

// Inlining
#if defined(__clang__) || defined(__gcc__)
#define DINLINE __attribute__((always_inline)) inline
#define DNOINLINE __attribute__((noinline))
#elif defined(_MSC_VER)
#define DINLINE __forceinline
#define DNOINLINE __declspec(noinline)
#else
#define DINLINE static inline
#define DNOINLINE
#endif

#define GB(num) (static_cast<u64>(num) * 1024 * 1024 * 1024ULL)
#define MB(num) (static_cast<u64>(num) * 1024 * 1024ULL)
#define KI(num) (static_cast<u64>(num) * 1024ULL)

#define DALIGN_UP(p, align) ((reinterpret_cast<uintptr_t>(p) + ((align) - 1)) & ~((align) - 1))
