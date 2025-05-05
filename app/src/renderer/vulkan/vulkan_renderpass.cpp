#include "vulkan_renderpass.hpp"
#include "core/application.hpp"
#include "renderer/vulkan/vulkan_backend.hpp"
#include "renderer/vulkan/vulkan_types.hpp"

bool vulkan_create_renderpass(vulkan_context *vk_context)
{
    // create render pass
    VkAttachmentDescription color_attachment{};
    color_attachment.flags          = 0;
    color_attachment.format         = vk_context->vk_swapchain.img_format;
    color_attachment.samples        = VK_SAMPLE_COUNT_1_BIT;
    color_attachment.loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR;
    color_attachment.storeOp        = VK_ATTACHMENT_STORE_OP_STORE;
    color_attachment.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    color_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    color_attachment.initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
    color_attachment.finalLayout    = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference color_attachment_reference{};
    color_attachment_reference.attachment = 0;
    color_attachment_reference.layout     = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentDescription depth_attachment{};
    depth_attachment.format         = vk_context->vk_swapchain.depth_image.format;
    depth_attachment.samples        = VK_SAMPLE_COUNT_1_BIT;
    depth_attachment.loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depth_attachment.storeOp        = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depth_attachment.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depth_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depth_attachment.initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
    depth_attachment.finalLayout    = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkAttachmentReference depth_attachment_reference{};
    depth_attachment_reference.attachment = 1;
    depth_attachment_reference.layout     = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass_desc{};
    subpass_desc.flags                   = 0;
    subpass_desc.pipelineBindPoint       = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass_desc.inputAttachmentCount    = 0;
    subpass_desc.pInputAttachments       = nullptr;
    subpass_desc.colorAttachmentCount    = 1;
    subpass_desc.pColorAttachments       = &color_attachment_reference;
    subpass_desc.pResolveAttachments     = nullptr;
    subpass_desc.pDepthStencilAttachment = &depth_attachment_reference;
    subpass_desc.preserveAttachmentCount = 0;
    subpass_desc.pPreserveAttachments    = nullptr;

    VkSubpassDependency dependency{};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask =
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstStageMask =
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

    VkAttachmentDescription attachment_desc[2] = {color_attachment, depth_attachment};

    VkRenderPassCreateInfo render_pass_create_info{};
    render_pass_create_info.sType           = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    render_pass_create_info.pNext           = 0;
    render_pass_create_info.flags           = 0;
    render_pass_create_info.attachmentCount = 2;
    render_pass_create_info.pAttachments    = attachment_desc;
    render_pass_create_info.subpassCount    = 1;
    render_pass_create_info.pSubpasses      = &subpass_desc;
    render_pass_create_info.dependencyCount = 1;
    render_pass_create_info.pDependencies   = &dependency;

    VkResult result = vkCreateRenderPass(vk_context->vk_device.logical, &render_pass_create_info,
                                         vk_context->vk_allocator, &vk_context->vk_renderpass);
    VK_CHECK(result);
    return true;
}

bool vulkan_begin_frame_renderpass(vulkan_context *vk_context, VkCommandBuffer command_buffer,
                                   vulkan_geometry_data *geo_data, u32 image_index)
{
    VkClearValue clear_values[2] = {};
    clear_values[0]              = {.color = {{0.11f, 0.125f, 0.51f, 1.0f}}};
    clear_values[1]              = {.depthStencil = {1.0f, 0}};

    VkRenderPassBeginInfo renderpass_begin_info{};
    renderpass_begin_info.sType             = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderpass_begin_info.pNext             = 0;
    renderpass_begin_info.renderPass        = vk_context->vk_renderpass;
    renderpass_begin_info.framebuffer       = vk_context->vk_swapchain.buffers[image_index];
    renderpass_begin_info.renderArea.offset = {0, 0};
    renderpass_begin_info.renderArea.extent = vk_context->vk_swapchain.surface_extent;
    renderpass_begin_info.clearValueCount   = 2;
    renderpass_begin_info.pClearValues      = clear_values;

    vkCmdBeginRenderPass(command_buffer, &renderpass_begin_info, VK_SUBPASS_CONTENTS_INLINE);

    vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vk_context->vk_graphics_pipeline.handle);

    VkExtent2D *swapchain_extent = &vk_context->vk_swapchain.surface_extent;

    VkViewport view_port{};
    view_port.x        = 0.0f;
    view_port.y        = 0.0f;
    view_port.width    = (f32)swapchain_extent->width;
    view_port.height   = (f32)swapchain_extent->height;
    view_port.minDepth = 0.0f;
    view_port.maxDepth = 1.0f;

    vkCmdSetViewport(command_buffer, 0, 1, &view_port);

    VkRect2D scissor{};
    scissor.offset = {0, 0};
    scissor.extent = *swapchain_extent;
    vkCmdSetScissor(command_buffer, 0, 1, &scissor);

    VkBuffer     vertex_buffers[] = {vk_context->vertex_buffer.handle};
    VkDeviceSize offsets[]        = {0};

    vkCmdBindVertexBuffers(command_buffer, 0, 1, vertex_buffers, offsets);
    vkCmdBindIndexBuffer(command_buffer, vk_context->index_buffer.handle, 0, VK_INDEX_TYPE_UINT32);

    vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vk_context->vk_graphics_pipeline.layout, 0,
                            1, &vk_context->descriptor_sets[image_index], 0, nullptr);

    // u32 index_offset  = vulkan_calculate_index_offset(vk_context, geo_data->id);
    // u32 vertex_offset = vulkan_calculate_vertex_offset(vk_context, geo_data->id);

    // vkCmdDrawIndexed(command_buffer, geo_data->indices_count, 1, index_offset, vertex_offset, 0);
    vkCmdDrawIndexed(command_buffer, 1000000, 1, 0, 0, 0);
    vkCmdEndRenderPass(command_buffer);

    return true;
}
