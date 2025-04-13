#include "vulkan_command_buffers.hpp"

bool vulkan_create_command_pool(vulkan_context *vk_context)
{
    VkCommandPoolCreateInfo command_pool_create_info{};

    command_pool_create_info.sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    command_pool_create_info.pNext            = 0;
    command_pool_create_info.flags            = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    command_pool_create_info.queueFamilyIndex = vk_context->vk_device.graphics_family_index;

    VkResult result = vkCreateCommandPool(vk_context->vk_device.logical, &command_pool_create_info,
                                          vk_context->vk_allocator, &vk_context->graphics_command_pool);
    VK_CHECK(result);

    return true;
}

bool vulkan_create_command_buffer(vulkan_context *vk_context, VkCommandPool *command_pool)
{
    VkCommandBufferAllocateInfo command_buffer_alloc_info{};

    command_buffer_alloc_info.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    command_buffer_alloc_info.pNext              = nullptr;
    command_buffer_alloc_info.commandPool        = vk_context->graphics_command_pool;
    command_buffer_alloc_info.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    command_buffer_alloc_info.commandBufferCount = 1;

    VkResult result = vkAllocateCommandBuffers(vk_context->vk_device.logical, &command_buffer_alloc_info,
                                               &vk_context->command_buffer);
    return true;
}

void vulkan_record_command_buffer_and_use(vulkan_context *vk_context, VkCommandBuffer command_buffer, u32 image_index)
{
    VkCommandBufferBeginInfo command_buffer_begin_info{};
    command_buffer_begin_info.sType            = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    command_buffer_begin_info.flags            = 0;       // Optional
    command_buffer_begin_info.pInheritanceInfo = nullptr; // Optional

    VkResult result                            = vkBeginCommandBuffer(command_buffer, &command_buffer_begin_info);
    VK_CHECK(result);

    // start a render pass

    // starting color
    VkClearValue clear_color = {
        .color = {0.0f, 0.0f, 0.0f, 1.0f},
    };

    VkRenderPassBeginInfo renderpass_begin_info{};
    renderpass_begin_info.sType             = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderpass_begin_info.pNext             = 0;
    renderpass_begin_info.renderPass        = vk_context->vk_renderpass;
    renderpass_begin_info.framebuffer       = vk_context->vk_swapchain.buffers[image_index].handle;
    renderpass_begin_info.renderArea.offset = {0, 0};
    renderpass_begin_info.renderArea.extent = vk_context->vk_swapchain.surface_extent;
    renderpass_begin_info.clearValueCount   = 1;
    renderpass_begin_info.pClearValues      = &clear_color;

    vkCmdBeginRenderPass(command_buffer, &renderpass_begin_info, VK_SUBPASS_CONTENTS_INLINE);

    vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vk_context->vk_graphics_pipeline.handle);

    VkExtent2D *swapchain_extent = &vk_context->vk_swapchain.surface_extent;

    VkViewport view_port{};
    view_port.x        = 0.0f;
    view_port.y        = 0.0f;
    view_port.width    = swapchain_extent->width;
    view_port.height   = swapchain_extent->height;
    view_port.minDepth = 0.0f;
    view_port.maxDepth = 1.0f;

    vkCmdSetViewport(command_buffer, 0, 1, &view_port);

    VkRect2D scissor{};
    scissor.offset = {0, 0};
    scissor.extent = *swapchain_extent;
    vkCmdSetScissor(command_buffer, 0, 1, &scissor);

    vkCmdDraw(command_buffer, 3, 1, 0, 0);

    vkCmdEndRenderPass(command_buffer);

    result = vkEndCommandBuffer(command_buffer);
    VK_CHECK(result);
}
