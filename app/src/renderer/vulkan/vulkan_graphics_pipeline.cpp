#include "vulkan_graphics_pipeline.hpp"
#include "core/dfile_system.hpp"
#include "core/dmemory.hpp"

VkShaderModule create_shader_module(vulkan_context *vk_context, const char *shader_code, u64 shader_code_size);

bool vulkan_create_graphics_pipeline(vulkan_context *vk_context)
{
    // INFO: shader stage
    char       *vert_shader_code;
    u64         vert_shader_code_buffer_size_requirements = INVALID_ID_64;
    const char *vert_shader_file_name                     = "default_shader.vert.spv";

    file_open_and_read(vert_shader_file_name, &vert_shader_code_buffer_size_requirements, 0, 1);
    vert_shader_code = (char *)dallocate(vert_shader_code_buffer_size_requirements, MEM_TAG_RENDERER);
    file_open_and_read(vert_shader_file_name, &vert_shader_code_buffer_size_requirements, vert_shader_code, 1);

    char       *frag_shader_code;
    u64         frag_shader_code_buffer_size_requirements = INVALID_ID_64;
    const char *frag_shader_file_name                     = "default_shader.frag.spv";

    file_open_and_read(frag_shader_file_name, &frag_shader_code_buffer_size_requirements, 0, 1);
    frag_shader_code = (char *)dallocate(frag_shader_code_buffer_size_requirements, MEM_TAG_RENDERER);
    file_open_and_read(frag_shader_file_name, &frag_shader_code_buffer_size_requirements, frag_shader_code, 1);

    VkShaderModule vert_shader_module =
        create_shader_module(vk_context, vert_shader_code, vert_shader_code_buffer_size_requirements);
    VkShaderModule frag_shader_module =
        create_shader_module(vk_context, frag_shader_code, frag_shader_code_buffer_size_requirements);

    // create the pipeline shader stage
    u32                   stage_flag_bits_count = 2;
    VkShaderStageFlagBits stage_flag_bits[2]    = {VK_SHADER_STAGE_VERTEX_BIT, VK_SHADER_STAGE_FRAGMENT_BIT};

    VkShaderModule shader_modules[2]            = {vert_shader_module, frag_shader_module};

    VkPipelineShaderStageCreateInfo shader_stage_create_infos[2]{};

    for (u32 i = 0; i < stage_flag_bits_count; i++)
    {
        VkPipelineShaderStageCreateInfo shader_stage_create_info{};
        shader_stage_create_info.sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        shader_stage_create_info.pNext  = 0;
        shader_stage_create_info.flags  = 0;
        shader_stage_create_info.stage  = stage_flag_bits[i];
        shader_stage_create_info.module = shader_modules[i];
        shader_stage_create_info.pName  = "main";

        shader_stage_create_infos[i]    = shader_stage_create_info;
    }
    // INFO:shader stage end

    // INFO: Dynamic states
    u32            dynamic_states_count = 2;
    VkDynamicState dynamic_states[2]    = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};

    VkPipelineDynamicStateCreateInfo dynamic_state_create_info{};
    dynamic_state_create_info.sType             = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamic_state_create_info.pNext             = 0;
    dynamic_state_create_info.flags             = 0;
    dynamic_state_create_info.dynamicStateCount = dynamic_states_count;
    dynamic_state_create_info.pDynamicStates    = dynamic_states;
    // INFO: Dynamic stated end

    // INFO: Vertex INPUT

    VkPipelineVertexInputStateCreateInfo vertex_input_state_create_info{};

    vertex_input_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertex_input_state_create_info.pNext = 0;
    vertex_input_state_create_info.flags = 0;
    vertex_input_state_create_info.vertexBindingDescriptionCount   = 0;
    vertex_input_state_create_info.pVertexBindingDescriptions      = nullptr;
    vertex_input_state_create_info.vertexAttributeDescriptionCount = 0;
    vertex_input_state_create_info.pVertexAttributeDescriptions    = nullptr;

    VkPipelineInputAssemblyStateCreateInfo assembly_state_create_info{};
    assembly_state_create_info.sType                  = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    assembly_state_create_info.pNext                  = 0;
    assembly_state_create_info.flags                  = 0;
    assembly_state_create_info.topology               = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    assembly_state_create_info.primitiveRestartEnable = VK_FALSE;

    VkViewport view_port{};
    view_port.x        = 0.0f;
    view_port.y        = 0.0f;
    view_port.width    = vk_context->vk_swapchain.surface_extent.width;
    view_port.height   = vk_context->vk_swapchain.surface_extent.width;
    view_port.minDepth = 0.0f;
    view_port.maxDepth = 1.0f;

    VkRect2D scissor{};
    scissor.offset = {0, 0};
    scissor.extent = vk_context->vk_swapchain.surface_extent;

    VkPipelineViewportStateCreateInfo viewport_state_create_info{};
    viewport_state_create_info.sType         = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewport_state_create_info.pNext         = 0;
    viewport_state_create_info.flags         = 0;
    viewport_state_create_info.viewportCount = 1;
    viewport_state_create_info.pViewports    = &view_port;
    viewport_state_create_info.scissorCount  = 1;
    viewport_state_create_info.pScissors     = &scissor;

    VkPipelineRasterizationStateCreateInfo rasterization_create_info{};
    rasterization_create_info.sType                   = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterization_create_info.pNext                   = 0;
    rasterization_create_info.flags                   = 0;
    rasterization_create_info.depthClampEnable        = VK_FALSE;
    rasterization_create_info.rasterizerDiscardEnable = VK_FALSE;
    rasterization_create_info.polygonMode             = VK_POLYGON_MODE_FILL;
    rasterization_create_info.cullMode                = VK_CULL_MODE_BACK_BIT;
    rasterization_create_info.frontFace               = VK_FRONT_FACE_CLOCKWISE;
    rasterization_create_info.depthBiasEnable         = VK_FALSE;
    rasterization_create_info.depthBiasConstantFactor = 0.0f;
    rasterization_create_info.depthBiasClamp          = 0.0f;
    rasterization_create_info.depthBiasSlopeFactor    = 0.0f;
    rasterization_create_info.lineWidth               = 1.0f;

    VkPipelineMultisampleStateCreateInfo multisampling_state_create_info{};
    multisampling_state_create_info.sType                 = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling_state_create_info.pNext                 = 0;
    multisampling_state_create_info.flags                 = 0;
    multisampling_state_create_info.rasterizationSamples  = VK_SAMPLE_COUNT_1_BIT;
    multisampling_state_create_info.sampleShadingEnable   = VK_FALSE;
    multisampling_state_create_info.minSampleShading      = 1.0f;
    multisampling_state_create_info.pSampleMask           = nullptr;
    multisampling_state_create_info.alphaToCoverageEnable = VK_FALSE;
    multisampling_state_create_info.alphaToOneEnable      = VK_FALSE;

    VkPipelineColorBlendAttachmentState color_blend_attachment_state{};
    color_blend_attachment_state.colorWriteMask =
        VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    color_blend_attachment_state.blendEnable         = VK_FALSE;
    color_blend_attachment_state.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;  // Optional
    color_blend_attachment_state.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
    color_blend_attachment_state.colorBlendOp        = VK_BLEND_OP_ADD;      // Optional
    color_blend_attachment_state.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;  // Optional
    color_blend_attachment_state.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
    color_blend_attachment_state.alphaBlendOp        = VK_BLEND_OP_ADD;      // Optional

    VkPipelineColorBlendStateCreateInfo color_blend_state_create_info{};
    color_blend_state_create_info.sType             = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    color_blend_state_create_info.pNext             = 0;
    color_blend_state_create_info.flags             = 0;
    color_blend_state_create_info.logicOpEnable     = VK_FALSE;
    color_blend_state_create_info.logicOp           = VK_LOGIC_OP_COPY;
    color_blend_state_create_info.attachmentCount   = 1;
    color_blend_state_create_info.pAttachments      = &color_blend_attachment_state;
    color_blend_state_create_info.blendConstants[0] = 0.0f;
    color_blend_state_create_info.blendConstants[1] = 0.0f;
    color_blend_state_create_info.blendConstants[2] = 0.0f;
    color_blend_state_create_info.blendConstants[3] = 0.0f;

    VkPipelineLayoutCreateInfo pipeline_layout_create_info{};

    pipeline_layout_create_info.sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipeline_layout_create_info.pNext                  = 0;
    pipeline_layout_create_info.flags                  = 0;
    pipeline_layout_create_info.setLayoutCount         = 0;
    pipeline_layout_create_info.pSetLayouts            = nullptr;
    pipeline_layout_create_info.pushConstantRangeCount = 0;
    pipeline_layout_create_info.pPushConstantRanges    = nullptr;

    VkResult result = vkCreatePipelineLayout(vk_context->device.logical, &pipeline_layout_create_info,
                                             vk_context->vk_allocator, &vk_context->graphics_pipeline_layout);
    VK_CHECK(result);

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

    VkRenderPassCreateInfo render_pass_create_info{};
    render_pass_create_info.sType           = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    render_pass_create_info.pNext           = 0;
    render_pass_create_info.flags           = 0;
    render_pass_create_info.attachmentCount = 1;
    render_pass_create_info.pAttachments    = &color_attachment;
    render_pass_create_info.subpassCount    = 1;
    render_pass_create_info.pSubpasses      = &subpass_desc;
    render_pass_create_info.dependencyCount = 0;
    render_pass_create_info.pDependencies   = nullptr;

    result = vkCreateRenderPass(vk_context->device.logical, &render_pass_create_info, vk_context->vk_allocator,
                                &vk_context->vk_renderpass);

    VK_CHECK(result);

    vkDestroyShaderModule(vk_context->device.logical, vert_shader_module, vk_context->vk_allocator);
    vkDestroyShaderModule(vk_context->device.logical, frag_shader_module, vk_context->vk_allocator);

    return true;
}

VkShaderModule create_shader_module(vulkan_context *vk_context, const char *shader_code, u64 shader_code_size)
{
    VkShaderModuleCreateInfo shader_module_create_info{};

    shader_module_create_info.sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    shader_module_create_info.pNext    = 0;
    shader_module_create_info.flags    = 0;
    shader_module_create_info.codeSize = shader_code_size;
    shader_module_create_info.pCode    = (u32 *)shader_code;

    VkShaderModule shader_module{};

    VkResult result = vkCreateShaderModule(vk_context->device.logical, &shader_module_create_info,
                                           vk_context->vk_allocator, &shader_module);
    VK_CHECK(result);

    return shader_module;
}
