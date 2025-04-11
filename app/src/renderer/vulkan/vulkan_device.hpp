#pragma once
#include "vulkan_types.hpp"

bool vulkan_create_logical_device(vulkan_context *context);

// INFO: the surface_formats and presnt_modes will be malloced under the assumption that the caller will call free
void vulkan_device_query_swapchain_support(vulkan_context *vk_context, VkPhysicalDevice physical_device,
                                           vulkan_swapchain *out_swapchain);
