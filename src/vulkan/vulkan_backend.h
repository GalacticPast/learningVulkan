#pragma once

#include "defines.h"
#include "vulkan_types.h"

struct platform_state;

b8 initialize_vulkan(struct platform_state *plat_state, vulkan_context *context, const char *applicaion_name);
b8 shutdown_vulkan(vulkan_context *context);
