#pragma once
#include "containers/darray.hpp"
#include "vulkan_types.hpp"

bool vulkan_create_buffer(vulkan_context *vk_context, vulkan_buffer *out_buffer, VkBufferUsageFlags usg_flags,
                          VkMemoryPropertyFlags memory_properties_flags, u64 buffer_size);
bool vulkan_copy_buffer(vulkan_context *vk_context, vulkan_buffer *dst_buffer, vulkan_buffer *src_buffer,
                        u64 buffer_size);
bool vulkan_destroy_buffer(vulkan_context *vk_context, vulkan_buffer *buffer);
bool vulkan_copy_data_to_buffer(vulkan_context *vk_context, vulkan_buffer *src_buffer, void *to_be_mapped_data,
                                void *to_be_copied_data, u32 to_be_copied_data_size);

bool vulkan_create_vertex_buffer(vulkan_context *vk_context);
bool vulkan_create_index_buffer(vulkan_context *vk_context);
bool vulkan_create_global_uniform_buffers(vulkan_context *vk_context);
