#pragma once

#include "defines.h"
#include "vulkan_types.h"

#define VK_CHECK(expr)                                                                                                                               \
    {                                                                                                                                                \
        ASSERT(expr == VK_SUCCESS);                                                                                                                  \
    }

b8 initialize_vulkan(vulkan_context *context, const char *applicaion_name);
b8 shutdown_vulkan(vulkan_context *context);
