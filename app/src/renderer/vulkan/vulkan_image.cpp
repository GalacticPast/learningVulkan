#include "vulkan_image.hpp"
#include "vulkan/vulkan_core.h"
#include "vulkan_buffers.hpp"
#include "vulkan_device.hpp"

bool vulkan_create_image(vulkan_context *vk_context, vulkan_image *out_image, u32 img_width, u32 img_height,
                         VkFormat img_format, VkMemoryPropertyFlags mem_properties_flags, VkImageUsageFlags img_usage,
                         VkImageTiling img_tiling)
{
    VkDevice &device = vk_context->vk_device.logical;

    VkImageCreateInfo image_create_info = {};

    image_create_info.sType         = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    image_create_info.imageType     = VK_IMAGE_TYPE_2D;
    image_create_info.extent.width  = img_width;
    image_create_info.extent.height = img_height;
    image_create_info.extent.depth  = 1; // TODO: Support configurable depth.
    image_create_info.mipLevels     = out_image->mip_levels;
    image_create_info.arrayLayers   = 1; // TODO: Support number of layers in the image.
    image_create_info.format        = img_format;
    image_create_info.tiling        = img_tiling;
    image_create_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    image_create_info.usage         = img_usage;
    image_create_info.samples       = VK_SAMPLE_COUNT_1_BIT;     // TODO: Configurable sample count.
    image_create_info.sharingMode   = VK_SHARING_MODE_EXCLUSIVE; // TODO: Configurable sharing mode.

    VkResult result = vkCreateImage(device, &image_create_info, vk_context->vk_allocator, &out_image->handle);
    VK_CHECK(result);

    VkMemoryRequirements img_mem_requirements{};
    vkGetImageMemoryRequirements(device, out_image->handle, &img_mem_requirements);

    VkMemoryAllocateInfo mem_alloc_info{};
    mem_alloc_info.sType          = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    mem_alloc_info.allocationSize = img_mem_requirements.size;
    mem_alloc_info.memoryTypeIndex =
        vulkan_find_memory_type_index(vk_context, img_mem_requirements.memoryTypeBits, mem_properties_flags);

    result = vkAllocateMemory(device, &mem_alloc_info, nullptr, &out_image->memory);
    VK_CHECK(result);

    vkBindImageMemory(device, out_image->handle, out_image->memory, 0);
    return true;
}

