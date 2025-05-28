#include "vulkan_graphics_pipeline.hpp"
#include "containers/darray.hpp"
#include "core/logger.hpp"
#include "math/dmath_types.hpp"

VkShaderModule create_shader_module(vulkan_context *vk_context, const char *shader_code, u64 shader_code_size);

bool vulkan_create_graphics_pipeline(vulkan_context *vk_context, vulkan_shader *shader)
{
    DDEBUG("Creating vulkan graphics pipeline");

    VkShaderModule vert_shader_module = create_shader_module(
        vk_context, reinterpret_cast<const char *>(shader->vertex_shader_code.data), shader->vertex_shader_code.size());

    VkShaderModule frag_shader_module = create_shader_module(
        vk_context, reinterpret_cast<const char *>(shader->fragment_shader_code.data), shader->fragment_shader_code.size());

    // create the pipeline shader stage
    u32                   shader_stage_count = 2;
    VkShaderStageFlagBits stage_flag_bits[2] = {VK_SHADER_STAGE_VERTEX_BIT, VK_SHADER_STAGE_FRAGMENT_BIT};

    VkShaderModule shader_modules[2] = {vert_shader_module, frag_shader_module};

    VkPipelineShaderStageCreateInfo shader_stage_create_infos[2]{};

    for (u32 i = 0; i < shader_stage_count; i++)
    {
        VkPipelineShaderStageCreateInfo shader_stage_create_info{};
        shader_stage_create_info.sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        shader_stage_create_info.pNext  = 0;
        shader_stage_create_info.flags  = 0;
        shader_stage_create_info.stage  = stage_flag_bits[i];
        shader_stage_create_info.module = shader_modules[i];
        shader_stage_create_info.pName  = "main";

        shader_stage_create_infos[i] = shader_stage_create_info;
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

    u32                             vertex_input_attribute_descriptions_count = 3;
    VkVertexInputBindingDescription vertex_input_binding_description{};
    vertex_input_binding_description.binding   = 0;
    vertex_input_binding_description.stride    = sizeof(vertex);
    vertex_input_binding_description.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    VkVertexInputAttributeDescription vertex_input_attribute_descriptions[3]{};
    VkFormat vertex_input_attribute_formats[3] = {VK_FORMAT_R32G32B32_SFLOAT, VK_FORMAT_R32G32B32_SFLOAT,
                                                  VK_FORMAT_R32G32_SFLOAT};

    // position
    vertex_input_attribute_descriptions[0].location = 0;
    vertex_input_attribute_descriptions[0].binding  = 0;
    vertex_input_attribute_descriptions[0].format   = vertex_input_attribute_formats[0];
    vertex_input_attribute_descriptions[0].offset   = 0;

    // normals
    vertex_input_attribute_descriptions[1].location = 1;
    vertex_input_attribute_descriptions[1].binding  = 0;
    vertex_input_attribute_descriptions[1].format   = vertex_input_attribute_formats[1];
    vertex_input_attribute_descriptions[1].offset   = sizeof(math::vec3) * 1;

    // texture coordinates
    vertex_input_attribute_descriptions[2].location = 2;
    vertex_input_attribute_descriptions[2].binding  = 0;
    vertex_input_attribute_descriptions[2].format   = vertex_input_attribute_formats[2];
    vertex_input_attribute_descriptions[2].offset   = sizeof(math::vec3) * 2;

    VkPipelineVertexInputStateCreateInfo vertex_input_state_create_info{};

    vertex_input_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertex_input_state_create_info.pNext = 0;
    vertex_input_state_create_info.flags = 0;
    vertex_input_state_create_info.vertexBindingDescriptionCount   = 1;
    vertex_input_state_create_info.pVertexBindingDescriptions      = &vertex_input_binding_description;
    vertex_input_state_create_info.vertexAttributeDescriptionCount = vertex_input_attribute_descriptions_count;
    vertex_input_state_create_info.pVertexAttributeDescriptions    = vertex_input_attribute_descriptions;

    VkPipelineInputAssemblyStateCreateInfo input_assembly_state_create_info{};
    input_assembly_state_create_info.sType    = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    input_assembly_state_create_info.pNext    = 0;
    input_assembly_state_create_info.flags    = 0;
    input_assembly_state_create_info.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    input_assembly_state_create_info.primitiveRestartEnable = VK_FALSE;

    VkViewport view_port{};
    view_port.x        = 0.0f;
    view_port.y        = 0.0f;
    view_port.width    = static_cast<f32>(vk_context->vk_swapchain.surface_extent.width);
    view_port.height   = static_cast<f32>(vk_context->vk_swapchain.surface_extent.width);
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

    VkPipelineRasterizationStateCreateInfo rasterization_state_create_info{};
    rasterization_state_create_info.sType            = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterization_state_create_info.pNext            = 0;
    rasterization_state_create_info.flags            = 0;
    rasterization_state_create_info.depthClampEnable = VK_FALSE;
    rasterization_state_create_info.rasterizerDiscardEnable = VK_FALSE;
    rasterization_state_create_info.polygonMode             = VK_POLYGON_MODE_FILL;
    rasterization_state_create_info.cullMode                = VK_CULL_MODE_BACK_BIT;
    rasterization_state_create_info.frontFace               = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterization_state_create_info.depthBiasEnable         = VK_FALSE;
    rasterization_state_create_info.depthBiasConstantFactor = 0.0f;
    rasterization_state_create_info.depthBiasClamp          = 0.0f;
    rasterization_state_create_info.depthBiasSlopeFactor    = 0.0f;
    rasterization_state_create_info.lineWidth               = 1.0f;

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

    VkPipelineDepthStencilStateCreateInfo depth_stencil_state_create_info{};
    depth_stencil_state_create_info.sType                 = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depth_stencil_state_create_info.pNext                 = 0;
    depth_stencil_state_create_info.flags                 = 0;
    depth_stencil_state_create_info.depthTestEnable       = VK_TRUE;
    depth_stencil_state_create_info.depthWriteEnable      = VK_TRUE;
    depth_stencil_state_create_info.depthCompareOp        = VK_COMPARE_OP_LESS;
    depth_stencil_state_create_info.depthBoundsTestEnable = VK_FALSE;
    depth_stencil_state_create_info.minDepthBounds        = 0.0f;
    depth_stencil_state_create_info.maxDepthBounds        = 1.0f;
    depth_stencil_state_create_info.stencilTestEnable     = VK_FALSE;
    depth_stencil_state_create_info.front                 = {};
    depth_stencil_state_create_info.back                  = {};

    // global global_descriptor_set_ubo_layout_binding
    u32                          global_descriptor_binding_count = 2;
    VkDescriptorSetLayoutBinding global_descriptor_set_layout_bindings[2]{};
    VkShaderStageFlags           global_stage_flags[2] = {VK_SHADER_STAGE_VERTEX_BIT, VK_SHADER_STAGE_FRAGMENT_BIT};

    for (u32 i = 0; i < 2; i++)
    {
        global_descriptor_set_layout_bindings[i].binding            = i;
        global_descriptor_set_layout_bindings[i].descriptorType     = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        global_descriptor_set_layout_bindings[i].descriptorCount    = 1;
        global_descriptor_set_layout_bindings[i].stageFlags         = global_stage_flags[i];
        global_descriptor_set_layout_bindings[i].pImmutableSamplers = 0;
    }

    VkDescriptorSetLayoutCreateInfo global_descriptor_set_layout_create_info{};
    global_descriptor_set_layout_create_info.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    global_descriptor_set_layout_create_info.pNext        = 0;
    global_descriptor_set_layout_create_info.flags        = 0;
    global_descriptor_set_layout_create_info.bindingCount = global_descriptor_binding_count;
    global_descriptor_set_layout_create_info.pBindings    = global_descriptor_set_layout_bindings;

    VkResult result =
        vkCreateDescriptorSetLayout(vk_context->vk_device.logical, &global_descriptor_set_layout_create_info,
                                    vk_context->vk_allocator, &shader->per_frame_descriptor_layout);
    VK_CHECK(result);

    // material_descriptor_set_layout_binding
    u32                          material_descriptors_binding_count = 2;
    VkDescriptorSetLayoutBinding material_descriptor_set_layout_bindings[2]{};
    VkShaderStageFlags           stage_flags[2] = {VK_SHADER_STAGE_FRAGMENT_BIT, VK_SHADER_STAGE_FRAGMENT_BIT};
    VkDescriptorType             des_types[2]   = {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                                                   VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER};

    for (u32 i = 0; i < material_descriptors_binding_count; i++)
    {
        material_descriptor_set_layout_bindings[i].binding            = i;
        material_descriptor_set_layout_bindings[i].descriptorCount    = 1;
        material_descriptor_set_layout_bindings[i].descriptorType     = des_types[i];
        material_descriptor_set_layout_bindings[i].stageFlags         = stage_flags[i];
        material_descriptor_set_layout_bindings[i].pImmutableSamplers = nullptr;
    }

    VkDescriptorSetLayoutCreateInfo material_descriptor_set_layout_create_info{};
    material_descriptor_set_layout_create_info.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    material_descriptor_set_layout_create_info.pNext        = 0;
    material_descriptor_set_layout_create_info.flags        = 0;
    material_descriptor_set_layout_create_info.bindingCount = material_descriptors_binding_count;
    material_descriptor_set_layout_create_info.pBindings    = material_descriptor_set_layout_bindings;

    result = vkCreateDescriptorSetLayout(vk_context->vk_device.logical, &material_descriptor_set_layout_create_info,
                                         vk_context->vk_allocator, &shader->per_group_descriptor_layout);
    VK_CHECK(result);

    VkDescriptorSetLayout set_layouts[2] = {shader->per_frame_descriptor_layout,
                                            shader->per_group_descriptor_layout};

    // object specific push constants
    VkPushConstantRange object_push_constant_range{};
    object_push_constant_range.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    object_push_constant_range.offset     = 0;

    DASSERT_MSG(sizeof(vk_push_constant) == 128, "Object uniform buffer must be 128 bytes wide.");
    object_push_constant_range.size = sizeof(vk_push_constant);

    VkPipelineLayoutCreateInfo pipeline_layout_create_info{};

    pipeline_layout_create_info.sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipeline_layout_create_info.pNext                  = 0;
    pipeline_layout_create_info.flags                  = 0;
    pipeline_layout_create_info.setLayoutCount         = 2;
    pipeline_layout_create_info.pSetLayouts            = set_layouts;
    pipeline_layout_create_info.pushConstantRangeCount = 1;
    pipeline_layout_create_info.pPushConstantRanges    = &object_push_constant_range;

    result = vkCreatePipelineLayout(vk_context->vk_device.logical, &pipeline_layout_create_info,
                                    vk_context->vk_allocator, &vk_context->vk_graphics_pipeline.layout);
    VK_CHECK(result);

    VkGraphicsPipelineCreateInfo graphics_pipeline_create_info{};
    graphics_pipeline_create_info.sType               = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    graphics_pipeline_create_info.pNext               = 0;
    graphics_pipeline_create_info.flags               = 0;
    graphics_pipeline_create_info.stageCount          = shader_stage_count;
    graphics_pipeline_create_info.pStages             = shader_stage_create_infos;
    graphics_pipeline_create_info.pVertexInputState   = &vertex_input_state_create_info;
    graphics_pipeline_create_info.pInputAssemblyState = &input_assembly_state_create_info;
    graphics_pipeline_create_info.pTessellationState  = nullptr;
    graphics_pipeline_create_info.pViewportState      = &viewport_state_create_info;
    graphics_pipeline_create_info.pRasterizationState = &rasterization_state_create_info;
    graphics_pipeline_create_info.pMultisampleState   = &multisampling_state_create_info;
    graphics_pipeline_create_info.pDepthStencilState  = &depth_stencil_state_create_info;
    graphics_pipeline_create_info.pColorBlendState    = &color_blend_state_create_info;
    graphics_pipeline_create_info.pDynamicState       = &dynamic_state_create_info;
    graphics_pipeline_create_info.layout              = vk_context->vk_graphics_pipeline.layout;
    graphics_pipeline_create_info.renderPass          = vk_context->vk_renderpass;
    graphics_pipeline_create_info.subpass             = 0;
    graphics_pipeline_create_info.basePipelineHandle  = VK_NULL_HANDLE;
    graphics_pipeline_create_info.basePipelineIndex   = -1;

    result = vkCreateGraphicsPipelines(vk_context->vk_device.logical, VK_NULL_HANDLE, 1, &graphics_pipeline_create_info,
                                       vk_context->vk_allocator, &vk_context->vk_graphics_pipeline.handle);
    VK_CHECK(result);

    vkDestroyShaderModule(vk_context->vk_device.logical, vert_shader_module, vk_context->vk_allocator);
    vkDestroyShaderModule(vk_context->vk_device.logical, frag_shader_module, vk_context->vk_allocator);

    return true;
}

VkShaderModule create_shader_module(vulkan_context *vk_context, const char *shader_code, u64 shader_code_size)
{
    VkShaderModuleCreateInfo shader_module_create_info{};

    shader_module_create_info.sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    shader_module_create_info.pNext    = 0;
    shader_module_create_info.flags    = 0;
    shader_module_create_info.codeSize = shader_code_size;
    shader_module_create_info.pCode    = reinterpret_cast<u32 *>(const_cast<char *>(shader_code));

    VkShaderModule shader_module{};

    VkResult result = vkCreateShaderModule(vk_context->vk_device.logical, &shader_module_create_info,
                                           vk_context->vk_allocator, &shader_module);
    VK_CHECK(result);

    return shader_module;
}
