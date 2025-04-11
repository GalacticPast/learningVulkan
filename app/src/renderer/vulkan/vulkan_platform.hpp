#pragma once
#include "defines.hpp"
#include "vulkan_types.hpp"
#include <vector>

bool vulkan_platform_get_required_vulkan_extensions(std::vector<const char *> &extensions_array);

bool vulkan_platform_create_surface(vulkan_context *vk_context);
