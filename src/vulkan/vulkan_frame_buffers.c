#include "vulkan_frame_buffers.h"
#include "containers/array.h"
#include "core/memory.h"

b8 vulkan_create_frame_buffers(vulkan_context *context)
{
    INFO("Creating vulkan frame buffers...");

    u32 swapchain_image_views_size   = context->swapchain.image_views_count;
    context->swapchain.frame_buffers = ALLOCATE_MEMORY_RENDERER(sizeof(VkFramebuffer) * swapchain_image_views_size);

    for (u32 i = 0; i < swapchain_image_views_size; i++)
    {
        VkFramebufferCreateInfo frame_buffer_create_info = {};
        frame_buffer_create_info.sType                   = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        frame_buffer_create_info.pNext                   = 0;
        frame_buffer_create_info.flags                   = 0;
        frame_buffer_create_info.renderPass              = context->renderpass;
        frame_buffer_create_info.attachmentCount         = 1;
        frame_buffer_create_info.pAttachments            = &context->swapchain.image_views[i];
        frame_buffer_create_info.width                   = context->swapchain.extent2D.width;
        frame_buffer_create_info.height                  = context->swapchain.extent2D.height;
        frame_buffer_create_info.layers                  = 1;

        VK_CHECK(vkCreateFramebuffer(context->device.logical, &frame_buffer_create_info, 0, &context->swapchain.frame_buffers[i]));
    }
    return true;
}
