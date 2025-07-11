#include "core/dfile_system.hpp"
#include "core/dmemory.hpp"
#include "core/dstring.hpp"
#include "core/logger.hpp"
#include "defines.hpp"

#include "math/dmath_types.hpp"
#include "resources/geometry_system.hpp"
#include "resources/material_system.hpp"
#include "resources/resource_types.hpp"
#include "resources/shader_system.hpp"

#include "vulkan/vulkan_core.h"
#include "vulkan_backend.hpp"
#include "vulkan_buffers.hpp"
#include "vulkan_device.hpp"
#include "vulkan_image.hpp"
#include "vulkan_pipeline.hpp"
#include "vulkan_platform.hpp"
#include "vulkan_renderpass.hpp"
#include "vulkan_swapchain.hpp"
#include "vulkan_types.hpp"

static vulkan_context *vk_context;

VkBool32 vulkan_dbg_msg_rprt_callback(VkDebugUtilsMessageSeverityFlagBitsEXT      messageSeverity,
                                      VkDebugUtilsMessageTypeFlagsEXT             messageTypes,
                                      const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData, void *pUserData);

bool vulkan_check_validation_layer_support();
bool vulkan_create_debug_messenger(VkDebugUtilsMessengerCreateInfoEXT *dbg_messenger_create_info);
bool vulkan_create_default_texture();
bool vulkan_allocate_descriptor_set(VkDescriptorPool *desc_pool, VkDescriptorSetLayout *set_layout,
                                    u32 descriptor_count);
bool vulkan_create_framebuffers(vulkan_context *vk_context);

bool vulkan_backend_initialize(arena *arena, u64 *vulkan_backend_memory_requirements, application_config *app_config,
                               void *state)
{
    *vulkan_backend_memory_requirements = sizeof(vulkan_context);
    if (!state)
    {
        return true;
    }
    DDEBUG("Initializing Vulkan backend...");
    vk_context               = reinterpret_cast<vulkan_context *>(state);
    vk_context->vk_allocator = nullptr;
    vk_context->arena        = arena;

    {
        vk_context->command_buffers.reserve(arena, MAX_FRAMES_IN_FLIGHT);
    }

    DDEBUG("Creating vulkan instance...");

    VkApplicationInfo app_info{};

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Weverything"
    // because the VK macros have old c style cast in them , the compiler cries about it and doesnt compile, well tbh
    // its case I have the dont allow old c style cast enabled.
    app_info.sType              = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    app_info.pNext              = nullptr;
    app_info.pApplicationName   = app_config->application_name;
    app_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    app_info.pEngineName        = app_config->application_name;
    app_info.engineVersion      = VK_MAKE_VERSION(1, 0, 0);
    app_info.apiVersion         = VK_API_VERSION_1_3;
#pragma clang diagnostic pop

    bool enable_validation_layers = false;
    bool validation_layer_support = false;

#ifdef DEBUG

    enable_validation_layers = true;

    // vulkan debug messenger
    validation_layer_support = vulkan_check_validation_layer_support();
    if (!validation_layer_support && enable_validation_layers)
    {
        DERROR("Validation layer support requsted, but not available.");
        return false;
    }
#endif

    vk_context->enabled_layer_count = 0;

    u32 index = 0;
    if (enable_validation_layers && validation_layer_support)
    {
        vk_context->enabled_layer_count++;
        vk_context->enabled_layer_names[index++] = "VK_LAYER_KHRONOS_validation";
    }

    // INFO: get platform specifig extensions and extensions count
    darray<const char *> vulkan_instance_extensions;
    vulkan_instance_extensions.c_init(arena);
    const char *vk_generic_surface_ext = VK_KHR_SURFACE_EXTENSION_NAME;

    vulkan_instance_extensions.push_back(vk_generic_surface_ext);
    vulkan_platform_get_required_vulkan_extensions(vulkan_instance_extensions);

#if DEBUG
    const char *vk_debug_uitls  = VK_EXT_DEBUG_UTILS_EXTENSION_NAME;
    const char *vk_debug_report = VK_EXT_DEBUG_REPORT_EXTENSION_NAME;

    vulkan_instance_extensions.push_back(vk_debug_uitls);
    vulkan_instance_extensions.push_back(vk_debug_report);
#endif

    u32                   dbg_vulkan_instance_extensions_count = 0;
    VkExtensionProperties dbg_instance_extension_properties[256];

    vkEnumerateInstanceExtensionProperties(0, &dbg_vulkan_instance_extensions_count, 0);
    vkEnumerateInstanceExtensionProperties(0, &dbg_vulkan_instance_extensions_count, dbg_instance_extension_properties);

    // check for extensions support
#ifdef DEBUG
    {

        for (u32 i = 0; i < dbg_vulkan_instance_extensions_count; i++)
        {
            DDEBUG("Extension Name: %s", dbg_instance_extension_properties[i].extensionName);
        }
    }
#endif

    for (u32 i = 0; i < vulkan_instance_extensions.size(); i++)
    {
        bool        found_ext = false;
        const char *str0      = vulkan_instance_extensions[i];

        for (u32 j = 0; j < dbg_vulkan_instance_extensions_count; j++)
        {
            if (string_compare(str0, dbg_instance_extension_properties[j].extensionName))
            {
                found_ext = true;
                break;
            }
        }
        if (found_ext == false)
        {
            DERROR("Extension Name: %s not found", vulkan_instance_extensions[i]);
            return false;
        }
    }

    // Instance info
    VkDebugUtilsMessengerCreateInfoEXT dbg_messenger_create_info{};

#ifdef DEBUG
    vulkan_create_debug_messenger(&dbg_messenger_create_info);
#endif

    VkInstanceCreateInfo inst_create_info{};

    inst_create_info.sType                   = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    inst_create_info.pNext                   = 0;
    inst_create_info.flags                   = 0;
    inst_create_info.enabledLayerCount       = vk_context->enabled_layer_count;
    inst_create_info.ppEnabledLayerNames     = vk_context->enabled_layer_names;
    inst_create_info.enabledExtensionCount   = static_cast<u32>(vulkan_instance_extensions.size());
    inst_create_info.ppEnabledExtensionNames = reinterpret_cast<const char **>(vulkan_instance_extensions.data);
    inst_create_info.pApplicationInfo        = &app_info;

    VkResult result = vkCreateInstance(&inst_create_info, vk_context->vk_allocator, &vk_context->vk_instance);
    VK_CHECK(result);

    if (enable_validation_layers && validation_layer_support)
    {
        bool result = vulkan_create_debug_messenger(&dbg_messenger_create_info);
        if (!result)
        {
            DERROR("Couldn't setup Vulkan Create debug messenger.");
            return false;
        }
    }
    if (!vulkan_platform_create_surface(vk_context))
    {
        DERROR("Vulkan surface creation failed.");
        return false;
    }

    if (!vulkan_create_logical_device(vk_context))
    {
        DERROR("Vulkan logical device creation failed.");
        return false;
    }

    if (!vulkan_create_swapchain(vk_context))
    {
        DERROR("Vulkan swapchain creation failed.");
        return false;
    }

    if (!vulkan_create_renderpass(vk_context))
    {
        DERROR("Vulkan renderpass creation failed.");
        return false;
    }

    if (!shader_system_create_default_shaders(&vk_context->default_material_shader_id,
                                              &vk_context->default_skybox_shader_id,
                                              &vk_context->default_grid_shader_id, &vk_context->default_ui_shader_id))
    {
        DERROR("Vulkan default_material shader,skybox shader, grid shader creation failed.");
        return false;
    }
    if (!vulkan_create_framebuffers(vk_context))
    {
        DERROR("Vulkan framebuffer creation failed.");
        return false;
    }
    if (!vulkan_create_command_pools(vk_context))
    {
        DERROR("Vulkan command pool creation failed.");
        return false;
    }
    if (!vulkan_allocate_command_buffers(vk_context, &vk_context->graphics_command_pool,
                                         vk_context->command_buffers.data, MAX_FRAMES_IN_FLIGHT, false))
    {
        DERROR("Vulkan command buffers creation failed.");
        return false;
    }
    if (!vulkan_create_vertex_buffer(vk_context))
    {
        DERROR("Vulkan Vertex buffer creation failed.");
        return false;
    }
    if (!vulkan_create_index_buffer(vk_context))
    {
        DERROR("Vulkan Vertex buffer creation failed.");
        return false;
    }
    if (!vulkan_create_sync_objects(vk_context))
    {
        DERROR("Vulkan sync objects creation failed.");
        return false;
    }

    for (u32 i = 0; i < MAX_GEOMETRIES_LOADED; i++)
    {
        vk_context->world_internal_geometries[i].id            = INVALID_ID;
        vk_context->world_internal_geometries[i].vertex_count  = INVALID_ID;
        vk_context->world_internal_geometries[i].indices_count = INVALID_ID;
        vk_context->ui_internal_geometries[i].id               = INVALID_ID;
        vk_context->ui_internal_geometries[i].vertex_count     = INVALID_ID;
        vk_context->ui_internal_geometries[i].indices_count    = INVALID_ID;
    }

    vk_context->current_frame_index    = 0;
    vk_context->world_geometries_count = 0;
    vk_context->ui_geometries_count    = 0;
    vk_context->frame_counter          = 0;

    return true;
}

