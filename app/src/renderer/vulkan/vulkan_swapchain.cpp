#include "vulkan_swapchain.hpp"
#include "core/dmemory.hpp"
#include "core/logger.hpp"
#include "vulkan_device.hpp"

bool vulkan_create_swapchain(vulkan_context *vk_context)
{
    // INFO: query swapchain support
    vulkan_device_query_swapchain_support(vk_context, vk_context->device.physical, &vk_context->vk_swapchain);

    vulkan_swapchain *vk_swapchain = &vk_context->vk_swapchain;

    u32 best_surface_format_index  = INVALID_ID;
    u32 best_present_mode_index    = INVALID_ID;
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
    // choose the swap extent

    return false;
}
