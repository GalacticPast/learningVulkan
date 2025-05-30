#pragma once
#include "containers/darray.hpp"
#include "vulkan_types.hpp"

bool vulkan_create_buffer(vulkan_context *vk_context, vulkan_buffer *out_buffer, VkBufferUsageFlags usg_flags,
                          VkMemoryPropertyFlags memory_properties_flags, u64 buffer_size);
bool vulkan_copy_buffer(vulkan_context *vk_context, vulkan_buffer *dst_buffer, u64 dst_offset,
                        vulkan_buffer *src_buffer, u64 buffer_size);
bool vulkan_destroy_buffer(vulkan_context *vk_context, vulkan_buffer *buffer);
bool vulkan_copy_data_to_buffer(vulkan_context *vk_context, vulkan_buffer *src_buffer, void *to_be_mapped_data,
                                void *to_be_copied_data, u32 to_be_copied_data_size);

bool vulkan_create_vertex_buffer(vulkan_context *vk_context);
bool vulkan_create_index_buffer(vulkan_context *vk_context);

//shader specific
bool vulkan_shader_create_per_frame_uniform_buffers(vulkan_context *vk_context, vulkan_shader* shader);

// I assume that the memory for the command buffers are already allocated when you call the belowfunction
bool vulkan_allocate_command_buffers(vulkan_context *vk_context, VkCommandPool *command_pool,
                                     VkCommandBuffer *cmd_buffers, u32 command_buffers_count, bool is_single_use);

void vulkan_begin_command_buffer_single_use(vulkan_context *vk_context, VkCommandBuffer command_buffer);
void vulkan_flush_command_buffer(vulkan_context *vk_context, VkCommandBuffer command_buffer);

void vulkan_end_command_buffer_single_use(vulkan_context *vk_context, VkCommandBuffer command_buffer, bool free);
void vulkan_free_command_buffers(vulkan_context *vk_context, VkCommandPool *command_pool, VkCommandBuffer *cmd_buffers,
                                 u32 cmd_buffers_count);
