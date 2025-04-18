#include "vulkan_swapchain.hpp"
#include "core/dmemory.hpp"
#include "core/logger.hpp"
#include "defines.hpp"
#include "platform/platform.hpp"
#include "vulkan_device.hpp"
#include "vulkan_framebuffer.hpp"
#include "vulkan_image.hpp"

bool create_swapchain(vulkan_context *vk_context)
{
    // INFO: query swapchain support
    vulkan_device_query_swapchain_support(vk_context, vk_context->vk_device.physical, &vk_context->vk_swapchain);

    vulkan_swapchain *vk_swapchain = &vk_context->vk_swapchain;

    u32        best_surface_format_index = INVALID_ID;
    u32        best_present_mode_index   = INVALID_ID;
    VkExtent2D best_2d_extent;
    // choose the best surface format

    for (u32 i = 0; i < vk_swapchain->surface_formats_count; i++)
    {
        if (vk_swapchain->surface_formats[i].format == VK_FORMAT_B8G8R8A8_SRGB &&
            vk_swapchain->surface_formats[i].colorSpace == VK_COLORSPACE_SRGB_NONLINEAR_KHR)
        {
            best_surface_format_index = i;
            break;
        }
    }
    if (best_surface_format_index == INVALID_ID)
    {
        DERROR("Compatible surface format not found. Choosing the first one");
        best_surface_format_index = 0;
    }

    // choose the best present mode
    for (u32 i = 0; i < vk_swapchain->present_modes_count; i++)
    {
        if (vk_swapchain->present_modes[i] == VK_PRESENT_MODE_MAILBOX_KHR)
        {
            best_present_mode_index = i;
            break;
        }
    }
    if (best_present_mode_index == INVALID_ID)
    {
        for (u32 i = 0; i < vk_swapchain->present_modes_count; i++)
        {
            if (vk_swapchain->present_modes[i] == VK_PRESENT_MODE_FIFO_KHR)
            {
                best_present_mode_index = i;
                break;
            }
        }
    }
    u32 screen_width  = INVALID_ID;
    u32 screen_height = INVALID_ID;
    platform_get_window_dimensions(&screen_width, &screen_height);

    // choose the swap extent

    best_2d_extent        = {};
    best_2d_extent.width  = DCLAMP(screen_width, vk_swapchain->surface_capabilities.minImageExtent.width,
                                   vk_swapchain->surface_capabilities.maxImageExtent.width);
    best_2d_extent.height = DCLAMP(screen_height, vk_swapchain->surface_capabilities.minImageExtent.height,
                                   vk_swapchain->surface_capabilities.maxImageExtent.height);

    // Create swapchain
    //  INFO: triple-buffering if possible
    u32 image_count = MAX_FRAMES_IN_FLIGHT;
    if (vk_swapchain->surface_capabilities.minImageCount + 1 < image_count)
    {
        image_count = vk_swapchain->surface_capabilities.minImageCount + 1;
        DWARN("Triple buffring requested but found %d buffering", image_count);
    }

    u32 best_image_sharing_mode    = INVALID_ID;
    u32 enabled_queue_family_count = vk_context->vk_device.enabled_queue_family_count;
    u32 queue_family_indicies[4]   = {vk_context->vk_device.graphics_family_index,
                                      vk_context->vk_device.present_family_index, 0, 0};

    if (enabled_queue_family_count == 1)
    {
        best_image_sharing_mode = VK_SHARING_MODE_EXCLUSIVE;
    }
    else
    {
        best_image_sharing_mode = VK_SHARING_MODE_CONCURRENT;
    }

    VkSwapchainCreateInfoKHR swapchain_create_info{};
    swapchain_create_info.sType                 = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swapchain_create_info.pNext                 = 0;
    swapchain_create_info.flags                 = 0;
    swapchain_create_info.surface               = vk_context->vk_surface;
    swapchain_create_info.minImageCount         = image_count;
    swapchain_create_info.imageFormat           = vk_swapchain->surface_formats[best_surface_format_index].format;
    swapchain_create_info.imageColorSpace       = vk_swapchain->surface_formats[best_surface_format_index].colorSpace;
    swapchain_create_info.imageExtent           = best_2d_extent;
    swapchain_create_info.imageArrayLayers      = 1;
    swapchain_create_info.imageUsage            = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    swapchain_create_info.imageSharingMode      = (VkSharingMode)best_image_sharing_mode;
    swapchain_create_info.queueFamilyIndexCount = enabled_queue_family_count;
    swapchain_create_info.pQueueFamilyIndices   = queue_family_indicies;
    swapchain_create_info.preTransform          = vk_swapchain->surface_capabilities.currentTransform;
    swapchain_create_info.compositeAlpha        = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    swapchain_create_info.presentMode           = vk_swapchain->present_modes[best_present_mode_index];
    swapchain_create_info.clipped               = VK_TRUE;
    swapchain_create_info.oldSwapchain          = VK_NULL_HANDLE;

    VkResult result = vkCreateSwapchainKHR(vk_context->vk_device.logical, &swapchain_create_info,
                                           vk_context->vk_allocator, &vk_swapchain->handle);
    VK_CHECK(result);

    u32 swapchain_images_count = INVALID_ID;

    result = vkGetSwapchainImagesKHR(vk_context->vk_device.logical, vk_swapchain->handle, &swapchain_images_count, 0);
    VK_CHECK(result);

    if (vk_swapchain->images == nullptr)
    {
        vk_swapchain->images = (VkImage *)dallocate(sizeof(VkImage) * swapchain_images_count, MEM_TAG_RENDERER);
    }
    vkGetSwapchainImagesKHR(vk_context->vk_device.logical, vk_swapchain->handle, &swapchain_images_count,
                            vk_swapchain->images);

    vk_swapchain->images_count   = swapchain_images_count;
    vk_swapchain->img_format     = vk_swapchain->surface_formats[best_surface_format_index].format;
    vk_swapchain->surface_extent = best_2d_extent;
    vk_swapchain->width          = screen_width;
    vk_swapchain->height         = screen_height;

    if (vk_context->vk_swapchain.img_views == nullptr)
    {
        vk_context->vk_swapchain.img_views =
            (VkImageView *)dallocate(sizeof(VkImageView) * swapchain_images_count, MEM_TAG_RENDERER);
    }

    for (u32 i = 0; i < swapchain_images_count; i++)
    {
        vulkan_create_image_view(vk_context, vk_swapchain->images[i], vk_swapchain->img_format,
                                 &vk_swapchain->img_views[i], VK_IMAGE_ASPECT_COLOR_BIT);
    }

    bool res = vulkan_find_suitable_depth_format(&vk_context->vk_device, &vk_context->vk_swapchain.depth_image);
    DASSERT(res == true);

    res = vulkan_create_image(vk_context, &vk_context->vk_swapchain.depth_image,
                              vk_context->vk_swapchain.depth_image.format, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                              VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_IMAGE_TILING_OPTIMAL);
    DASSERT(res == true);
    res = vulkan_create_image_view(vk_context, vk_context->vk_swapchain.depth_image.handle,
                                   vk_context->vk_swapchain.depth_image.format,
                                   &vk_context->vk_swapchain.depth_image.view, VK_IMAGE_ASPECT_DEPTH_BIT);
    DASSERT(res == true);

    return true;
}

