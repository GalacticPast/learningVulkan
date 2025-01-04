#pragma once
#include "logger.h"
#if _DEBUG
#ifdef PLATFORM_LINUX
#define debugbreak() __builtin_trap()
#endif

#ifdef PLATFORM_WINDOWS
#define debugbreak() __debugbreak()
#endif

#define ASSERT(expr)                                                                                                                                 \
    if (expr)                                                                                                                                        \
    {                                                                                                                                                \
    }                                                                                                                                                \
    else                                                                                                                                             \
    {                                                                                                                                                \
        FATAL("Assertion failure %s in file: %s at line: %d", #expr, __FILE__, __LINE__);                                                            \
        debugbreak();                                                                                                                                \
    }

#else

#define ASSERT(expr)

#endif
