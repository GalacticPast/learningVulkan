#include "vulkan_pipeline.hpp"
#include "containers/darray.hpp"
#include "core/logger.hpp"
#include "math/dmath_types.hpp"
#include "renderer/vulkan/vulkan_types.hpp"
#include "resources/resource_types.hpp"
#include "vulkan/vulkan_core.h"

VkShaderModule create_shader_module(vulkan_context *vk_context, const char *shader_code, u64 shader_code_size);

bool vulkan_create_pipeline(vulkan_context *vk_context, vulkan_shader *shader)
{
    DDEBUG("Creating vulkan graphics pipeline");

    VkShaderModule frag_shader_module =
        create_shader_module(vk_context, reinterpret_cast<const char *>(shader->fragment_shader_code.data),
                             shader->fragment_shader_code.size());

    // create the pipeline shader stage
    arena                        *arena              = vk_context->arena;
    u32                           shader_stage_count = 0;
    darray<VkShaderStageFlagBits> stage_flag_bits    = {};
    stage_flag_bits.c_init(arena);

    VkShaderModule vert_shader_module = create_shader_module(
        vk_context, reinterpret_cast<const char *>(shader->vertex_shader_code.data), shader->vertex_shader_code.size());

    // INFO: this is redundant
    u32 num_stages = shader->stages.size();
    for (u32 i = 0; i < num_stages; i++)
    {
        if (shader->stages[i] & STAGE_VERTEX)
        {
            shader_stage_count++;
            stage_flag_bits.push_back(VK_SHADER_STAGE_VERTEX_BIT);
        }
        else if (shader->stages[i] & STAGE_FRAGMENT)
        {
            shader_stage_count++;
            stage_flag_bits.push_back(VK_SHADER_STAGE_FRAGMENT_BIT);
        }
    }

    DASSERT_MSG(shader_stage_count, "There cannot be 0 shader stages for a pipeline");

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

    VkPipelineVertexInputStateCreateInfo vertex_input_state_create_info{};

    vertex_input_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertex_input_state_create_info.pNext = 0;
    vertex_input_state_create_info.flags = 0;
    vertex_input_state_create_info.vertexBindingDescriptionCount   = 1;
    vertex_input_state_create_info.pVertexBindingDescriptions      = &shader->attribute_description;
    vertex_input_state_create_info.vertexAttributeDescriptionCount = shader->input_attribute_descriptions.size();
    vertex_input_state_create_info.pVertexAttributeDescriptions    = shader->input_attribute_descriptions.data;

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

    VkPipelineRasterizationStateCreateInfo rasterizer_create_info = {};
    rasterizer_create_info.sType                   = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer_create_info.depthClampEnable        = VK_FALSE;
    rasterizer_create_info.rasterizerDiscardEnable = VK_FALSE;
    rasterizer_create_info.polygonMode             = VK_POLYGON_MODE_FILL;
    rasterizer_create_info.lineWidth               = 1.0f;

    if (shader->pipeline_configuration.mode == CULL_FRONT_BIT)
    {
        rasterizer_create_info.cullMode = VK_CULL_MODE_FRONT_BIT;
    }
    else if (shader->pipeline_configuration.mode == CULL_BACK_BIT)
    {
        rasterizer_create_info.cullMode = VK_CULL_MODE_BACK_BIT;
    }
    else
    {
        rasterizer_create_info.cullMode = VK_CULL_MODE_NONE;
    }

    rasterizer_create_info.frontFace               = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterizer_create_info.depthBiasEnable         = VK_FALSE;
    rasterizer_create_info.depthBiasConstantFactor = 0.0f;
    rasterizer_create_info.depthBiasClamp          = 0.0f;
    rasterizer_create_info.depthBiasSlopeFactor    = 0.0f;

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

    pipeline_color_blend_state color_blend_options = shader->pipeline_configuration.color_blend;

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

    if (color_blend_options.enable_color_blend)
    {
        color_blend_attachment_state.blendEnable         = VK_TRUE;
        // TODO: better checking
        // HACK:
        // FIXME
        color_blend_attachment_state.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
        color_blend_attachment_state.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    }

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
    depth_stencil_state_create_info.sType            = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depth_stencil_state_create_info.pNext            = 0;
    depth_stencil_state_create_info.flags            = 0;
    depth_stencil_state_create_info.depthTestEnable  = VK_TRUE;
    depth_stencil_state_create_info.depthWriteEnable = VK_TRUE;
    depth_stencil_state_create_info.depthCompareOp   = VK_COMPARE_OP_LESS;

    if (shader->pipeline_configuration.mode == CULL_NONE_BIT)
    {
        depth_stencil_state_create_info.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
    }

    depth_stencil_state_create_info.depthBoundsTestEnable = VK_FALSE;
    depth_stencil_state_create_info.minDepthBounds        = 0.0f;
    depth_stencil_state_create_info.maxDepthBounds        = 1.0f;
    depth_stencil_state_create_info.stencilTestEnable     = VK_FALSE;
    depth_stencil_state_create_info.front                 = {};
    depth_stencil_state_create_info.back                  = {};

    if (!shader->pipeline_configuration.depth_state.enable_depth_blend)
    {
        depth_stencil_state_create_info.depthTestEnable  = VK_FALSE;
        depth_stencil_state_create_info.depthWriteEnable = VK_FALSE;
        depth_stencil_state_create_info.depthCompareOp   = VK_COMPARE_OP_NEVER;
    }

    // global global_descriptor_set_ubo_layout_binding

    // material_descriptor_set_layout_binding

    VkDescriptorSetLayout set_layouts[2] = {shader->per_frame_descriptor_layout, shader->per_group_descriptor_layout};

    // object specific push constants
    VkPushConstantRange object_push_constant_range{};
    object_push_constant_range.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    object_push_constant_range.offset     = 0;

    STATIC_ASSERT(sizeof(vk_push_constant) == 128, "Object uniform buffer must be 128 bytes wide.");
    object_push_constant_range.size = sizeof(vk_push_constant);

    u32 layout_count = 0;

    for (u32 i = 0; i < 2; i++)
    {
        if (set_layouts[i])
        {
            layout_count++;
        }
    }

    VkPipelineLayoutCreateInfo pipeline_layout_create_info{};

    pipeline_layout_create_info.sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipeline_layout_create_info.pNext                  = 0;
    pipeline_layout_create_info.flags                  = 0;
    pipeline_layout_create_info.setLayoutCount         = layout_count;
    pipeline_layout_create_info.pSetLayouts            = set_layouts;
    pipeline_layout_create_info.pushConstantRangeCount = 1;
    pipeline_layout_create_info.pPushConstantRanges    = &object_push_constant_range;

    VkResult result = vkCreatePipelineLayout(vk_context->vk_device.logical, &pipeline_layout_create_info,
                                             vk_context->vk_allocator, &shader->pipeline.layout);
    VK_CHECK(result);

    VkGraphicsPipelineCreateInfo pipeline_create_info{};
    pipeline_create_info.sType               = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipeline_create_info.pNext               = 0;
    pipeline_create_info.flags               = 0;
    pipeline_create_info.stageCount          = shader_stage_count;
    pipeline_create_info.pStages             = shader_stage_create_infos;
    pipeline_create_info.pVertexInputState   = &vertex_input_state_create_info;
    pipeline_create_info.pInputAssemblyState = &input_assembly_state_create_info;
    pipeline_create_info.pTessellationState  = nullptr;
    pipeline_create_info.pViewportState      = &viewport_state_create_info;
    pipeline_create_info.pRasterizationState = &rasterizer_create_info;
    pipeline_create_info.pMultisampleState   = &multisampling_state_create_info;
    pipeline_create_info.pDepthStencilState  = &depth_stencil_state_create_info;
    pipeline_create_info.pColorBlendState    = &color_blend_state_create_info;
    pipeline_create_info.pDynamicState       = &dynamic_state_create_info;
    pipeline_create_info.layout              = shader->pipeline.layout;

    if (shader->renderpass_type == WORLD_RENDERPASS)
    {
        pipeline_create_info.renderPass = vk_context->world_renderpass.handle;
    }
    else if (shader->renderpass_type == UI_RENDERPASS)
    {
        pipeline_create_info.renderPass = vk_context->ui_renderpass.handle;
    }
    else
    {
        DERROR("Uknown Renderpass type %d", shader->renderpass_type);
        return false;
    }

    pipeline_create_info.subpass            = 0;
    pipeline_create_info.basePipelineHandle = VK_NULL_HANDLE;
    pipeline_create_info.basePipelineIndex  = -1;

    result = vkCreateGraphicsPipelines(vk_context->vk_device.logical, VK_NULL_HANDLE, 1, &pipeline_create_info,
                                       vk_context->vk_allocator, &shader->pipeline.handle);
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
