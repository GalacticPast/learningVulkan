#pragma once
#include "vulkan_types.hpp"

bool vulkan_create_graphics_command_pool(vulkan_context *vk_context);
bool vulkan_create_descriptor_command_pool_n_sets(vulkan_context *vk_context);

// I assume that the memory for the command buffers are already allocated when you call the belowfunction
bool vulkan_allocate_command_buffers(vulkan_context *vk_context, VkCommandPool *command_pool,
                                     VkCommandBuffer *cmd_buffers, u32 command_buffers_count, bool is_single_use);

void vulkan_begin_command_buffer_single_use(vulkan_context *vk_context, VkCommandBuffer command_buffer);
void vulkan_end_command_buffer_single_use(vulkan_context *vk_context, VkCommandBuffer command_buffer);
void vulkan_free_command_buffers(vulkan_context *vk_context, VkCommandPool *command_pool, VkCommandBuffer *cmd_buffers,
                                 u32 cmd_buffers_count);
