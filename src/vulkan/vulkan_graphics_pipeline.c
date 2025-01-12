#include "vulkan_graphics_pipeline.h"
#include "containers/array.h"
#include "core/fileIO.h"
#include "math/math_types.h"

void create_shader_module(vulkan_context *context, VkShaderModule *shader_module, const char *binary_code);

b8 vulkan_create_graphics_pipeline(vulkan_context *context)
{
    INFO("Creating vulkan graphics pipeline...");

    VkShaderModule vert_shader_module = {0};
    char          *vert_shader_code   = read_file("vert_shader.spv", true);
    create_shader_module(context, &vert_shader_module, vert_shader_code);

    VkShaderModule frag_shader_module = {0};
    char          *frag_shader_code   = read_file("frag_shader.spv", true);
    create_shader_module(context, &frag_shader_module, frag_shader_code);

    VkPipelineShaderStageCreateInfo pipeline_vertex_shader_stage_create_info = {};

    pipeline_vertex_shader_stage_create_info.sType               = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    pipeline_vertex_shader_stage_create_info.pNext               = 0;
    pipeline_vertex_shader_stage_create_info.flags               = 0;
    pipeline_vertex_shader_stage_create_info.stage               = VK_SHADER_STAGE_VERTEX_BIT;
    pipeline_vertex_shader_stage_create_info.module              = vert_shader_module;
    pipeline_vertex_shader_stage_create_info.pName               = "main";
    pipeline_vertex_shader_stage_create_info.pSpecializationInfo = 0;

    VkPipelineShaderStageCreateInfo pipeline_fragment_shader_stage_create_info = {};

    pipeline_fragment_shader_stage_create_info.sType               = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    pipeline_fragment_shader_stage_create_info.pNext               = 0;
    pipeline_fragment_shader_stage_create_info.flags               = 0;
    pipeline_fragment_shader_stage_create_info.stage               = VK_SHADER_STAGE_FRAGMENT_BIT;
    pipeline_fragment_shader_stage_create_info.module              = frag_shader_module;
    pipeline_fragment_shader_stage_create_info.pName               = "main";
    pipeline_fragment_shader_stage_create_info.pSpecializationInfo = 0;

    VkPipelineShaderStageCreateInfo shader_create_infos[2] = {};
    shader_create_infos[0]                                 = pipeline_vertex_shader_stage_create_info;
    shader_create_infos[1]                                 = pipeline_fragment_shader_stage_create_info;

    VkDynamicState dynamic_states[2] = {};
    dynamic_states[0]                = VK_DYNAMIC_STATE_VIEWPORT;
    dynamic_states[1]                = VK_DYNAMIC_STATE_SCISSOR;

    VkPipelineDynamicStateCreateInfo dynamic_state_create_info = {};

    dynamic_state_create_info.sType             = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamic_state_create_info.pNext             = 0;
    dynamic_state_create_info.flags             = 0;
    dynamic_state_create_info.dynamicStateCount = 2;
    dynamic_state_create_info.pDynamicStates    = dynamic_states;

    VkVertexInputBindingDescription vertex_binding_description = {};
    vertex_binding_description.binding                         = 0;
    vertex_binding_description.stride                          = sizeof(vertex_3d);
    vertex_binding_description.inputRate                       = VK_VERTEX_INPUT_RATE_VERTEX;

    VkVertexInputAttributeDescription vertex_attribute_descriptions[2] = {};

    for (u32 i = 0; i < 2; i++)
    {
        vertex_attribute_descriptions[i].location = i;
        vertex_attribute_descriptions[i].binding  = 0;
        vertex_attribute_descriptions[i].format   = VK_FORMAT_R32G32B32_SFLOAT;
        vertex_attribute_descriptions[i].offset   = i == 0 ? 0 : sizeof(vec3);
    }

    VkPipelineVertexInputStateCreateInfo vertex_input_state_create_info = {};
    vertex_input_state_create_info.sType                                = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertex_input_state_create_info.pNext                                = 0;
    vertex_input_state_create_info.flags                                = 0;
    vertex_input_state_create_info.vertexBindingDescriptionCount        = 1;
    vertex_input_state_create_info.pVertexBindingDescriptions           = &vertex_binding_description;
    vertex_input_state_create_info.vertexAttributeDescriptionCount      = 2;
    vertex_input_state_create_info.pVertexAttributeDescriptions         = vertex_attribute_descriptions;

    VkPipelineInputAssemblyStateCreateInfo input_assembly_state_create_info = {};
    input_assembly_state_create_info.sType                                  = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    input_assembly_state_create_info.pNext                                  = 0;
    input_assembly_state_create_info.flags                                  = 0;
    input_assembly_state_create_info.topology                               = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    input_assembly_state_create_info.primitiveRestartEnable                 = VK_FALSE;

    VkViewport view_port_info = {};
    view_port_info.x          = 0;
    view_port_info.y          = 0;
    view_port_info.width      = (f32)context->frame_buffer_width;
    view_port_info.height     = (f32)context->frame_buffer_height;
    view_port_info.minDepth   = 0.0f;
    view_port_info.maxDepth   = 1.0f;

    VkRect2D   scissor_info = {};
    VkOffset2D offset       = {0, 0};
    scissor_info.offset     = offset;
    scissor_info.extent     = context->swapchain.extent2D;

    VkPipelineViewportStateCreateInfo view_port_state_create_info = {};

    view_port_state_create_info.sType         = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    view_port_state_create_info.pNext         = 0;
    view_port_state_create_info.flags         = 0;
    view_port_state_create_info.viewportCount = 1;
    view_port_state_create_info.pViewports    = &view_port_info;
    view_port_state_create_info.scissorCount  = 1;
    view_port_state_create_info.pScissors     = &scissor_info;

    VkPipelineRasterizationStateCreateInfo rasterizer_state_create_info = {};
    rasterizer_state_create_info.sType                                  = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer_state_create_info.pNext                                  = 0;
    rasterizer_state_create_info.flags                                  = 0;
    rasterizer_state_create_info.depthClampEnable                       = VK_FALSE;
    rasterizer_state_create_info.rasterizerDiscardEnable                = VK_FALSE;
    rasterizer_state_create_info.polygonMode                            = VK_POLYGON_MODE_FILL;
    rasterizer_state_create_info.cullMode                               = VK_CULL_MODE_BACK_BIT;
    rasterizer_state_create_info.frontFace                              = VK_FRONT_FACE_CLOCKWISE;
    rasterizer_state_create_info.depthBiasEnable                        = VK_FALSE;
    rasterizer_state_create_info.depthBiasConstantFactor                = 0;
    rasterizer_state_create_info.depthBiasClamp                         = 0;
    rasterizer_state_create_info.depthBiasSlopeFactor                   = 0;
    rasterizer_state_create_info.lineWidth                              = 1.0f;

    VkPipelineMultisampleStateCreateInfo multisampling_state_create_info = {};
    multisampling_state_create_info.sType                                = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling_state_create_info.pNext                                = 0;
    multisampling_state_create_info.flags                                = 0;
    multisampling_state_create_info.rasterizationSamples                 = VK_SAMPLE_COUNT_1_BIT;
    multisampling_state_create_info.sampleShadingEnable                  = VK_FALSE;
    multisampling_state_create_info.minSampleShading                     = 1.0f;
    multisampling_state_create_info.pSampleMask                          = VK_NULL_HANDLE;
    multisampling_state_create_info.alphaToCoverageEnable                = VK_FALSE;
    multisampling_state_create_info.alphaToOneEnable                     = VK_FALSE;

    VkPipelineColorBlendAttachmentState color_blend_attachment_state = {};
    color_blend_attachment_state.blendEnable                         = VK_FALSE;
    color_blend_attachment_state.srcColorBlendFactor                 = VK_BLEND_FACTOR_ONE;
    color_blend_attachment_state.dstColorBlendFactor                 = VK_BLEND_FACTOR_ZERO;
    color_blend_attachment_state.colorBlendOp                        = VK_BLEND_OP_ADD;
    color_blend_attachment_state.srcAlphaBlendFactor                 = VK_BLEND_FACTOR_ONE;
    color_blend_attachment_state.dstAlphaBlendFactor                 = VK_BLEND_FACTOR_ZERO;
    color_blend_attachment_state.alphaBlendOp                        = VK_BLEND_OP_ADD;
    color_blend_attachment_state.colorWriteMask                      = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

    VkPipelineColorBlendStateCreateInfo color_blend_state_create_info = {};
    color_blend_state_create_info.sType                               = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    color_blend_state_create_info.pNext                               = 0;
    color_blend_state_create_info.flags                               = 0;
    color_blend_state_create_info.logicOpEnable                       = VK_FALSE;
    color_blend_state_create_info.logicOp                             = VK_LOGIC_OP_COPY;
    color_blend_state_create_info.attachmentCount                     = 1;
    color_blend_state_create_info.pAttachments                        = &color_blend_attachment_state;
    color_blend_state_create_info.blendConstants[0]                   = 0;
    color_blend_state_create_info.blendConstants[1]                   = 0;
    color_blend_state_create_info.blendConstants[2]                   = 0;
    color_blend_state_create_info.blendConstants[3]                   = 0;

    VkPipelineLayoutCreateInfo pipeline_layout_create_info = {};
    pipeline_layout_create_info.sType                      = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipeline_layout_create_info.pNext                      = 0;
    pipeline_layout_create_info.flags                      = 0;
    pipeline_layout_create_info.setLayoutCount             = 0;
    pipeline_layout_create_info.pSetLayouts                = VK_NULL_HANDLE;
    pipeline_layout_create_info.pushConstantRangeCount     = 0;
    pipeline_layout_create_info.pPushConstantRanges        = VK_NULL_HANDLE;

    VK_CHECK(vkCreatePipelineLayout(context->device.logical, &pipeline_layout_create_info, 0, &context->graphics_pipeline.layout));

    VkGraphicsPipelineCreateInfo pipeline_create_info = {};

    pipeline_create_info.sType               = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipeline_create_info.pNext               = 0;
    pipeline_create_info.flags               = 0;
    pipeline_create_info.stageCount          = 2;
    pipeline_create_info.pStages             = shader_create_infos;
    pipeline_create_info.pVertexInputState   = &vertex_input_state_create_info;
    pipeline_create_info.pInputAssemblyState = &input_assembly_state_create_info;
    pipeline_create_info.pTessellationState  = VK_NULL_HANDLE;
    pipeline_create_info.pViewportState      = &view_port_state_create_info;
    pipeline_create_info.pRasterizationState = &rasterizer_state_create_info;
    pipeline_create_info.pMultisampleState   = &multisampling_state_create_info;
    pipeline_create_info.pDepthStencilState  = VK_NULL_HANDLE;
    pipeline_create_info.pColorBlendState    = &color_blend_state_create_info;
    pipeline_create_info.pDynamicState       = &dynamic_state_create_info;
    pipeline_create_info.layout              = context->graphics_pipeline.layout;
    pipeline_create_info.renderPass          = context->renderpass;
    pipeline_create_info.subpass             = 0;
    pipeline_create_info.basePipelineHandle  = VK_NULL_HANDLE;
    pipeline_create_info.basePipelineIndex   = -1;

    VK_CHECK(vkCreateGraphicsPipelines(context->device.logical, VK_NULL_HANDLE, 1, &pipeline_create_info, 0, &context->graphics_pipeline.handle));

    DEBUG("Destroying shader modules...");
    array_destroy(vert_shader_code);
    array_destroy(frag_shader_code);

    vkDestroyShaderModule(context->device.logical, vert_shader_module, 0);
    vkDestroyShaderModule(context->device.logical, frag_shader_module, 0);
    DEBUG("Shader modules destroyed.");

    INFO("Graphics pipeline created");
    return true;
}

void create_shader_module(vulkan_context *context, VkShaderModule *shader_module, const char *binary_code)
{

    VkShaderModuleCreateInfo shader_module_create_info = {};

    shader_module_create_info.sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    shader_module_create_info.pNext    = 0;
    shader_module_create_info.flags    = 0;
    u32 code_size                      = (u32)array_get_length((char *)binary_code);
    shader_module_create_info.codeSize = code_size;
    shader_module_create_info.pCode    = (const u32 *)binary_code;
    DEBUG("Creaating shader module...");

    VK_CHECK(vkCreateShaderModule(context->device.logical, &shader_module_create_info, 0, shader_module));

    DEBUG("Shader module created.");
}