bool destroy_swapchain(vulkan_context *vk_context)
{
    VkDevice              &device    = vk_context->vk_device.logical;
    VkAllocationCallbacks *allocator = vk_context->vk_allocator;
    if (vk_context->vk_swapchain.handle)
    {
        for (u32 i = 0; i < vk_context->vk_swapchain.images_count; i++)
        {
            vkDestroyImageView(device, vk_context->vk_swapchain.img_views[i], vk_context->vk_allocator);
        }
        for (u32 i = 0; i < vk_context->vk_swapchain.images_count; i++)
        {
            vkDestroyFramebuffer(device, vk_context->vk_swapchain.buffers[i].handle, vk_context->vk_allocator);
        }
        vkDestroyImageView(device, vk_context->vk_swapchain.depth_image.view, allocator);
        vkDestroyImage(device, vk_context->vk_swapchain.depth_image.handle, allocator);
        vkFreeMemory(device, vk_context->vk_swapchain.depth_image.memory, allocator);

        vkDestroySwapchainKHR(vk_context->vk_device.logical, vk_context->vk_swapchain.handle, vk_context->vk_allocator);
    }
    return true;
}

bool vulkan_create_swapchain(vulkan_context *vk_context)
{
    bool result = create_swapchain(vk_context);
    if (!result)
    {
        DDEBUG("Create swapchain failed.");
        return false;
    }
    return result;
}

bool vulkan_recreate_swapchain(vulkan_context *vk_context)
{
    vkDeviceWaitIdle(vk_context->vk_device.logical);
    bool result = destroy_swapchain(vk_context);
    if (!result)
    {
        DDEBUG("Destroy swapchain failed.");
        return false;
    }
    result = create_swapchain(vk_context);
    if (!result)
    {
        DDEBUG("Create swapchain failed.");
        return false;
    }
    vulkan_create_framebuffers(vk_context);
    return result;
}
