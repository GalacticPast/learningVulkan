#pragma once
#include "vulkan_types.hpp"

bool vulkan_create_image(vulkan_context *vk_context, vulkan_image *out_image, u32 img_width, u32 img_height,
                         VkFormat img_format, VkMemoryPropertyFlags mem_properties_flags, VkImageUsageFlags img_usage,
                         VkImageTiling img_tiling);

bool vulkan_create_image_view(vulkan_context *vk_context, VkImage *image, VkImageView *out_view, VkFormat img_format,
                              VkImageAspectFlags img_aspect_flags);

bool vulkan_destroy_image(vulkan_context *vk_context, vulkan_image *image);

bool vulkan_copy_buffer_data_to_image(vulkan_context *vk_context, vulkan_buffer *src_buffer, vulkan_image *image);
bool vulkan_transition_image_layout(vulkan_context *vk_context, vulkan_image *image, VkImageLayout old_layout,
                                    VkImageLayout new_layout);
