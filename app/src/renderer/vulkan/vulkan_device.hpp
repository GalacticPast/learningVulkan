#pragma once
#include "vulkan_types.hpp"

bool vulkan_create_logical_device(vulkan_context *context);

// INFO: the surface_formats and presnt_modes will be malloced under the assumption that the caller will call free
void vulkan_device_query_swapchain_support(vulkan_context *vk_context, VkPhysicalDevice physical_device,
                                           vulkan_swapchain *out_swapchain);

u32 vulkan_find_memory_type_index(vulkan_context *vk_context, u32 mem_type_filter,
                                  VkMemoryPropertyFlags mem_properties_flags);

bool vulkan_find_suitable_depth_format(vulkan_device *device, vulkan_image *depth_image);

bool vulkan_create_sync_objects(vulkan_context *vk_context);
