#include "vulkan_framebuffer.hpp"
#include "core/dmemory.hpp"
#include "core/logger.hpp"

bool vulkan_create_framebuffers(vulkan_context *vk_context)
{
    DINFO("Creating vulkan framebuffers...");

    u32 framebuffers_count = vk_context->vk_swapchain.images_count;
    if (vk_context->vk_swapchain.buffers == nullptr)
    {
        vk_context->vk_swapchain.buffers =
            (vulkan_framebuffer *)dallocate(sizeof(vulkan_framebuffer) * framebuffers_count, MEM_TAG_RENDERER);
    }
    else
    {
        dzero_memory(vk_context->vk_swapchain.buffers, sizeof(vulkan_framebuffer) * framebuffers_count);
    }

    vulkan_framebuffer *buffers = vk_context->vk_swapchain.buffers;

    for (u32 i = 0; i < framebuffers_count; i++)
    {
        VkImageView framebuffer_attachments[2] = {vk_context->vk_swapchain.img_views[i],
                                                  vk_context->vk_swapchain.depth_image.view};

        VkFramebufferCreateInfo framebuffer_create_info{};

        framebuffer_create_info.sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebuffer_create_info.pNext           = 0;
        framebuffer_create_info.flags           = 0;
        framebuffer_create_info.renderPass      = vk_context->vk_renderpass;
        framebuffer_create_info.attachmentCount = 2;
        framebuffer_create_info.pAttachments    = framebuffer_attachments;
        framebuffer_create_info.width           = vk_context->vk_swapchain.surface_extent.width;
        framebuffer_create_info.height          = vk_context->vk_swapchain.surface_extent.height;
        framebuffer_create_info.layers          = 1;

        VkResult result = vkCreateFramebuffer(vk_context->vk_device.logical, &framebuffer_create_info,
                                              vk_context->vk_allocator, &buffers[i].handle);
        VK_CHECK(result);
    }

    return true;
}
