#include "vulkan_swapchain.h"
#include "containers/array.h"
#include "core/memory.h"
#include "platform/platform.h"
#include "vulkan_device.h"
#include "vulkan_frame_buffers.h"
#include "vulkan_image.h"

b8 create(vulkan_context *context);
b8 vulkan_cleanup_swapchain(vulkan_context *context);

b8 vulkan_create_swapchain(vulkan_context *context)
{
    return create(context);
}

b8 vulkan_recreate_swapchain(vulkan_context *context)
{
    vkDeviceWaitIdle(context->device.logical);

    if (!vulkan_cleanup_swapchain(context))
    {
        ERROR("vulkan swapchain cleanup failed");
        return false;
    }

    if (!create(context))
    {
        ERROR("Vulkan swapchain recreation failed");
        return false;
    }
    if (!vulkan_create_image_views(context))
    {
        ERROR("Vulkan image views creation failed");
        return false;
    }
    if (!vulkan_create_frame_buffers(context))
    {
        ERROR("Vulkan frame buffers recreation failed");
        return false;
    }
    return true;
}

b8 create(vulkan_context *context)
{
    vulkan_query_swapchain_support_details(context->device.physical, context->surface, &context->device.swapchain_support_details);

    VkSurfaceCapabilitiesKHR capabilities = context->device.swapchain_support_details.capabilities;

    VkExtent2D swap_extent = {};
    swap_extent.width      = context->frame_buffer_width;
    swap_extent.height     = context->frame_buffer_height;

    // clamp the values;
    swap_extent.width  = CLAMP(swap_extent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
    swap_extent.height = CLAMP(swap_extent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

    VkSurfaceFormatKHR surface_format;

    u32 format_count = context->device.swapchain_support_details.format_count;
    for (u32 i = 0; i < format_count; i++)
    {
        VkSurfaceFormatKHR available_format = context->device.swapchain_support_details.formats[i];
        if (available_format.format == VK_FORMAT_B8G8R8A8_SRGB && available_format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
        {
            surface_format = available_format;
            break;
        }
    }

    VkPresentModeKHR *present_mode = VK_NULL_HANDLE;

    u32 present_mode_count = context->device.swapchain_support_details.present_mode_count;
    for (u32 i = 0; i < present_mode_count; i++)
    {
        VkPresentModeKHR available_present_mode = context->device.swapchain_support_details.present_modes[i];
        if (available_present_mode == VK_PRESENT_MODE_MAILBOX_KHR)
        {
            present_mode = &available_present_mode;
            break;
        }
    }
    if (!present_mode)
    {
        present_mode = &context->device.swapchain_support_details.present_modes[0];
    }

    u32 image_count = capabilities.minImageCount + 1;

    if (capabilities.maxImageCount > 0 && image_count > capabilities.maxImageCount)
    {
        image_count = capabilities.maxImageCount;
    }

    VkSwapchainCreateInfoKHR swapchain_create_info = {};

    swapchain_create_info.sType            = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swapchain_create_info.pNext            = 0;
    swapchain_create_info.flags            = 0;
    swapchain_create_info.surface          = context->surface;
    swapchain_create_info.minImageCount    = image_count;
    swapchain_create_info.imageFormat      = surface_format.format;
    swapchain_create_info.imageColorSpace  = surface_format.colorSpace;
    swapchain_create_info.imageExtent      = swap_extent;
    swapchain_create_info.imageArrayLayers = 1;
    swapchain_create_info.imageUsage       = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    if (context->device.graphics_queue_index != context->device.present_queue_index)
    {
        swapchain_create_info.imageSharingMode      = VK_SHARING_MODE_CONCURRENT;
        swapchain_create_info.queueFamilyIndexCount = 2;
        u32 indicies[2]                             = {context->device.graphics_queue_index, context->device.present_queue_index};
        swapchain_create_info.pQueueFamilyIndices   = indicies;
    }
    else
    {
        swapchain_create_info.imageSharingMode      = VK_SHARING_MODE_EXCLUSIVE;
        swapchain_create_info.queueFamilyIndexCount = 0;
        swapchain_create_info.pQueueFamilyIndices   = 0;
    }

    swapchain_create_info.preTransform   = capabilities.currentTransform;
    swapchain_create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    swapchain_create_info.presentMode    = *present_mode;
    swapchain_create_info.clipped        = VK_TRUE;
    swapchain_create_info.oldSwapchain   = VK_NULL_HANDLE;

    VK_CHECK(vkCreateSwapchainKHR(context->device.logical, &swapchain_create_info, 0, &context->swapchain.handle));

    context->swapchain.format       = surface_format;
    context->swapchain.present_mode = *present_mode;
    context->swapchain.extent2D     = swap_extent;

    image_count = 0;
    VK_CHECK(vkGetSwapchainImagesKHR(context->device.logical, context->swapchain.handle, &image_count, 0));
    context->swapchain.images       = ALLOCATE_MEMORY_RENDERER(sizeof(VkImage) * image_count);
    context->swapchain.images_count = image_count;
    VK_CHECK(vkGetSwapchainImagesKHR(context->device.logical, context->swapchain.handle, &image_count, context->swapchain.images));

    return true;
}

b8 vulkan_cleanup_swapchain(vulkan_context *context)
{
    INFO("Destroying frame buffers...");
    u32 frame_buffers_count = context->swapchain.image_views_count;
    for (u32 i = 0; i < frame_buffers_count; i++)
    {
        vkDestroyFramebuffer(context->device.logical, context->swapchain.frame_buffers[i], 0);
    }

    INFO("Destroying image views...");
    u32 image_view_count = context->swapchain.image_views_count;
    for (u32 i = 0; i < image_view_count; i++)
    {
        vkDestroyImageView(context->device.logical, context->swapchain.image_views[i], 0);
    }
    INFO("Destroying vulkan swapchain...");
    vkDestroySwapchainKHR(context->device.logical, context->swapchain.handle, 0);

    return true;
}
