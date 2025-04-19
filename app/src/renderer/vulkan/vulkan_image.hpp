#pragma once
#include "vulkan_types.hpp"

bool vulkan_create_image(vulkan_context *vk_context, vulkan_image *out_image, u32 img_width, u32 img_height,
                         VkFormat img_format, VkMemoryPropertyFlags mem_properties_flags, VkImageUsageFlags img_usage,
                         VkImageTiling img_tiling);

bool vulkan_create_image_view(vulkan_context *vk_context, VkImage image, VkFormat format, VkImageView *out_view,
                              VkImageAspectFlags img_aspect_flags);
