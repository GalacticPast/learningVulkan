#pragma once

#include "defines.h"
#include "vulkan_types.h"

b8 vulkan_create_command_buffers(vulkan_context *context);
b8 vulkan_create_command_pool(vulkan_context *context);
b8 vulkan_record_command_buffer(vulkan_context *context, u32 image_index);
