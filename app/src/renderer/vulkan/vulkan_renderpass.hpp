#pragma once
#include "vulkan_types.hpp"

bool vulkan_create_renderpass(vulkan_context *vk_context);
bool vulkan_begin_frame_renderpass(vulkan_context *vk_context, VkCommandBuffer command_buffer,
                                   struct render_data *render_data, u32 image_index);