bool vulkan_create_command_pools(vulkan_context *vk_context)
{
    VkCommandPoolCreateInfo command_pool_create_info{};

    command_pool_create_info.sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    command_pool_create_info.pNext            = 0;
    command_pool_create_info.flags            = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    command_pool_create_info.queueFamilyIndex = vk_context->vk_device.graphics_family_index;

    VkResult result = vkCreateCommandPool(vk_context->vk_device.logical, &command_pool_create_info,
                                          vk_context->vk_allocator, &vk_context->graphics_command_pool);
    VK_CHECK(result);

    command_pool_create_info.queueFamilyIndex = vk_context->vk_device.transfer_family_index;

    result = vkCreateCommandPool(vk_context->vk_device.logical, &command_pool_create_info, vk_context->vk_allocator,
                                 &vk_context->transfer_command_pool);
    VK_CHECK(result);

    return true;
}

bool vulkan_create_framebuffers(vulkan_context *vk_context)
{
    DINFO("Creating vulkan framebuffers...");
    arena *arena              = vk_context->arena;
    u32    framebuffers_count = vk_context->vk_swapchain.images_count;

    // WORLD renderpass framebuffers
    {
        if (vk_context->vk_swapchain.world_framebuffers == nullptr)
        {
            vk_context->vk_swapchain.world_framebuffers = static_cast<VkFramebuffer *>(
                dallocate(arena, sizeof(VkFramebuffer) * framebuffers_count, MEM_TAG_RENDERER));
        }
        else
        {
            dzero_memory(vk_context->vk_swapchain.world_framebuffers, sizeof(VkFramebuffer) * framebuffers_count);
        }

        VkFramebuffer *buffers = vk_context->vk_swapchain.world_framebuffers;

        for (u32 i = 0; i < framebuffers_count; i++)
        {
            VkImageView framebuffer_attachments[2] = {vk_context->vk_swapchain.img_views[i],
                                                      vk_context->vk_swapchain.depth_image.view};

            VkFramebufferCreateInfo framebuffer_create_info{};

            framebuffer_create_info.sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            framebuffer_create_info.pNext           = 0;
            framebuffer_create_info.flags           = 0;
            framebuffer_create_info.renderPass      = vk_context->world_renderpass.handle;
            framebuffer_create_info.attachmentCount = 2;
            framebuffer_create_info.pAttachments    = framebuffer_attachments;
            framebuffer_create_info.width           = vk_context->vk_swapchain.surface_extent.width;
            framebuffer_create_info.height          = vk_context->vk_swapchain.surface_extent.height;
            framebuffer_create_info.layers          = 1;

            VkResult result = vkCreateFramebuffer(vk_context->vk_device.logical, &framebuffer_create_info,
                                                  vk_context->vk_allocator, &buffers[i]);
            VK_CHECK(result);
        }
    }
    //
    // ui renderpass framebuffers
    {
        if (vk_context->vk_swapchain.ui_framebuffers == nullptr)
        {
            vk_context->vk_swapchain.ui_framebuffers = static_cast<VkFramebuffer *>(
                dallocate(arena, sizeof(VkFramebuffer) * framebuffers_count, MEM_TAG_RENDERER));
        }
        else
        {
            dzero_memory(vk_context->vk_swapchain.ui_framebuffers, sizeof(VkFramebuffer) * framebuffers_count);
        }

        VkFramebuffer *buffers = vk_context->vk_swapchain.ui_framebuffers;

        for (u32 i = 0; i < framebuffers_count; i++)
        {
            VkImageView framebuffer_attachments[1] = {
                vk_context->vk_swapchain.img_views[i],
            };

            VkFramebufferCreateInfo framebuffer_create_info{};

            framebuffer_create_info.sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            framebuffer_create_info.pNext           = 0;
            framebuffer_create_info.flags           = 0;
            framebuffer_create_info.renderPass      = vk_context->ui_renderpass.handle;
            framebuffer_create_info.attachmentCount = 1;
            framebuffer_create_info.pAttachments    = framebuffer_attachments;
            framebuffer_create_info.width           = vk_context->vk_swapchain.surface_extent.width;
            framebuffer_create_info.height          = vk_context->vk_swapchain.surface_extent.height;
            framebuffer_create_info.layers          = 1;

            VkResult result = vkCreateFramebuffer(vk_context->vk_device.logical, &framebuffer_create_info,
                                                  vk_context->vk_allocator, &buffers[i]);
            VK_CHECK(result);
        }
    }

    return true;
}

bool vulkan_allocate_descriptor_set(VkDescriptorPool *desc_pool, VkDescriptorSetLayout *set_layout,
                                    VkDescriptorSet *set)
{
    VkDescriptorSetAllocateInfo per_frame_desc_set_alloc_info{};
    per_frame_desc_set_alloc_info.sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    per_frame_desc_set_alloc_info.pNext              = 0;
    per_frame_desc_set_alloc_info.descriptorPool     = *desc_pool;
    per_frame_desc_set_alloc_info.descriptorSetCount = 1;
    per_frame_desc_set_alloc_info.pSetLayouts        = set_layout;

    VkResult result = vkAllocateDescriptorSets(vk_context->vk_device.logical, &per_frame_desc_set_alloc_info, set);
    VK_CHECK(result);
    return true;
}

// HACK: this is a hack
bool vulkan_update_cubemap_descriptor_set(vulkan_shader *shader, material *material)
{
    VkDescriptorBufferInfo desc_buffer_info{};

    u32 descriptor_set_index = material->internal_id;

    vulkan_allocate_descriptor_set(&shader->per_group_descriptor_pool, &shader->per_group_descriptor_layout,
                                   &shader->per_group_descriptor_sets[descriptor_set_index]);
    shader->total_descriptors_allocated = descriptor_set_index;

    DTRACE("Updataing descriptor_set_index: %d", descriptor_set_index);

    u32                   image_count    = 1;
    VkDescriptorImageInfo image_infos[1] = {};

    vulkan_texture *tex_state  = static_cast<vulkan_texture *>(material->map.diffuse->vulkan_texture_state);
    image_infos[0].sampler     = tex_state->sampler;
    image_infos[0].imageView   = tex_state->image.view;
    image_infos[0].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    VkWriteDescriptorSet desc_writes[1]{};

    desc_writes[0].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    desc_writes[0].pNext            = 0;
    desc_writes[0].dstSet           = shader->per_group_descriptor_sets[descriptor_set_index];
    desc_writes[0].dstBinding       = 0;
    desc_writes[0].dstArrayElement  = 0;
    desc_writes[0].descriptorCount  = 1;
    desc_writes[0].descriptorType   = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    desc_writes[0].pBufferInfo      = 0;
    desc_writes[0].pImageInfo       = &image_infos[0];
    desc_writes[0].pTexelBufferView = 0;

    vkUpdateDescriptorSets(vk_context->vk_device.logical, image_count, desc_writes, 0, nullptr);
    return true;
}

bool vulkan_update_materials_descriptor_set(vulkan_shader *shader, material *material)
{
    VkDescriptorBufferInfo desc_buffer_info{};

    u32 descriptor_set_index = material->internal_id;

    vulkan_allocate_descriptor_set(&shader->per_group_descriptor_pool, &shader->per_group_descriptor_layout,
                                   &shader->per_group_descriptor_sets[descriptor_set_index]);
    shader->total_descriptors_allocated = descriptor_set_index;

    DTRACE("Updataing descriptor_set_index: %d", descriptor_set_index);

    // HACK:
    u32                   image_count    = 3;
    VkDescriptorImageInfo image_infos[3] = {};

    if (shader->renderpass_type == UI_RENDERPASS)
    {
        image_count = 1;
    }

    // INFO: in case
    // I dont get the default material in the start because the default material might not be initialized yet.
    // Because:desc_writes Create default material calls this function .
    struct material *default_mat = nullptr;

    // albedo
    {
        vulkan_texture *tex_state = nullptr;

        if (material->map.diffuse)
        {
            tex_state = static_cast<vulkan_texture *>(material->map.diffuse->vulkan_texture_state);
        }
        if (!tex_state)
        {
            if (default_mat)
            {
                DTRACE("No aldbedo map found for material interal_id: %d", descriptor_set_index);
            }
            else
            {
                default_mat = material_system_get_default_material();
            }
            tex_state = static_cast<vulkan_texture *>(default_mat->map.diffuse->vulkan_texture_state);
        }

        image_infos[0].sampler     = tex_state->sampler;
        image_infos[0].imageView   = tex_state->image.view;
        image_infos[0].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    }
    // normal
    {
        vulkan_texture *tex_state = nullptr;

        if (material->map.normal)
        {
            tex_state = static_cast<vulkan_texture *>(material->map.normal->vulkan_texture_state);
        }
        if (!tex_state)
        {
            if (default_mat)
            {
                DTRACE("No alpha map found for material interal_id: %d", descriptor_set_index);
            }
            else
            {
                default_mat = material_system_get_default_material();
            }
            tex_state = static_cast<vulkan_texture *>(default_mat->map.normal->vulkan_texture_state);
        }

        image_infos[1].sampler     = tex_state->sampler;
        image_infos[1].imageView   = tex_state->image.view;
        image_infos[1].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    }
    // specular
    {
        vulkan_texture *tex_state = nullptr;

        if (material->map.specular)
        {
            tex_state = static_cast<vulkan_texture *>(material->map.specular->vulkan_texture_state);
        }
        if (!tex_state)
        {
            if (default_mat)
            {
                DTRACE("No alpha map found for material interal_id: %d", descriptor_set_index);
            }
            else
            {
                default_mat = material_system_get_default_material();
            }
            tex_state = static_cast<vulkan_texture *>(default_mat->map.specular->vulkan_texture_state);
        }

        image_infos[2].sampler     = tex_state->sampler;
        image_infos[2].imageView   = tex_state->image.view;
        image_infos[2].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    }

    VkWriteDescriptorSet desc_writes[3]{};
    for (u32 i = 0; i < image_count; i++)
    {
        desc_writes[i].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        desc_writes[i].pNext            = 0;
        desc_writes[i].dstSet           = shader->per_group_descriptor_sets[descriptor_set_index];
        desc_writes[i].dstBinding       = i;
        desc_writes[i].dstArrayElement  = 0;
        desc_writes[i].descriptorCount  = 1;
        desc_writes[i].descriptorType   = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        desc_writes[i].pBufferInfo      = 0;
        desc_writes[i].pImageInfo       = &image_infos[i];
        desc_writes[i].pTexelBufferView = 0;
    }

    vkUpdateDescriptorSets(vk_context->vk_device.logical, image_count, desc_writes, 0, nullptr);
    return true;
}

