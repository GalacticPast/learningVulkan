#pragma once

#if _DEBUG
#if PLATFORM_LINUX_X11 || PLATFORM_LINUX_WAYLAND
#define debugbreak() __builtin_trap()
#elif PLATFORM_WINDOWS
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
