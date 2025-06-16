#pragma once
#include "vulkan_types.hpp"

bool vulkan_create_renderpass(vulkan_context *vk_context);
bool vulkan_begin_renderpass(vulkan_context *vk_context,renderpass_types renderpass_type, VkCommandBuffer command_buffer, u32 image_index);
bool vulkan_end_renderpass(VkCommandBuffer *command_buffer);
