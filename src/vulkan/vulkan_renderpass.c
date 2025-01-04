#include "vulkan_renderpass.h"

b8 vulkan_create_renderpass(vulkan_context *context)
{
    DEBUG("Creating render pass...");
    VkAttachmentDescription color_attachment = {};

    color_attachment.flags          = 0;
    color_attachment.format         = context->swapchain.format.format;
    color_attachment.samples        = VK_SAMPLE_COUNT_1_BIT;
    color_attachment.loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR;
    color_attachment.storeOp        = VK_ATTACHMENT_STORE_OP_STORE;
    color_attachment.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    color_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    color_attachment.initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
    color_attachment.finalLayout    = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference color_attachment_ref = {};
    color_attachment_ref.attachment            = 0;
    color_attachment_ref.layout                = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass_description    = {};
    subpass_description.flags                   = 0;
    subpass_description.pipelineBindPoint       = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass_description.inputAttachmentCount    = 0;
    subpass_description.pInputAttachments       = 0;
    subpass_description.colorAttachmentCount    = 1;
    subpass_description.pColorAttachments       = &color_attachment_ref;
    subpass_description.pResolveAttachments     = 0;
    subpass_description.pDepthStencilAttachment = 0;
    subpass_description.preserveAttachmentCount = 0;
    subpass_description.pPreserveAttachments    = 0;

    VkRenderPassCreateInfo renderpass_create_info = {};
    renderpass_create_info.sType                  = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderpass_create_info.pNext                  = 0;
    renderpass_create_info.flags                  = 0;
    renderpass_create_info.attachmentCount        = 1;
    renderpass_create_info.pAttachments           = &color_attachment;
    renderpass_create_info.subpassCount           = 1;
    renderpass_create_info.pSubpasses             = &subpass_description;
    renderpass_create_info.dependencyCount        = 0;
    renderpass_create_info.pDependencies          = 0;

    VK_CHECK(vkCreateRenderPass(context->device.logical, &renderpass_create_info, 0, &context->renderpass));
    DEBUG("Render pass created");

    return true;
}
