#pragma once
#include "vulkan_types.hpp"

bool vulkan_create_graphics_command_pool(vulkan_context *vk_context);
bool vulkan_create_command_buffer(vulkan_context *vk_context, VkCommandPool *command_pool);
void vulkan_record_command_buffer_and_use(vulkan_context *vk_context, VkCommandBuffer command_buffer,
                                          struct render_data *render_data, u32 image_index);

bool vulkan_create_descriptor_command_pool_n_sets(vulkan_context *vk_context);