bool vulkan_update_global_descriptor_sets(shader *shader, darray<u32> &ranges)
{
    ZoneScoped;
    DASSERT(shader);

    vulkan_shader *vk_shader = static_cast<vulkan_shader *>(shader->internal_vulkan_shader_state);
    DASSERT(vk_shader);
    u32 size = ranges.size();
    DASSERT_MSG(size < 16, "Size is greater than 16 which was our previous assumtion");

    VkDescriptorBufferInfo desc_buffer_infos[16]{};
    VkWriteDescriptorSet   desc_writes[16]{};
    // HACK:
    VkDeviceSize vk_ranges[16] = {};

    for (u32 i = 0; i < size; i++)
    {
        vk_ranges[i] = ranges[i];
    }

    u32 curr_frame = vk_context->current_frame_index;

    for (u32 i = 0; i < size; i++)
    {
        desc_buffer_infos[i].buffer = vk_shader->per_frame_uniform_buffer.handle;
        desc_buffer_infos[i].offset = 0;
        desc_buffer_infos[i].range  = ranges[i];

        desc_writes[i].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        desc_writes[i].pNext            = 0;
        desc_writes[i].dstSet           = vk_shader->per_frame_descriptor_set;
        desc_writes[i].dstBinding       = i;
        desc_writes[i].dstArrayElement  = 0;
        desc_writes[i].descriptorCount  = 1;
        desc_writes[i].descriptorType   = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
        desc_writes[i].pBufferInfo      = &desc_buffer_infos[i];
        desc_writes[i].pImageInfo       = 0;
        desc_writes[i].pTexelBufferView = 0;
    }
    vkUpdateDescriptorSets(vk_context->vk_device.logical, size, desc_writes, 0, nullptr);

    return true;
}

// TODO: destroy material
bool vulkan_create_cubemap(material *cubemap_mat)
{
    DASSERT(cubemap_mat);
    static u32 id            = 0;
    cubemap_mat->internal_id = id;
    id++;

    shader *skybox_shader = shader_system_get_shader(vk_context->default_skybox_shader_id);

    vulkan_shader *shader = static_cast<vulkan_shader *>(skybox_shader->internal_vulkan_shader_state);
    bool           result = vulkan_update_cubemap_descriptor_set(shader, cubemap_mat);
    DASSERT(result == true);

    return true;
}

// TODO: destroy material
bool vulkan_create_material(material *in_mat, u64 shader_id)
{
    static u32 id       = 0;
    in_mat->internal_id = id;
    id++;
    shader *shader = shader_system_get_shader(shader_id);
    DASSERT(shader);

    vulkan_shader *vk_shader = static_cast<vulkan_shader *>(shader->internal_vulkan_shader_state);
    bool           result    = vulkan_update_materials_descriptor_set(vk_shader, in_mat);

    DASSERT(result == true);

    return true;
}

u64 align_upto(u64 size, u64 alignment)
{
    u64 aligned_size = ((size + alignment - 1) & ~(alignment - 1));
    return aligned_size;
}

