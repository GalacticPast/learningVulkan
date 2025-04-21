#pragma once
#include "vulkan_types.hpp"

bool vulkan_create_graphics_command_pool(vulkan_context *vk_context);
bool vulkan_create_descriptor_command_pool(vulkan_context *vk_context);
bool vulkan_update_descriptor_sets(vulkan_context *vk_context, vulkan_texture *texture);
