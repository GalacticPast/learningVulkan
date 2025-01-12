#include "vulkan_command_buffers.h"
#include "core/memory.h"

b8 vulkan_create_command_buffers(vulkan_context *context)
{

    context->command_buffers = ALLOCATE_MEMORY_RENDERER(sizeof(VkCommandBuffer) * context->max_frames_in_flight);

    VkCommandBufferAllocateInfo command_buffer_alloc_info = {};

    command_buffer_alloc_info.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    command_buffer_alloc_info.pNext              = 0;
    command_buffer_alloc_info.commandPool        = context->command_pool;
    command_buffer_alloc_info.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    command_buffer_alloc_info.commandBufferCount = context->max_frames_in_flight;

    VK_CHECK(vkAllocateCommandBuffers(context->device.logical, &command_buffer_alloc_info, context->command_buffers));

    return true;
}

b8 vulkan_create_command_pool(vulkan_context *context)
{
    VkCommandPoolCreateInfo command_pool_info = {};

    command_pool_info.sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    command_pool_info.pNext            = 0;
    command_pool_info.flags            = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    command_pool_info.queueFamilyIndex = context->device.graphics_queue_index;

    VK_CHECK(vkCreateCommandPool(context->device.logical, &command_pool_info, 0, &context->command_pool));
    return true;
}

b8 vulkan_record_command_buffer(vulkan_context *context, VkCommandBuffer command_buffer, u32 image_index)
{
    VkCommandBufferBeginInfo command_buffer_begin_info = {};

    command_buffer_begin_info.sType            = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    command_buffer_begin_info.pNext            = 0;
    command_buffer_begin_info.flags            = 0;
    command_buffer_begin_info.pInheritanceInfo = VK_NULL_HANDLE;

    VK_CHECK(vkBeginCommandBuffer(command_buffer, &command_buffer_begin_info));

    VkRenderPassBeginInfo render_pass_begin_info = {};

    render_pass_begin_info.sType             = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    render_pass_begin_info.pNext             = 0;
    render_pass_begin_info.renderPass        = context->renderpass;
    render_pass_begin_info.framebuffer       = context->swapchain.frame_buffers[image_index];
    render_pass_begin_info.renderArea.offset = (VkOffset2D){0, 0};
    render_pass_begin_info.renderArea.extent = context->swapchain.extent2D;

    VkClearValue clear_color     = {};
    clear_color.color.float32[0] = 0.0f;
    clear_color.color.float32[1] = 0.0f;
    clear_color.color.float32[2] = 0.0f;
    clear_color.color.float32[3] = 1.0f;

    render_pass_begin_info.clearValueCount = 1;
    render_pass_begin_info.pClearValues    = &clear_color;

    vkCmdBeginRenderPass(command_buffer, &render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);

    vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, context->graphics_pipeline.handle);

    VkViewport dynamic_view_port = {};

    dynamic_view_port.x        = 0;
    dynamic_view_port.y        = 0;
    dynamic_view_port.width    = (f32)context->swapchain.extent2D.width;
    dynamic_view_port.height   = (f32)context->swapchain.extent2D.height;
    dynamic_view_port.minDepth = 0.0f;
    dynamic_view_port.maxDepth = 1.0f;

    vkCmdSetViewport(command_buffer, 0, 1, &dynamic_view_port);

    VkRect2D dynamic_scissor = {};
    dynamic_scissor.offset   = (VkOffset2D){0, 0};
    dynamic_scissor.extent   = context->swapchain.extent2D;

    vkCmdSetScissor(command_buffer, 0, 1, &dynamic_scissor);

    VkBuffer     vertexBuffers[] = {context->vertex_buffer.handle};
    VkDeviceSize offsets[]       = {0};
    vkCmdBindVertexBuffers(command_buffer, 0, 1, vertexBuffers, offsets);

    vkCmdDraw(command_buffer, 6, 1, 0, 0);

    vkCmdEndRenderPass(command_buffer);

    VK_CHECK(vkEndCommandBuffer(command_buffer));

    return true;
}
