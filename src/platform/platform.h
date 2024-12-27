#include "core/logger.h"
#include "defines.h"

typedef struct platform_state
{
    void *internal_state;
} platform_state;

b8 platform_startup(platform_state *plat_state, const char *application_name, s32 x, s32 y, s32 width, s32 height);
void platform_shutdown(platform_state *plat_state);
void platform_log_message(const char *buffer, log_levels level, u32 max_chars);