bool vulkan_initialize_shader(shader_config *config, shader *in_shader)
{
    if (!vk_context)
    {
        DASSERT_MSG(vk_context, "Renderer system has not been initialized. Initalize the renderer system before "
                                "calling vulkan_create_shader.");
    }
    if (!config || !in_shader)
    {
        DERROR("Provided parameters to vulkan_create_shader were nullptr.");
        return false;
    }
    arena         *arena     = vk_context->arena;
    vulkan_shader *vk_shader = static_cast<vulkan_shader *>(dallocate(arena, sizeof(vulkan_shader), MEM_TAG_RENDERER));

    u32 uniforms_size                 = config->uniforms.size();
    vk_shader->pipeline_configuration = config->pipeline_configuration;
    vk_shader->renderpass_type        = config->renderpass_types;
    //
    // per-frame resources aka (uniform_s, aka set 0)
    {
        if (config->has_per_frame)
        {
            // create the per_frame_descriptor layout
            u32                                   per_frame_descriptor_binding_count = 0;
            darray<VkDescriptorSetLayoutBinding> &per_frame_descriptor_set_layout_bindings =
                vk_shader->per_frame_descriptor_sets_layout_bindings;
            per_frame_descriptor_set_layout_bindings.reserve(arena);

            darray<VkShaderStageFlags> per_frame_stage_flags{};
            per_frame_stage_flags.c_init(arena);
            darray<VkDescriptorType> des_types{};
            des_types.c_init(arena);

            u32 uniforms_count = config->uniforms.size();

            for (u32 i = 0; i < uniforms_count; i++)
            {
                if (config->uniforms[i].scope == SHADER_PER_FRAME_UNIFORM)
                {
                    per_frame_descriptor_binding_count++;
                    per_frame_stage_flags.push_back(config->uniforms[i].stage);
                    des_types.push_back(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC);
                }
            }

            for (u32 i = 0; i < per_frame_descriptor_binding_count; i++)
            {
                per_frame_descriptor_set_layout_bindings[i].binding            = i;
                per_frame_descriptor_set_layout_bindings[i].descriptorType     = des_types[i];
                per_frame_descriptor_set_layout_bindings[i].descriptorCount    = 1;
                per_frame_descriptor_set_layout_bindings[i].stageFlags         = per_frame_stage_flags[i];
                per_frame_descriptor_set_layout_bindings[i].pImmutableSamplers = 0;
            }

            VkDescriptorSetLayoutCreateInfo per_frame_descriptor_set_layout_create_info{};
            per_frame_descriptor_set_layout_create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
            per_frame_descriptor_set_layout_create_info.pNext = 0;
            per_frame_descriptor_set_layout_create_info.flags = 0;
            per_frame_descriptor_set_layout_create_info.bindingCount = per_frame_descriptor_binding_count;
            per_frame_descriptor_set_layout_create_info.pBindings    = per_frame_descriptor_set_layout_bindings.data;

            VkResult result =
                vkCreateDescriptorSetLayout(vk_context->vk_device.logical, &per_frame_descriptor_set_layout_create_info,
                                            vk_context->vk_allocator, &vk_shader->per_frame_descriptor_layout);
            VK_CHECK(result);

            // create the per_frame_descriptor pool
            VkDescriptorPoolSize per_frame_pool_sizes[1]{};

            per_frame_pool_sizes[0].type            = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
            per_frame_pool_sizes[0].descriptorCount = MAX_FRAMES_IN_FLIGHT;

            VkDescriptorPoolCreateInfo per_frame_descriptor_pool_create_info{};
            per_frame_descriptor_pool_create_info.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
            per_frame_descriptor_pool_create_info.pNext         = 0;
            per_frame_descriptor_pool_create_info.flags         = 0;
            per_frame_descriptor_pool_create_info.maxSets       = MAX_FRAMES_IN_FLIGHT;
            per_frame_descriptor_pool_create_info.poolSizeCount = 1;
            per_frame_descriptor_pool_create_info.pPoolSizes    = per_frame_pool_sizes;

            result = vkCreateDescriptorPool(vk_context->vk_device.logical, &per_frame_descriptor_pool_create_info,
                                            vk_context->vk_allocator, &vk_shader->per_frame_descriptor_pool);
            VK_CHECK(result);

            darray<VkDescriptorSetLayout> per_frame_desc_set_layout;
            per_frame_desc_set_layout.reserve(arena, MAX_FRAMES_IN_FLIGHT);
            for (u32 i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
            {
                per_frame_desc_set_layout[i] = vk_shader->per_frame_descriptor_layout;
            }

            VkDescriptorSetAllocateInfo per_frame_desc_set_alloc_info{};
            per_frame_desc_set_alloc_info.sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
            per_frame_desc_set_alloc_info.pNext              = 0;
            per_frame_desc_set_alloc_info.descriptorPool     = vk_shader->per_frame_descriptor_pool;
            per_frame_desc_set_alloc_info.descriptorSetCount = 1;
            per_frame_desc_set_alloc_info.pSetLayouts =
                reinterpret_cast<const VkDescriptorSetLayout *>(per_frame_desc_set_layout.data);

            result = vkAllocateDescriptorSets(vk_context->vk_device.logical, &per_frame_desc_set_alloc_info,
                                              &vk_shader->per_frame_descriptor_set);
            VK_CHECK(result);

            // get the uniform_size
            // NOTE: this is the unalgined sizes of the bindings for set 0 aka global aka per_frame
            vk_shader->per_frame_uniforms_size = config->per_frame_uniform_offsets;

            u32 &per_frame_ubo_size = vk_shader->per_frame_stride;
            per_frame_ubo_size      = 0;

            u32 required_alignment = vk_context->vk_device.physical_properties->limits.minUniformBufferOffsetAlignment;
            vk_shader->min_ubo_alignment = required_alignment;

            u32 size = config->per_frame_uniform_offsets.size();
            for (u32 i = 0; i < size; i++)
            {
                per_frame_ubo_size += config->per_frame_uniform_offsets[i];
                per_frame_ubo_size  = (per_frame_ubo_size + required_alignment - 1) & ~(required_alignment - 1);
                vk_shader->per_frame_uniforms_size[i] = per_frame_ubo_size;
            }

            per_frame_ubo_size = (per_frame_ubo_size + required_alignment - 1) & ~(required_alignment - 1);

            u64 total_per_frame_buffer_size = per_frame_ubo_size * MAX_FRAMES_IN_FLIGHT;

            {
                VkBufferUsageFlags    usg_flags = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
                VkMemoryPropertyFlags prop_flags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT |
                                                   VK_MEMORY_PROPERTY_HOST_COHERENT_BIT |
                                                   VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
                bool res = vulkan_create_buffer(vk_context, &vk_shader->per_frame_uniform_buffer, usg_flags, prop_flags,
                                                total_per_frame_buffer_size);
                DASSERT_MSG(res, "Vulkan Couldnt create per_frame_uniform_buffer.");

                // map the memory
                VK_CHECK(vkMapMemory(vk_context->vk_device.logical, vk_shader->per_frame_uniform_buffer.memory, 0,
                                     VK_WHOLE_SIZE, 0, &vk_shader->per_frame_mapped_data));
            }
        }
        else
        {
            DDEBUG("The shader doesnt have a per_frame_uniform buffer. Are you sure??");
        }
    }
    // per_group resources aka set 1
    {
        // HACK: this feels like a hack tbh
        if (config->has_per_group)
        {

            u32 per_group_descriptors_binding_count = 0;
            u32 uniforms_count                      = config->uniforms.size();

            darray<VkShaderStageFlags> stage_flags{};
            stage_flags.c_init(arena);
            darray<VkDescriptorType> des_types{};
            des_types.c_init(arena);

            for (u32 i = 0; i < uniforms_count; i++)
            {
                shader_scope scope = config->uniforms[i].scope;
                if (scope == SHADER_PER_GROUP_UNIFORM)
                {
                    per_group_descriptors_binding_count++;
                    stage_flags.push_back(config->uniforms[i].stage);

                    // we only check for the first one for now
                    attribute_types type = config->uniforms[i].types[0];

                    if (type == SAMPLER_2D)
                    {
                        des_types.push_back(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
                    }
                    else if (type == SAMPLER_CUBE)
                    {
                        des_types.push_back(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
                    }
                    else
                    {
                        DERROR("Unknown descriptor Types %d", type);
                    };
                }
            }

            darray<VkDescriptorSetLayoutBinding> &per_group_descriptor_set_layout_bindings =
                vk_shader->per_group_descriptor_sets_layout_bindings;
            per_group_descriptor_set_layout_bindings.reserve(arena);

            for (u32 i = 0; i < per_group_descriptors_binding_count; i++)
            {
                per_group_descriptor_set_layout_bindings[i].binding            = i;
                per_group_descriptor_set_layout_bindings[i].descriptorCount    = 1;
                per_group_descriptor_set_layout_bindings[i].descriptorType     = des_types[i];
                per_group_descriptor_set_layout_bindings[i].stageFlags         = stage_flags[i];
                per_group_descriptor_set_layout_bindings[i].pImmutableSamplers = nullptr;
            }

            VkDescriptorSetLayoutCreateInfo per_group_descriptor_set_layout_create_info{};
            per_group_descriptor_set_layout_create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
            per_group_descriptor_set_layout_create_info.pNext = 0;
            per_group_descriptor_set_layout_create_info.flags = 0;
            per_group_descriptor_set_layout_create_info.bindingCount = per_group_descriptors_binding_count;
            per_group_descriptor_set_layout_create_info.pBindings    = per_group_descriptor_set_layout_bindings.data;

            VkResult result =
                vkCreateDescriptorSetLayout(vk_context->vk_device.logical, &per_group_descriptor_set_layout_create_info,
                                            vk_context->vk_allocator, &vk_shader->per_group_descriptor_layout);
            VK_CHECK(result);

            // NOTE: if it has more than one binding then you need to multiply the max_num_sets * num_bindings
            u32 max_image_sampler_limit =
                vk_context->vk_device.physical_properties->limits.maxDescriptorSetSampledImages;
            u32 max_descriptors = DMIN(max_image_sampler_limit, VULKAN_MAX_DESCRIPTOR_SET_COUNT);

            VkDescriptorPoolSize material_pool_sizes[1]{};

            material_pool_sizes[0].type            = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            material_pool_sizes[0].descriptorCount = max_descriptors * 3;

            VkDescriptorPoolCreateInfo material_descriptor_pool_create_info{};
            material_descriptor_pool_create_info.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
            material_descriptor_pool_create_info.pNext         = 0;
            material_descriptor_pool_create_info.flags         = 0;
            material_descriptor_pool_create_info.maxSets       = max_descriptors * 3;
            material_descriptor_pool_create_info.poolSizeCount = 1;
            material_descriptor_pool_create_info.pPoolSizes    = material_pool_sizes;

            result = vkCreateDescriptorPool(vk_context->vk_device.logical, &material_descriptor_pool_create_info,
                                            vk_context->vk_allocator, &vk_shader->per_group_descriptor_pool);
            VK_CHECK(result);

            vk_shader->per_group_descriptor_sets.reserve(arena, max_descriptors);

            u32 &per_group_ubo_size = vk_shader->per_group_ubo_size;
            per_group_ubo_size      = 0;

            u32 size = config->per_group_uniform_offsets.size();
            for (u32 i = 0; i < size; i++)
            {
                per_group_ubo_size += config->per_group_uniform_offsets[i];
            }

            u32 required_alignment = vk_context->vk_device.physical_properties->limits.minUniformBufferOffsetAlignment;
            per_group_ubo_size     = (per_group_ubo_size + required_alignment - 1) & ~(required_alignment - 1);

            u64 total_per_group_buffer_size = per_group_ubo_size * max_descriptors;

            VkBufferUsageFlags    usg_flags  = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
            VkMemoryPropertyFlags prop_flags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT |
                                               VK_MEMORY_PROPERTY_HOST_COHERENT_BIT |
                                               VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
            bool res = vulkan_create_buffer(vk_context, &vk_shader->per_group_uniform_buffer, usg_flags, prop_flags,
                                            total_per_group_buffer_size);
            DASSERT_MSG(res, "Vulkan Couldnt create per_group_uniform_buffer.");
            VK_CHECK(result);
        }
        else
        {
            DDEBUG("Shader doesnt have per_group aka ( set 1 ). Are you sure??");
        }
    }

    // atttributes
    {

        u32 vertex_input_attribute_descriptions_count = config->attributes.size();
        darray<VkVertexInputAttributeDescription> &vertex_input_attribute_descriptions =
            vk_shader->input_attribute_descriptions;
        vertex_input_attribute_descriptions.reserve(arena, vertex_input_attribute_descriptions_count);

        u32 offset = 0;

        for (u32 i = 0; i < vertex_input_attribute_descriptions_count; i++)
        {
            DASSERT(i == config->attributes[i].location);
            vertex_input_attribute_descriptions[i].location  = config->attributes[i].location;
            vertex_input_attribute_descriptions[i].binding   = 0;
            vertex_input_attribute_descriptions[i].offset    = offset;
            offset                                          += config->attributes[i].type;

            if (config->attributes[i].type == VEC_3)
            {
                vertex_input_attribute_descriptions[i].format = VK_FORMAT_R32G32B32_SFLOAT;
            }
            else if (config->attributes[i].type == VEC_2)
            {
                vertex_input_attribute_descriptions[i].format = VK_FORMAT_R32G32_SFLOAT;
            }
            else if (config->attributes[i].type == VEC_4)
            {
                vertex_input_attribute_descriptions[i].format = VK_FORMAT_R32G32B32A32_SFLOAT;
            }
            else
            {
                DERROR("No support currently for %d type for shader_attributes.", config->attributes[i].type);
                return false;
            }
        }

        VkVertexInputBindingDescription &vertex_input_binding_description = vk_shader->attribute_description;
        vertex_input_binding_description.binding                          = 0;
        vertex_input_binding_description.stride                           = offset;
        vertex_input_binding_description.inputRate                        = VK_VERTEX_INPUT_RATE_VERTEX;
    }

    u64 vert_shader_code__size_requirements = INVALID_ID_64;
    file_open_and_read(config->vert_spv_full_path.c_str(), &vert_shader_code__size_requirements, 0, 1);
    DASSERT(vert_shader_code__size_requirements != INVALID_ID_64);

    vk_shader->vertex_shader_code.reserve(arena, vert_shader_code__size_requirements);
    file_open_and_read(config->vert_spv_full_path.c_str(), &vert_shader_code__size_requirements,
                       vk_shader->vertex_shader_code.data, 1);

    u64 frag_shader_code__size_requirements = INVALID_ID_64;
    file_open_and_read(config->frag_spv_full_path.c_str(), &frag_shader_code__size_requirements, 0, 1);
    DASSERT(frag_shader_code__size_requirements != INVALID_ID_64);

    vk_shader->fragment_shader_code.reserve(arena, frag_shader_code__size_requirements);
    file_open_and_read(config->frag_spv_full_path.c_str(), &frag_shader_code__size_requirements,
                       vk_shader->fragment_shader_code.data, 1);

    vk_shader->stages = config->stages;
    bool res          = vulkan_create_pipeline(vk_context, vk_shader);

    in_shader->internal_vulkan_shader_state = vk_shader;
    return true;
}

bool vulkan_destroy_shader(shader *shader)
{
    DASSERT(shader);
    vulkan_shader *vk_shader = static_cast<vulkan_shader *>(shader->internal_vulkan_shader_state);
    DASSERT(vk_shader);

    VkDevice              &device    = vk_context->vk_device.logical;
    VkAllocationCallbacks *allocator = vk_context->vk_allocator;

    vkDestroyDescriptorPool(device, vk_shader->per_frame_descriptor_pool, allocator);
    vkDestroyDescriptorSetLayout(device, vk_shader->per_frame_descriptor_layout, allocator);
    vk_shader->per_frame_mapped_data = nullptr;

    vkDestroyDescriptorPool(device, vk_shader->per_group_descriptor_pool, allocator);
    vkDestroyDescriptorSetLayout(device, vk_shader->per_group_descriptor_layout, allocator);
    vk_shader->per_group_descriptor_sets.~darray();

    vulkan_destroy_buffer(vk_context, &vk_shader->per_frame_uniform_buffer);
    vulkan_destroy_buffer(vk_context, &vk_shader->per_group_uniform_buffer);

    vkDestroyPipeline(device, vk_shader->pipeline.handle, allocator);

    vkDestroyPipelineLayout(device, vk_shader->pipeline.layout, allocator);

    return true;
}
bool vulkan_create_texture(texture *in_texture, u8 *pixels)
{
    DASSERT(pixels);
    DASSERT(in_texture);
    arena *arena = vk_context->arena;

    in_texture->vulkan_texture_state =
        static_cast<vulkan_texture *>(dallocate(arena, sizeof(vulkan_texture), MEM_TAG_RENDERER));
    vulkan_texture *vk_texture = static_cast<vulkan_texture *>(in_texture->vulkan_texture_state);

    u32 tex_width             = in_texture->width;
    u32 tex_height            = in_texture->height;
    u32 tex_channel           = 4;
    u64 texture_size          = INVALID_ID_64;
    // HACK: this should be more explicit. For now we know that we set the value of texture size only when we uplaod a
    // cubemap to the gpu.
    bool          is_cube_map = false;
    vulkan_buffer staging_buffer{};
    vulkan_image *image = &vk_texture->image;
    image->width        = tex_width;
    image->height       = tex_height;

    if (in_texture->texure_size != INVALID_ID_64)
    {
        texture_size            = in_texture->texure_size;
        image->view_type        = VK_IMAGE_VIEW_TYPE_CUBE;
        image->img_create_flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
        is_cube_map             = true;
        image->mip_levels       = 1;
    }
    else
    {
        image->view_type        = VK_IMAGE_VIEW_TYPE_2D;
        image->img_create_flags = 0;
        texture_size            = tex_width * tex_height * 4;
        u32 max                 = DMAX(tex_width, tex_height);
        u32 mip_level           = floor(log2(max)) + 1;
        image->mip_levels       = mip_level;
    }

    // INFO: use the correct colorspace for the image, if the texture is a default texture then it should not be mapping
    // to sRGB colorspace. There was a bug before when the final color of a default normal map was (0.25, 0.25,1,1) even
    // though the actual value was (0.5,0.5,1,1) because I used the sRGB format
    if (in_texture->format == IMG_FORMAT_UNORM)
    {
        image->format = VK_FORMAT_R8G8B8A8_UNORM;
    }
    else if (in_texture->format == IMG_FORMAT_SRGB)
    {
        image->format = VK_FORMAT_R8G8B8A8_SRGB;
    }
    else
    {
        DERROR("Uknown image format. %d", in_texture->format);
        return false;
    }

    bool result =
        vulkan_create_buffer(vk_context, &staging_buffer, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                             VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, texture_size);
    DASSERT(result == true);

    void *data = nullptr;
    vulkan_copy_data_to_buffer(vk_context, &staging_buffer, data, pixels, texture_size);

    VkImageUsageFlags img_usage =
        VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;

    result = vulkan_create_image(vk_context, image, tex_width, tex_height, image->format,
                                 VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, img_usage, VK_IMAGE_TILING_OPTIMAL);

    DASSERT(result == true);

    vulkan_transition_image_layout(vk_context, &vk_context->transfer_command_pool,
                                   &vk_context->vk_device.transfer_queue, image, VK_IMAGE_LAYOUT_UNDEFINED,
                                   VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

    vulkan_copy_buffer_data_to_image(vk_context, &vk_context->transfer_command_pool,
                                     &vk_context->vk_device.transfer_queue, &staging_buffer, image);

    if (!is_cube_map)
    {
        vulkan_generate_mipmaps(vk_context, image);
    }
    else
    {
        vulkan_transition_image_layout(vk_context, &vk_context->transfer_command_pool,
                                       &vk_context->vk_device.transfer_queue, image,
                                       VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
        vulkan_transition_image_layout(vk_context, &vk_context->graphics_command_pool,
                                       &vk_context->vk_device.graphics_queue, image,
                                       VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    }

    vulkan_destroy_buffer(vk_context, &staging_buffer);

    result = vulkan_create_image_view(vk_context, &image->handle, &image->view, image->view_type, image->format,
                                      VK_IMAGE_ASPECT_COLOR_BIT, image->mip_levels);
    DASSERT(result == true);

    f32 max_sampler_anisotropy = vk_context->vk_device.physical_properties->limits.maxSamplerAnisotropy;

    VkSamplerCreateInfo texture_sampler_create_info{};
    texture_sampler_create_info.sType                   = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    texture_sampler_create_info.pNext                   = 0;
    texture_sampler_create_info.flags                   = 0;
    texture_sampler_create_info.magFilter               = VK_FILTER_LINEAR;
    texture_sampler_create_info.minFilter               = VK_FILTER_LINEAR;
    texture_sampler_create_info.mipmapMode              = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    texture_sampler_create_info.addressModeU            = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    texture_sampler_create_info.addressModeV            = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    texture_sampler_create_info.addressModeW            = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    texture_sampler_create_info.mipLodBias              = 0.0f;
    texture_sampler_create_info.anisotropyEnable        = VK_TRUE;
    texture_sampler_create_info.maxAnisotropy           = max_sampler_anisotropy;
    texture_sampler_create_info.compareEnable           = VK_FALSE;
    texture_sampler_create_info.compareOp               = VK_COMPARE_OP_ALWAYS;
    texture_sampler_create_info.minLod                  = 0.0f;
    texture_sampler_create_info.maxLod                  = image->mip_levels;
    texture_sampler_create_info.borderColor             = VK_BORDER_COLOR_INT_OPAQUE_WHITE;
    texture_sampler_create_info.unnormalizedCoordinates = VK_FALSE;

    VkResult res = vkCreateSampler(vk_context->vk_device.logical, &texture_sampler_create_info,
                                   vk_context->vk_allocator, &vk_texture->sampler);

    VK_CHECK(res);

    return true;
}

bool vulkan_destroy_shader(struct shader *in_shader);

bool vulkan_destroy_texture(texture *in_texture)
{
    vulkan_texture *vk_texture = static_cast<vulkan_texture *>(in_texture->vulkan_texture_state);
    if (!vk_texture)
    {
        DWARN("Vk texture is nullptr, invalid texture.");
        return false;
    }
    vulkan_image *image = &vk_texture->image;

    vulkan_destroy_image(vk_context, image);
    vkDestroySampler(vk_context->vk_device.logical, vk_texture->sampler, vk_context->vk_allocator);
    dfree(vk_texture, sizeof(vulkan_texture), MEM_TAG_RENDERER);
    in_texture->vulkan_texture_state = nullptr;
    return true;
}

bool vulkan_create_debug_messenger(VkDebugUtilsMessengerCreateInfoEXT *dbg_messenger_create_info)
{

    dbg_messenger_create_info->sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    dbg_messenger_create_info->pNext = 0;
    dbg_messenger_create_info->flags = 0;
    dbg_messenger_create_info->messageSeverity =
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;

    dbg_messenger_create_info->messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                                             VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                                             VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    dbg_messenger_create_info->pfnUserCallback = vulkan_dbg_msg_rprt_callback;
    dbg_messenger_create_info->pUserData       = nullptr;

    if (!vk_context->vk_instance)
    {
        DDEBUG("Populating vulkan_debug_utils_messenger struct");
        return true;
    }

    PFN_vkCreateDebugUtilsMessengerEXT func_create_dbg_utils_messenger_ext =
        reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(
            vkGetInstanceProcAddr(vk_context->vk_instance, "vkCreateDebugUtilsMessengerEXT"));

    if (func_create_dbg_utils_messenger_ext == nullptr)
    {
        DERROR("Vulkan Create debug messenger ext called but fucn ptr to vkCreateDebugUtilsMessengerEXT is nullptr");
        return false;
    }
    if (vk_context->vk_instance)
    {
        VK_CHECK(func_create_dbg_utils_messenger_ext(vk_context->vk_instance, dbg_messenger_create_info,
                                                     vk_context->vk_allocator, &vk_context->vk_dbg_messenger));
    }

    return true;
}

void vulkan_backend_shutdown()

{
    DDEBUG("Shutting down vulkan...");

    VkDevice              &device    = vk_context->vk_device.logical;
    VkAllocationCallbacks *allocator = vk_context->vk_allocator;

    vkDeviceWaitIdle(device);
    for (u32 i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        vkDestroyFence(device, vk_context->in_flight_fences[i], allocator);
        vkDestroySemaphore(device, vk_context->render_finished_semaphores[i], allocator);
        vkDestroySemaphore(device, vk_context->image_available_semaphores[i], allocator);
    }
    dfree(vk_context->image_available_semaphores, sizeof(VkSemaphore) * MAX_FRAMES_IN_FLIGHT, MEM_TAG_RENDERER);
    dfree(vk_context->render_finished_semaphores, sizeof(VkSemaphore) * MAX_FRAMES_IN_FLIGHT, MEM_TAG_RENDERER);
    dfree(vk_context->in_flight_fences, sizeof(VkFence) * MAX_FRAMES_IN_FLIGHT, MEM_TAG_RENDERER);

    vulkan_destroy_buffer(vk_context, &vk_context->world_renderpass.vertex_buffer);
    vulkan_destroy_buffer(vk_context, &vk_context->world_renderpass.index_buffer);

    vulkan_destroy_buffer(vk_context, &vk_context->ui_renderpass.vertex_buffer);
    vulkan_destroy_buffer(vk_context, &vk_context->ui_renderpass.index_buffer);

    vulkan_free_command_buffers(vk_context, &vk_context->graphics_command_pool,
                                static_cast<VkCommandBuffer *>(vk_context->command_buffers.data), MAX_FRAMES_IN_FLIGHT);
    vk_context->command_buffers.~darray();
    vkDestroyCommandPool(device, vk_context->graphics_command_pool, allocator);
    vkDestroyCommandPool(device, vk_context->transfer_command_pool, allocator);

    for (u32 i = 0; i < vk_context->vk_swapchain.images_count; i++)
    {
        vkDestroyFramebuffer(device, vk_context->vk_swapchain.world_framebuffers[i], allocator);
        vkDestroyFramebuffer(device, vk_context->vk_swapchain.ui_framebuffers[i], allocator);
    }
    dfree(vk_context->vk_swapchain.world_framebuffers, sizeof(VkFramebuffer) * vk_context->vk_swapchain.images_count,
          MEM_TAG_RENDERER);
    dfree(vk_context->vk_swapchain.ui_framebuffers, sizeof(VkFramebuffer) * vk_context->vk_swapchain.images_count,
          MEM_TAG_RENDERER);

    shader *material_shader = shader_system_get_shader(vk_context->default_material_shader_id);
    DASSERT(material_shader);

    shader *skybox_shader = shader_system_get_shader(vk_context->default_skybox_shader_id);
    DASSERT(skybox_shader);

    shader *grid_shader = shader_system_get_shader(vk_context->default_grid_shader_id);
    DASSERT(grid_shader);

    shader *ui_shader = shader_system_get_shader(vk_context->default_ui_shader_id);
    DASSERT(ui_shader);

    vulkan_destroy_shader(material_shader);
    vulkan_destroy_shader(skybox_shader);
    vulkan_destroy_shader(grid_shader);
    vulkan_destroy_shader(ui_shader);

    vkDestroyRenderPass(device, vk_context->world_renderpass.handle, allocator);
    vkDestroyRenderPass(device, vk_context->ui_renderpass.handle, allocator);

    for (u32 i = 0; i < vk_context->vk_swapchain.images_count; i++)
    {
        vkDestroyImageView(device, vk_context->vk_swapchain.img_views[i], allocator);
    }
    vulkan_destroy_image(vk_context, &vk_context->vk_swapchain.depth_image);

    dfree(vk_context->vk_swapchain.images, sizeof(VkImage) * vk_context->vk_swapchain.images_count, MEM_TAG_RENDERER);
    dfree(vk_context->vk_swapchain.img_views, sizeof(VkImageView) * vk_context->vk_swapchain.images_count,
          MEM_TAG_RENDERER);

    dfree(vk_context->vk_swapchain.surface_formats, sizeof(VkSurfaceFormatKHR) * vk_context->vk_swapchain.images_count,
          MEM_TAG_RENDERER);
    dfree(vk_context->vk_swapchain.present_modes, sizeof(VkPresentModeKHR) * vk_context->vk_swapchain.images_count,
          MEM_TAG_RENDERER);

    vkDestroySwapchainKHR(device, vk_context->vk_swapchain.handle, allocator);

    vkDestroySurfaceKHR(vk_context->vk_instance, vk_context->vk_surface, allocator);

    // destroy logical device
    if (vk_context->vk_device.physical_properties)
    {
        dfree(vk_context->vk_device.physical_properties, sizeof(VkPhysicalDeviceProperties), MEM_TAG_RENDERER);
    }
    if (vk_context->vk_device.physical_features)
    {
        dfree(vk_context->vk_device.physical_features, sizeof(VkPhysicalDeviceFeatures), MEM_TAG_RENDERER);
    }
    vkDestroyDevice(device, allocator);

#if DEBUG
    PFN_vkDestroyDebugUtilsMessengerEXT func_destroy_dbg_utils_messenger_ext =
        reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(
            vkGetInstanceProcAddr(vk_context->vk_instance, "vkDestroyDebugUtilsMessengerEXT"));

    if (func_destroy_dbg_utils_messenger_ext == nullptr)
    {
        DERROR("Vulkan destroy debug messenger ext called but fucn ptr to vkDestroyDebugUtilsMessengerEXT is nullptr");
    }
    else
    {
        func_destroy_dbg_utils_messenger_ext(vk_context->vk_instance, vk_context->vk_dbg_messenger, allocator);
    }
#endif
    vkDestroyInstance(vk_context->vk_instance, allocator);
}

bool vulkan_check_validation_layer_support()
{
    u32 inst_layer_properties_count = 0;
    vkEnumerateInstanceLayerProperties(&inst_layer_properties_count, 0);
    arena *arena = vk_context->arena;

    darray<VkLayerProperties> inst_layer_properties;
    inst_layer_properties.reserve(arena, inst_layer_properties_count);
    vkEnumerateInstanceLayerProperties(&inst_layer_properties_count,
                                       static_cast<VkLayerProperties *>(inst_layer_properties.data));

    const char *validation_layer_name = "VK_LAYER_KHRONOS_validation";

    bool validation_layer_found = false;

#ifdef DEBUG
    for (u32 i = 0; i < inst_layer_properties_count; i++)
    {
        DDEBUG("Layer Name: %s | Desc: %s", inst_layer_properties[i].layerName, inst_layer_properties[i].description);
    }
#endif

    for (u32 i = 0; i < inst_layer_properties_count; i++)
    {
        if (string_compare(inst_layer_properties[i].layerName, validation_layer_name))
        {
            validation_layer_found = true;
            break;
        }
    }
    if (!validation_layer_found)
    {
        DERROR("Validation layers requested but not found!!!");
        return false;
    }
    return validation_layer_found;
}

VkBool32 vulkan_dbg_msg_rprt_callback(VkDebugUtilsMessageSeverityFlagBitsEXT      messageSeverity,
                                      VkDebugUtilsMessageTypeFlagsEXT             messageTypes,
                                      const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData, void *pUserData)
{
    switch (messageSeverity)
    {
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT: {
        DTRACE("%s", pCallbackData->pMessage);
    }
    break;
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT: {
        DINFO("%s", pCallbackData->pMessage);
    }
    break;
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT: {
        DWARN("%s", pCallbackData->pMessage);
    }
    break;
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT: {
        DERROR("%s", pCallbackData->pMessage);
    }
    break;
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_FLAG_BITS_MAX_ENUM_EXT: {
    }
    break;
    }

    return VK_FALSE;
}

bool vulkan_update_global_uniform_buffer(shader *shader, u32 offset, u32 size, void *data)
{
    DASSERT(shader);
    vulkan_shader *vk_shader = static_cast<vulkan_shader *>(shader->internal_vulkan_shader_state);

    // HACK: make it better
    offset = align_upto(offset, vk_shader->min_ubo_alignment);

    u8 *addr = static_cast<u8 *>(vk_shader->per_frame_mapped_data) +
               (vk_shader->per_frame_stride * vk_context->current_frame_index + offset);
    dcopy_memory(addr, data, size);
    return true;
}

u32 vulkan_calculate_vertex_offset(vulkan_context *vk_context, u32 geometry_id,
                                   vulkan_geometry_data *vk_geo_internal_array)
{
    // INFO: maybe make this a u64 ?
    u32 vert_offset = 0;

    for (u32 i = 0; i < MAX_GEOMETRIES_LOADED && i < geometry_id; i++)
    {
        if (vk_geo_internal_array[i].id == INVALID_ID)
        {
            break;
        }
        vert_offset += vk_geo_internal_array[i].vertex_count;
    }
    u32 ans = vert_offset;
    return ans;
}

u32 vulkan_calculate_index_offset(vulkan_context *vk_context, u32 geometry_id,
                                  vulkan_geometry_data *vk_geo_internal_array)
{
    u32 index_offset = 0;

    for (u32 i = 0; i < MAX_GEOMETRIES_LOADED && i < geometry_id; i++)
    {
        if (vk_geo_internal_array[i].id == INVALID_ID)
        {
            break;
        }
        index_offset += vk_geo_internal_array[i].indices_count;
    }
    u32 ans = index_offset;
    return ans;
}

bool vulkan_create_geometry(renderpass_types type, geometry *out_geometry, u32 vertex_count, u32 vertex_size,
                            void *vertices, u32 index_count, u32 *indices)
{

    void *vertex_data        = nullptr;
    u32   vertex_buffer_size = vertex_count * vertex_size;
    void *index_data         = nullptr;
    u32   index_buffer_size  = index_count * sizeof(u32);

    vulkan_buffer vertex_staging_buffer{};
    vulkan_create_buffer(vk_context, &vertex_staging_buffer, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                         VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                         vertex_buffer_size);

    vulkan_copy_data_to_buffer(vk_context, &vertex_staging_buffer, vertex_data, vertices, vertex_buffer_size);

    vulkan_buffer index_staging_buffer{};
    if (index_buffer_size)
    {
        vulkan_create_buffer(vk_context, &index_staging_buffer, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                             VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                             index_buffer_size);
        vulkan_copy_data_to_buffer(vk_context, &index_staging_buffer, index_data, indices, index_buffer_size);
    }

    vulkan_geometry_data *internal_array = nullptr;
    u32                   vertex_offset  = INVALID_ID;
    u32                   index_offset   = INVALID_ID;

    if (type == WORLD_RENDERPASS)
    {
        vertex_offset =
            vulkan_calculate_vertex_offset(vk_context, vertex_offset, vk_context->world_internal_geometries) *
            vertex_size;
        index_offset = vulkan_calculate_index_offset(vk_context, index_offset, vk_context->world_internal_geometries) *
                       sizeof(u32);
        vk_context->world_geometries_count++;
        internal_array = vk_context->world_internal_geometries;
        vulkan_copy_buffer(vk_context, &vk_context->transfer_command_pool, &vk_context->vk_device.transfer_queue,
                           &vk_context->world_renderpass.vertex_buffer, vertex_offset, &vertex_staging_buffer,
                           vertex_buffer_size);
        vulkan_copy_buffer(vk_context, &vk_context->transfer_command_pool, &vk_context->vk_device.transfer_queue,
                           &vk_context->world_renderpass.index_buffer, index_offset, &index_staging_buffer,
                           index_buffer_size);
    }
    else if (type == UI_RENDERPASS)
    {
        vertex_offset = 0;
        index_offset  = 0;

        vk_context->ui_geometries_count++;
        internal_array = vk_context->ui_internal_geometries;
        vulkan_copy_buffer(vk_context, &vk_context->transfer_command_pool, &vk_context->vk_device.transfer_queue,
                           &vk_context->ui_renderpass.vertex_buffer, vertex_offset, &vertex_staging_buffer,
                           vertex_buffer_size);
        vulkan_copy_buffer(vk_context, &vk_context->transfer_command_pool, &vk_context->vk_device.transfer_queue,
                           &vk_context->ui_renderpass.index_buffer, index_offset, &index_staging_buffer,
                           index_buffer_size);
    }

    vulkan_destroy_buffer(vk_context, &vertex_staging_buffer);
    vulkan_destroy_buffer(vk_context, &index_staging_buffer);

    arena                *arena       = vk_context->arena;
    vulkan_geometry_data *geo_vk_data = static_cast<vulkan_geometry_data *>(out_geometry->vulkan_geometry_state);

    if (!geo_vk_data)
    {
        out_geometry->vulkan_geometry_state = dallocate(arena, sizeof(vulkan_geometry_data), MEM_TAG_RENDERER);
        geo_vk_data                         = static_cast<vulkan_geometry_data *>(out_geometry->vulkan_geometry_state);

        u32 low = INVALID_ID;

        for (s32 i = 0; i < MAX_GEOMETRIES_LOADED; i++)
        {
            if (internal_array[i].id == INVALID_ID)
            {
                geo_vk_data->id                 = i;
                internal_array[i].id            = i;
                internal_array[i].vertex_count  = vertex_count;
                internal_array[i].indices_count = index_count;
                break;
            }
        }
        if (geo_vk_data->id == INVALID_ID)
        {
            DERROR("Couldnt load geometry into vulkan_internal_geometries. Maybe we run out space??");
            return false;
        }
    }

    u32 id                           = geo_vk_data->id;
    internal_array[id].vertex_count  = vertex_count;
    internal_array[id].indices_count = index_count;
    geo_vk_data->indices_count       = index_count;
    geo_vk_data->vertex_count        = vertex_count;

    return true;
};
bool vulkan_destroy_geometry(geometry *geometry)
{
    // TODO: we still havenet freed this data from the vertex and index buffers.
    // if (geometry->id < MAX_GEOMETRIES_LOADED)
    //{
    //    vk_context->internal_geometries[geometry->id].id            = INVALID_ID;
    //    vk_context->internal_geometries[geometry->id].vertex_count  = INVALID_ID;
    //    vk_context->internal_geometries[geometry->id].indices_count = INVALID_ID;
    //}

    // dfree(geometry->vulkan_geometry_state, sizeof(vulkan_geometry_data), MEM_TAG_RENDERER);

    return true;
};

bool vulkan_draw_geometries(vulkan_shader *material_shader_vulkan_data, geometry_type type, u32 geometry_count,
                            geometry **geos, VkCommandBuffer *curr_command_buffer, u32 curr_frame_index)
{
    ZoneScoped;

    vulkan_shader *vk_shader = material_shader_vulkan_data;

    // bind the globals

    VkBuffer     vertex_buffers[1] = {};
    VkDeviceSize offsets[]         = {0};

    vulkan_geometry_data *internal_vulkan_geo_data_array = nullptr;

    u32 aligned_global     = vk_shader->per_frame_uniforms_size[0];
    u32 dynamic_offsets[2] = {curr_frame_index * vk_shader->per_frame_stride,
                              curr_frame_index * vk_shader->per_frame_stride + aligned_global};

    if (type == GEO_TYPE_3D)
    {
        vertex_buffers[0] = vk_context->world_renderpass.vertex_buffer.handle;
        vkCmdBindVertexBuffers(*curr_command_buffer, 0, 1, vertex_buffers, offsets);
        vkCmdBindIndexBuffer(*curr_command_buffer, vk_context->world_renderpass.index_buffer.handle, 0,
                             VK_INDEX_TYPE_UINT32);
        internal_vulkan_geo_data_array = vk_context->world_internal_geometries;
        vkCmdBindDescriptorSets(*curr_command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vk_shader->pipeline.layout, 0, 1,
                                &vk_shader->per_frame_descriptor_set, 2, dynamic_offsets);
    }
    else if (type == GEO_TYPE_2D)
    {
        vertex_buffers[0] = vk_context->ui_renderpass.vertex_buffer.handle;
        vkCmdBindVertexBuffers(*curr_command_buffer, 0, 1, vertex_buffers, offsets);
        vkCmdBindIndexBuffer(*curr_command_buffer, vk_context->ui_renderpass.index_buffer.handle, 0,
                             VK_INDEX_TYPE_UINT32);
        internal_vulkan_geo_data_array = vk_context->ui_internal_geometries;
        vkCmdBindDescriptorSets(*curr_command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vk_shader->pipeline.layout, 0, 1,
                                &vk_shader->per_frame_descriptor_set, 1, dynamic_offsets);
    }

    vk_push_constant pc{};

    for (u32 i = 0; i < geometry_count; i++)
    {

        material *mat = geos[i]->material;

        vulkan_geometry_data *vk_data = static_cast<vulkan_geometry_data *>(geos[i]->vulkan_geometry_state);

        u32 index_offset  = vulkan_calculate_index_offset(vk_context, vk_data->id, internal_vulkan_geo_data_array);
        u32 vertex_offset = vulkan_calculate_vertex_offset(vk_context, vk_data->id, internal_vulkan_geo_data_array);

        u32 descriptor_set_index = mat->internal_id;

        vkCmdBindDescriptorSets(*curr_command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vk_shader->pipeline.layout, 1, 1,
                                &vk_shader->per_group_descriptor_sets[descriptor_set_index], 0, nullptr);

        pc.material_pc.model = geos[i]->ubo.model;
        pc.material_pc.color = mat->diffuse_color;
        vkCmdPushConstants(*curr_command_buffer, vk_shader->pipeline.layout, VK_SHADER_STAGE_VERTEX_BIT, 0,
                           sizeof(vk_push_constant), &pc);

        vkCmdDrawIndexed(*curr_command_buffer, vk_data->indices_count, 1, index_offset, vertex_offset, 0);
    }
    return true;
}

bool vulkan_draw_grid(vulkan_shader *grid_shader_vulkan_data, VkCommandBuffer *curr_command_buffer)
{
    vulkan_shader *shader = grid_shader_vulkan_data;
    DASSERT(shader);
    vkCmdDraw(*curr_command_buffer, 6, 1, 0, 0);
    return true;
}

bool vulkan_draw_skybox(vulkan_shader *skybox_shader_vulkan_data, render_data *data,
                        VkCommandBuffer *curr_command_buffer, u32 curr_frame_index)
{
    // BIG HACK
    // HACK:
    vulkan_shader *shader = skybox_shader_vulkan_data;
    DASSERT(shader);

    u32 dynamic_offsets[1] = {curr_frame_index * shader->per_frame_stride};
    vkCmdBindDescriptorSets(*curr_command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, shader->pipeline.layout, 0, 1,
                            &shader->per_frame_descriptor_set, 1, dynamic_offsets);

    VkBuffer     vertex_buffers[] = {vk_context->world_renderpass.vertex_buffer.handle};
    VkDeviceSize offsets[]        = {0};

    vkCmdBindVertexBuffers(*curr_command_buffer, 0, 1, vertex_buffers, offsets);
    vkCmdBindIndexBuffer(*curr_command_buffer, vk_context->world_renderpass.index_buffer.handle, 0,
                         VK_INDEX_TYPE_UINT32);

    geometry *cube_geo = geometry_system_get_default_geometry();
    dstring   mat_name = DEFAULT_CUBEMAP_TEXTURE_HANDLE;
    material *cube_mat = material_system_get_from_name(&mat_name);

    vulkan_geometry_data *geo_data = static_cast<vulkan_geometry_data *>(cube_geo->vulkan_geometry_state);
    u32 index_offset  = vulkan_calculate_index_offset(vk_context, geo_data->id, vk_context->world_internal_geometries);
    u32 vertex_offset = vulkan_calculate_vertex_offset(vk_context, geo_data->id, vk_context->world_internal_geometries);

    u32 descriptor_set_index = cube_mat->internal_id;

    vkCmdBindDescriptorSets(*curr_command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, shader->pipeline.layout, 1, 1,
                            &shader->per_group_descriptor_sets[descriptor_set_index], 0, nullptr);

    vkCmdDrawIndexed(*curr_command_buffer, geo_data->indices_count, 1, index_offset, vertex_offset, 0);

    return true;
}

bool vulkan_draw_frame(render_data *render_data)
{
    ZoneScoped;
    u32      current_frame = vk_context->current_frame_index;
    VkResult result;

    {
        VK_CHECK(vkWaitForFences(vk_context->vk_device.logical, 1, &vk_context->in_flight_fences[current_frame],
                                 VK_TRUE, INVALID_ID_64));
        VK_CHECK(vkResetFences(vk_context->vk_device.logical, 1, &vk_context->in_flight_fences[current_frame]));
    }

    // HACK:

    u32 image_index = INVALID_ID;
    result = vkAcquireNextImageKHR(vk_context->vk_device.logical, vk_context->vk_swapchain.handle, INVALID_ID_64,
                                   vk_context->image_available_semaphores[current_frame], VK_NULL_HANDLE, &image_index);

    static vulkan_image swapchain_img{};
    swapchain_img.handle = vk_context->vk_swapchain.images[image_index];
    swapchain_img.format = vk_context->vk_swapchain.img_format;
    swapchain_img.view   = vk_context->vk_swapchain.img_views[image_index];

    // transition it
    if (vk_context->swapchain_recreated[current_frame])
    {
        vulkan_transition_image_layout(vk_context, &vk_context->graphics_command_pool,
                                       &vk_context->vk_device.graphics_queue, &swapchain_img, VK_IMAGE_LAYOUT_UNDEFINED,
                                       VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
        vk_context->swapchain_recreated[current_frame] = false;
    }
    else
    {
        vulkan_transition_image_layout(vk_context, &vk_context->graphics_command_pool,
                                       &vk_context->vk_device.graphics_queue, &swapchain_img,
                                       VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
    }

    if (result & VK_ERROR_OUT_OF_DATE_KHR)
    {
        vulkan_recreate_swapchain(vk_context);
        DDEBUG("VK_ERROR_OUT_OF_DATE_KHR");
        return false;
    }
    VK_CHECK(result);

    VkCommandBuffer &curr_command_buffer = vk_context->command_buffers[current_frame];

    // HACK:
    vkResetCommandBuffer(curr_command_buffer, 0);
    vulkan_begin_command_buffer_single_use(vk_context, curr_command_buffer);

    shader *material_shader = shader_system_get_shader(vk_context->default_material_shader_id);
    DASSERT(material_shader);

    shader *skybox_shader = shader_system_get_shader(vk_context->default_skybox_shader_id);
    DASSERT(skybox_shader);

    shader *grid_shader = shader_system_get_shader(vk_context->default_grid_shader_id);
    DASSERT(grid_shader);

    shader *ui_shader = shader_system_get_shader(vk_context->default_ui_shader_id);
    DASSERT(ui_shader);

    vulkan_shader *vk_material_shader = static_cast<vulkan_shader *>(material_shader->internal_vulkan_shader_state);
    vulkan_shader *vk_skybox_shader   = static_cast<vulkan_shader *>(skybox_shader->internal_vulkan_shader_state);
    vulkan_shader *vk_grid_shader     = static_cast<vulkan_shader *>(grid_shader->internal_vulkan_shader_state);
    vulkan_shader *vk_ui_shader       = static_cast<vulkan_shader *>(ui_shader->internal_vulkan_shader_state);

    vulkan_begin_renderpass(vk_context, WORLD_RENDERPASS, curr_command_buffer, current_frame);

    vkCmdBindPipeline(curr_command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vk_skybox_shader->pipeline.handle);
    vulkan_draw_skybox(vk_skybox_shader, render_data, &curr_command_buffer, current_frame);

    vkCmdBindPipeline(curr_command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vk_grid_shader->pipeline.handle);
    vulkan_draw_grid(vk_grid_shader, &curr_command_buffer);

    vkCmdBindPipeline(curr_command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vk_material_shader->pipeline.handle);
    vulkan_draw_geometries(vk_material_shader, GEO_TYPE_3D, render_data->geometry_count_3D,
                           render_data->test_geometry_3D, &curr_command_buffer, current_frame);

    vulkan_end_renderpass(&curr_command_buffer);

    vkCmdBindPipeline(curr_command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vk_ui_shader->pipeline.handle);
    vulkan_begin_renderpass(vk_context, UI_RENDERPASS, curr_command_buffer, current_frame);
    vulkan_draw_geometries(vk_ui_shader, GEO_TYPE_2D, render_data->geometry_count_2D, render_data->test_geometry_2D,
                           &curr_command_buffer, current_frame);
    vulkan_end_renderpass(&curr_command_buffer);

    vulkan_end_command_buffer_single_use(vk_context, curr_command_buffer, false);
    //

    VkSemaphore          wait_semaphores[]   = {vk_context->image_available_semaphores[current_frame]};
    VkSemaphore          signal_semaphores[] = {vk_context->render_finished_semaphores[current_frame]};
    VkPipelineStageFlags wait_stages[]       = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};

    {

        // transition it back
        vulkan_transition_image_layout(vk_context, &vk_context->graphics_command_pool,
                                       &vk_context->vk_device.graphics_queue, &swapchain_img,
                                       VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

        ZoneScopedN("queue submit");
        VkSubmitInfo submit_info{};
        submit_info.sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submit_info.waitSemaphoreCount   = 1;
        submit_info.pWaitSemaphores      = wait_semaphores;
        submit_info.pWaitDstStageMask    = wait_stages;
        submit_info.commandBufferCount   = 1;
        submit_info.pCommandBuffers      = &curr_command_buffer;
        submit_info.signalSemaphoreCount = 1;
        submit_info.pSignalSemaphores    = signal_semaphores;

        result = vkQueueSubmit(vk_context->vk_device.graphics_queue, 1, &submit_info,
                               vk_context->in_flight_fences[current_frame]);
        VK_CHECK(result);
        vkQueueWaitIdle(vk_context->vk_device.graphics_queue);
    }

    VkSwapchainKHR swapchains[1] = {};
    swapchains[0]                = vk_context->vk_swapchain.handle;

    VkPresentInfoKHR present_info{};
    present_info.sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    present_info.waitSemaphoreCount = 1;
    present_info.pWaitSemaphores    = signal_semaphores;
    present_info.swapchainCount     = 1;
    present_info.pSwapchains        = &vk_context->vk_swapchain.handle;
    present_info.pImageIndices      = &image_index;

    result = vkQueuePresentKHR(vk_context->vk_device.present_queue, &present_info);
    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
    {
        DDEBUG("VK_ERROR_OUT_OF_DATE_KHR or VK_SUBOTIMAL_KHR");
        vulkan_recreate_swapchain(vk_context);
    }
    VK_CHECK(result);

    vk_context->current_frame_index++;
    vk_context->current_frame_index %= MAX_FRAMES_IN_FLIGHT;
    vk_context->frame_counter++;

    return true;
}
bool vulkan_backend_resize()
{
    if (vk_context)
    {
        bool result = vulkan_recreate_swapchain(vk_context);
        return true;
    }
    return false;
}
