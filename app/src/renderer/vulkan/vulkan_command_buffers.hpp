#pragma once
#include "vulkan_types.hpp"

bool vulkan_create_graphics_command_pool(vulkan_context *vk_context);
bool vulkan_create_descriptor_command_pool_n_sets(vulkan_context *vk_context);
bool vulkan_create_command_buffer(vulkan_context *vk_context, VkCommandPool *command_pool);

void vulkan_begin_command_buffer_single_use(vulkan_context *vk_context, VkCommandBuffer command_buffer);
void vulkan_end_command_buffer_single_use(vulkan_context *vk_context, VkCommandBuffer command_buffer);
