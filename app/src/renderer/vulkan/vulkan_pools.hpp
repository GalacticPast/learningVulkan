#pragma once
#include "vulkan_types.hpp"

bool vulkan_create_graphics_command_pool(vulkan_context *vk_context);
bool vulkan_create_descriptor_command_pools(vulkan_context *vk_context);
bool vulkan_update_global_descriptor_sets(vulkan_context *vk_context, u32 frame_index);
bool vulkan_update_materials_descriptor_set(vulkan_context *vk_context, material *material);
