#include "vulkan_renderpass.hpp"

bool vulkan_create_renderpass(vulkan_context *vk_context)
{
    // create render pass
    VkAttachmentDescription color_attachment{};
    color_attachment.flags          = 0;
    color_attachment.format         = vk_context->vk_swapchain.vk_images.format;
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

    VkSubpassDescription subpass_desc{};
    subpass_desc.flags                   = 0;
    subpass_desc.pipelineBindPoint       = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass_desc.inputAttachmentCount    = 0;
    subpass_desc.pInputAttachments       = nullptr;
    subpass_desc.colorAttachmentCount    = 1;
    subpass_desc.pColorAttachments       = &color_attachment_reference;
    subpass_desc.pResolveAttachments     = nullptr;
    subpass_desc.pDepthStencilAttachment = nullptr;
    subpass_desc.preserveAttachmentCount = 0;
    subpass_desc.pPreserveAttachments    = nullptr;

    VkSubpassDependency dependency{};
    dependency.srcSubpass    = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass    = 0;
    dependency.srcStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    VkRenderPassCreateInfo render_pass_create_info{};
    render_pass_create_info.sType           = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    render_pass_create_info.pNext           = 0;
    render_pass_create_info.flags           = 0;
    render_pass_create_info.attachmentCount = 1;
    render_pass_create_info.pAttachments    = &color_attachment;
    render_pass_create_info.subpassCount    = 1;
    render_pass_create_info.pSubpasses      = &subpass_desc;
    render_pass_create_info.dependencyCount = 1;
    render_pass_create_info.pDependencies   = &dependency;

    VkResult result = vkCreateRenderPass(vk_context->vk_device.logical, &render_pass_create_info,
                                         vk_context->vk_allocator, &vk_context->vk_renderpass);
    VK_CHECK(result);
    return true;
}
