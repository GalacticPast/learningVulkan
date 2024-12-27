#include "defines.h"

typedef struct platform_state
{
    void *internal_state;
} platform_state;

b8 platform_startup(platform_state *plat_state, const char *application_name, i32 x, i32 y, i32 width, i32 height);
void platform_shutdown(platform_state *plat_state);
