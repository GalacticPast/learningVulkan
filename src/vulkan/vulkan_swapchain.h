#pragma once
#include "defines.h"
#include "platform/platform.h"
#include "vulkan_types.h"

b8 vulkan_create_swapchain(vulkan_context *context);
b8 vulkan_recreate_swapchain(vulkan_context *context);
