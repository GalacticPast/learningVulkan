#pragma once
#include "containers/darray.hpp"
#include "defines.hpp"
#include "vulkan_types.hpp"

bool vulkan_platform_get_required_vulkan_extensions(darray<const char *> &extensions_array);

bool vulkan_platform_create_surface(vulkan_context *vk_context);
