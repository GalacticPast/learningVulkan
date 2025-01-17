#pragma once

#include "defines.h"
#include "vulkan_types.h"

b8 vulkan_create_buffers(vulkan_context *context);
b8 vulkan_destroy_buffer(vulkan_context *context, vulkan_buffer *buffer);
