#include "vulkan_frame_buffers.h"
#include "containers/array.h"

b8 vulkan_create_frame_buffers(vulkan_context *context)
{
    INFO("Creating vulkan frame buffers...");

    u32 swapchain_image_views_size = (u32)array_get_length(context->image_views);
    context->frame_buffers         = array_create_with_capacity(VkFramebuffer, swapchain_image_views_size);

    for (u32 i = 0; i < swapchain_image_views_size; i++)
    {
        VkFramebufferCreateInfo frame_buffer_create_info = {};
        frame_buffer_create_info.sType                   = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        frame_buffer_create_info.pNext                   = 0;
        frame_buffer_create_info.flags                   = 0;
        frame_buffer_create_info.renderPass              = context->renderpass;
        frame_buffer_create_info.attachmentCount         = 1;
        frame_buffer_create_info.pAttachments            = &context->image_views[i];
        frame_buffer_create_info.width                   = context->swapchain.extent2D.width;
        frame_buffer_create_info.height                  = context->swapchain.extent2D.height;
        frame_buffer_create_info.layers                  = 1;

        VK_CHECK(vkCreateFramebuffer(context->device.logical, &frame_buffer_create_info, 0, &context->frame_buffers[i]));
    }
    array_set_length(context->frame_buffers, swapchain_image_views_size);
    return true;
}
