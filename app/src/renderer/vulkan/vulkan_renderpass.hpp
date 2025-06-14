#pragma once
#include "vulkan_types.hpp"

bool vulkan_create_renderpass(vulkan_context *vk_context);
bool vulkan_begin_frame_renderpass(vulkan_context *vk_context, VkCommandBuffer command_buffer, u32 image_index);
bool vulkan_end_frame_renderpass(VkCommandBuffer *command_buffer);