bool vulkan_create_image_view(vulkan_context *vk_context, VkImage *image, VkImageView *out_view, VkFormat img_format,
                              VkImageAspectFlags img_aspect_flags, u32 mip_levels)
{
    VkImageViewCreateInfo image_view_create_info{};

    image_view_create_info.sType                           = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    image_view_create_info.pNext                           = 0;
    image_view_create_info.flags                           = 0;
    image_view_create_info.image                           = *image;
    image_view_create_info.viewType                        = VK_IMAGE_VIEW_TYPE_2D;
    image_view_create_info.format                          = img_format;
    image_view_create_info.components.r                    = VK_COMPONENT_SWIZZLE_IDENTITY;
    image_view_create_info.components.g                    = VK_COMPONENT_SWIZZLE_IDENTITY;
    image_view_create_info.components.b                    = VK_COMPONENT_SWIZZLE_IDENTITY;
    image_view_create_info.components.a                    = VK_COMPONENT_SWIZZLE_IDENTITY;
    image_view_create_info.subresourceRange.aspectMask     = img_aspect_flags;
    image_view_create_info.subresourceRange.baseMipLevel   = 0;
    image_view_create_info.subresourceRange.levelCount     = mip_levels;
    image_view_create_info.subresourceRange.baseArrayLayer = 0;
    image_view_create_info.subresourceRange.layerCount     = 1;

    VkResult result =
        vkCreateImageView(vk_context->vk_device.logical, &image_view_create_info, vk_context->vk_allocator, out_view);
    VK_CHECK(result);
    return true;
}
bool vulkan_destroy_image(vulkan_context *vk_context, vulkan_image *image)
{
    VkDevice              &device    = vk_context->vk_device.logical;
    VkAllocationCallbacks *allocator = vk_context->vk_allocator;

    vkDestroyImage(device, image->handle, allocator);
    vkDestroyImageView(device, image->view, allocator);
    vkFreeMemory(device, image->memory, allocator);

    image->handle = 0;
    image->view   = 0;
    image->memory = 0;
    return true;
}
// NOTE: for now we use graphics command pool to allocate. Maybe we have to make it a function parameter??
bool vulkan_transition_image_layout(vulkan_context *vk_context, vulkan_image *image, VkImageLayout old_layout,
                                    VkImageLayout new_layout)
{
    VkCommandBuffer staging_cmd_buffer{};
    bool            result =
        vulkan_allocate_command_buffers(vk_context, &vk_context->graphics_command_pool, &staging_cmd_buffer, 1, true);
    if (!result)
    {
        DERROR("Vulkan command buffer allocation failed.");
        return false;
    }

    VkImageMemoryBarrier img_barrier{};

    img_barrier.sType                           = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    img_barrier.pNext                           = 0;
    img_barrier.oldLayout                       = old_layout;
    img_barrier.newLayout                       = new_layout;
    img_barrier.srcQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
    img_barrier.dstQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
    img_barrier.image                           = image->handle;
    img_barrier.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
    img_barrier.subresourceRange.baseMipLevel   = 0;
    img_barrier.subresourceRange.levelCount     = image->mip_levels;
    img_barrier.subresourceRange.baseArrayLayer = 0;
    img_barrier.subresourceRange.layerCount     = 1;

    VkPipelineStageFlags source_stage_flags;
    VkPipelineStageFlags destination_stage_flags;

    if (old_layout == VK_IMAGE_LAYOUT_UNDEFINED && new_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
    {
        img_barrier.srcAccessMask = 0;
        img_barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

        source_stage_flags      = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destination_stage_flags = VK_PIPELINE_STAGE_TRANSFER_BIT;
    }
    else if (old_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && new_layout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL)
    {
        img_barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        img_barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

        source_stage_flags      = VK_PIPELINE_STAGE_TRANSFER_BIT;
        destination_stage_flags = VK_PIPELINE_STAGE_TRANSFER_BIT;
    }
    else if (old_layout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL &&
             new_layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
    {
        img_barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        img_barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        source_stage_flags      = VK_PIPELINE_STAGE_TRANSFER_BIT;
        destination_stage_flags = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    }
    else
    {
        DERROR("Invalid image format");
        return false;
    }

    vkCmdPipelineBarrier(staging_cmd_buffer, source_stage_flags, destination_stage_flags, 0, 0, nullptr, 0, nullptr, 1,
                         &img_barrier);

    vulkan_end_command_buffer_single_use(vk_context, staging_cmd_buffer, false);

    vulkan_flush_command_buffer(vk_context, staging_cmd_buffer);

    vulkan_free_command_buffers(vk_context, &vk_context->graphics_command_pool, &staging_cmd_buffer, 1);
    return true;
}

bool vulkan_copy_buffer_data_to_image(vulkan_context *vk_context, vulkan_buffer *src_buffer, vulkan_image *image)
{
    VkCommandBuffer staging_command_buffer{};
    vulkan_allocate_command_buffers(vk_context, &vk_context->graphics_command_pool, &staging_command_buffer, 1, true);

    VkBufferImageCopy buffer_img_cpy_region{};
    buffer_img_cpy_region.bufferOffset      = 0;
    buffer_img_cpy_region.bufferRowLength   = 0;
    buffer_img_cpy_region.bufferImageHeight = 0;

    buffer_img_cpy_region.imageSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
    buffer_img_cpy_region.imageSubresource.mipLevel       = 0;
    buffer_img_cpy_region.imageSubresource.baseArrayLayer = 0;
    buffer_img_cpy_region.imageSubresource.layerCount     = 1;

    buffer_img_cpy_region.imageOffset = {0, 0, 0};
    buffer_img_cpy_region.imageExtent = {image->width, image->height, 1};

    vkCmdCopyBufferToImage(staging_command_buffer, src_buffer->handle, image->handle,
                           VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &buffer_img_cpy_region);

    vulkan_end_command_buffer_single_use(vk_context, staging_command_buffer, false);

    vulkan_flush_command_buffer(vk_context, staging_command_buffer);

    vulkan_free_command_buffers(vk_context, &vk_context->graphics_command_pool, &staging_command_buffer, 1);
    return true;
}

bool vulkan_generate_mipmaps(vulkan_context *vk_context, vulkan_image *image)
{
    VkFormatProperties format_properties{};
    vkGetPhysicalDeviceFormatProperties(vk_context->vk_device.physical, image->format, &format_properties);

    VkCommandBuffer staging_cmd_buffer{};
    vulkan_allocate_command_buffers(vk_context, &vk_context->graphics_command_pool, &staging_cmd_buffer, 1, true);

    if (!(format_properties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT))
    {
        DFATAL("texture image format does not support linear blitting!");
        return false;
    }

    s32 mipWidth  = image->width;
    s32 mipHeight = image->height;

    VkImageMemoryBarrier barrier{};
    barrier.sType                           = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.image                           = image->handle;
    barrier.srcQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
    barrier.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount     = 1;
    barrier.subresourceRange.levelCount     = 1;

    for (u32 i = 1; i < image->mip_levels; i++)
    {
        barrier.subresourceRange.baseMipLevel = i - 1;
        barrier.oldLayout                     = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.newLayout                     = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        barrier.srcAccessMask                 = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask                 = VK_ACCESS_TRANSFER_READ_BIT;

        vkCmdPipelineBarrier(staging_cmd_buffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0,
                             nullptr, 0, nullptr, 1, &barrier);

        VkImageBlit image_blit{};

        image_blit.srcOffsets[0]                 = {0, 0, 0};
        image_blit.srcOffsets[1]                 = {mipWidth, mipHeight, 1};
        image_blit.srcSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
        image_blit.srcSubresource.mipLevel       = i - 1;
        image_blit.srcSubresource.baseArrayLayer = 0;
        image_blit.srcSubresource.layerCount     = 1;

        image_blit.dstOffsets[0]             = {0, 0, 0};
        image_blit.dstOffsets[1]             = {mipWidth > 1 ? mipWidth / 2 : 1, mipHeight > 1 ? mipHeight / 2 : 1, 1};
        image_blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        image_blit.dstSubresource.mipLevel   = i;
        image_blit.dstSubresource.baseArrayLayer = 0;
        image_blit.dstSubresource.layerCount     = 1;

        vkCmdBlitImage(staging_cmd_buffer, image->handle, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, image->handle,
                       VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &image_blit, VK_FILTER_LINEAR);

        barrier.oldLayout     = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        barrier.newLayout     = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        vkCmdPipelineBarrier(staging_cmd_buffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                             0, 0, nullptr, 0, nullptr, 1, &barrier);

        if (mipWidth > 1)
            mipWidth /= 2;
        if (mipHeight > 1)
            mipHeight /= 2;
    }
    barrier.subresourceRange.baseMipLevel = image->mip_levels - 1;
    barrier.oldLayout                     = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.newLayout                     = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    barrier.srcAccessMask                 = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask                 = VK_ACCESS_SHADER_READ_BIT;

    vkCmdPipelineBarrier(staging_cmd_buffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
                         0, nullptr, 0, nullptr, 1, &barrier);

    vulkan_end_command_buffer_single_use(vk_context, staging_cmd_buffer, false);

    vulkan_flush_command_buffer(vk_context, staging_cmd_buffer);

    vulkan_free_command_buffers(vk_context, &vk_context->graphics_command_pool, &staging_cmd_buffer, 1);

    return true;
}
