#pragma once
#include "vulkan_types.hpp"

bool vulkan_create_renderpass(vulkan_context *vk_context);
bool vulkan_begin_frame_renderpass(vulkan_context *vk_context, VkCommandBuffer command_buffer,
                                   vulkan_geometry_data *geo_data, u32 image_index);
