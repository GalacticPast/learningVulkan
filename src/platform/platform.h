#pragma once

#include "core/logger.h"
#include "defines.h"

struct vulkan_context;

typedef struct platform_state
{
    void *internal_state;
} platform_state;

b8 platform_startup(platform_state *plat_state, const char *application_name, s32 x, s32 y, s32 width, s32 height);
b8 platform_pump_messages(platform_state *plat_state);

void platform_shutdown(platform_state *plat_state);
void platform_log_message(const char *buffer, log_levels level, u32 max_chars);

b8    platform_create_vulkan_surface(platform_state *plat_state, struct vulkan_context *context);
void *platform_get_required_surface_extensions(char **required_extension_names);
