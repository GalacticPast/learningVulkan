#include "containers/array.h"
#include "core/logger.h"
#include "vulkan_image.h"

b8 vulkan_create_image_views(vulkan_context *context)
{
    INFO("Creating Image Views...");
    u32 image_views_size = (u32)array_get_length(context->swapchain.images);
    context->image_views = array_create_with_capacity(VkImageView, image_views_size);
    array_set_length(context->image_views, image_views_size);

    for (u32 i = 0; i < image_views_size; i++)
    {
        VkImageViewCreateInfo image_view_create_info           = {};
        image_view_create_info.sType                           = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        image_view_create_info.pNext                           = 0;
        image_view_create_info.flags                           = 0;
        image_view_create_info.image                           = context->swapchain.images[i];
        image_view_create_info.viewType                        = VK_IMAGE_VIEW_TYPE_2D;
        image_view_create_info.format                          = context->swapchain.format.format;
        image_view_create_info.components.r                    = VK_COMPONENT_SWIZZLE_IDENTITY;
        image_view_create_info.components.g                    = VK_COMPONENT_SWIZZLE_IDENTITY;
        image_view_create_info.components.b                    = VK_COMPONENT_SWIZZLE_IDENTITY;
        image_view_create_info.components.a                    = VK_COMPONENT_SWIZZLE_IDENTITY;
        image_view_create_info.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
        image_view_create_info.subresourceRange.baseMipLevel   = 0;
        image_view_create_info.subresourceRange.levelCount     = 1;
        image_view_create_info.subresourceRange.baseArrayLayer = 0;
        image_view_create_info.subresourceRange.layerCount     = 1;

        VK_CHECK(vkCreateImageView(context->device.logical, &image_view_create_info, 0, &context->image_views[i]));
    }

    INFO("Image Views succesfully created");
    return true;
}
