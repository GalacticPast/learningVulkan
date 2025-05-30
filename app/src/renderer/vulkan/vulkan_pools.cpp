#include "core/application.hpp"
#include "math/dmath_types.hpp"
#include "renderer/vulkan/vulkan_types.hpp"
#include "resources/material_system.hpp"
#include "resources/resource_types.hpp"

#include "resources/material_system.hpp"

bool vulkan_create_graphics_command_pool(vulkan_context *vk_context)
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

bool vulkan_create_descriptor_command_pools(vulkan_context *vk_context)
{
    {
        // global descriptor command pool
        VkDescriptorPoolSize global_pool_sizes[1]{};

        global_pool_sizes[0].type            = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        global_pool_sizes[0].descriptorCount = MAX_FRAMES_IN_FLIGHT;

        VkDescriptorPoolCreateInfo global_descriptor_pool_create_info{};
        global_descriptor_pool_create_info.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        global_descriptor_pool_create_info.pNext         = 0;
        global_descriptor_pool_create_info.flags         = 0;
        global_descriptor_pool_create_info.maxSets       = MAX_FRAMES_IN_FLIGHT;
        global_descriptor_pool_create_info.poolSizeCount = 1;
        global_descriptor_pool_create_info.pPoolSizes    = global_pool_sizes;

        VkResult result = vkCreateDescriptorPool(vk_context->vk_device.logical, &global_descriptor_pool_create_info,
                                                 vk_context->vk_allocator, &vk_context->global_descriptor_command_pool);
        VK_CHECK(result);

        darray<VkDescriptorSetLayout> global_desc_set_layout(MAX_FRAMES_IN_FLIGHT);
        for (u32 i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
        {
            global_desc_set_layout[i] = vk_context->global_descriptor_layout;
        }

        VkDescriptorSetAllocateInfo global_desc_set_alloc_info{};
        global_desc_set_alloc_info.sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        global_desc_set_alloc_info.pNext              = 0;
        global_desc_set_alloc_info.descriptorPool     = vk_context->global_descriptor_command_pool;
        global_desc_set_alloc_info.descriptorSetCount = MAX_FRAMES_IN_FLIGHT;
        global_desc_set_alloc_info.pSetLayouts =
            reinterpret_cast<const VkDescriptorSetLayout *>(global_desc_set_layout.data);

        result = vkAllocateDescriptorSets(vk_context->vk_device.logical, &global_desc_set_alloc_info,
                                          static_cast<VkDescriptorSet *>(vk_context->global_descriptor_sets.data));
        VK_CHECK(result);
    }
    {
        // material
        VkDescriptorPoolSize material_pool_sizes[1]{};

        material_pool_sizes[0].type            = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        material_pool_sizes[0].descriptorCount = VULKAN_MAX_DESCRIPTOR_SET_COUNT;

        VkDescriptorPoolCreateInfo material_descriptor_pool_create_info{};
        material_descriptor_pool_create_info.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        material_descriptor_pool_create_info.pNext         = 0;
        material_descriptor_pool_create_info.flags         = 0;
        material_descriptor_pool_create_info.maxSets       = VULKAN_MAX_DESCRIPTOR_SET_COUNT;
        material_descriptor_pool_create_info.poolSizeCount = 1;
        material_descriptor_pool_create_info.pPoolSizes    = material_pool_sizes;

        VkResult result =
            vkCreateDescriptorPool(vk_context->vk_device.logical, &material_descriptor_pool_create_info,
                                   vk_context->vk_allocator, &vk_context->material_descriptor_command_pool);
        VK_CHECK(result);

        darray<VkDescriptorSetLayout> material_desc_set_layout(VULKAN_MAX_DESCRIPTOR_SET_COUNT);
        for (u32 i = 0; i < VULKAN_MAX_DESCRIPTOR_SET_COUNT; i++)
        {
            material_desc_set_layout[i] = vk_context->material_descriptor_layout;
        }

        VkDescriptorSetAllocateInfo material_desc_set_alloc_info{};
        material_desc_set_alloc_info.sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        material_desc_set_alloc_info.pNext              = 0;
        material_desc_set_alloc_info.descriptorPool     = vk_context->material_descriptor_command_pool;
        material_desc_set_alloc_info.descriptorSetCount = VULKAN_MAX_DESCRIPTOR_SET_COUNT;
        material_desc_set_alloc_info.pSetLayouts =
            reinterpret_cast<const VkDescriptorSetLayout *>(material_desc_set_layout.data);

        result = vkAllocateDescriptorSets(vk_context->vk_device.logical, &material_desc_set_alloc_info,
                                          static_cast<VkDescriptorSet *>(vk_context->material_descriptor_sets.data));
        VK_CHECK(result);
    }
    return true;
}

bool vulkan_update_materials_descriptor_set(vulkan_context *vk_context, material *material)
{
    VkDescriptorBufferInfo desc_buffer_info{};

    u32 descriptor_set_index = material->internal_id;
    DTRACE("Updataing descriptor_set_index: %d", descriptor_set_index);

    // for now we only have 2
    VkDescriptorImageInfo image_infos[2] = {};

    // INFO: in case
    // I dont get the default material in the start because the default material might not be initialized yet.
    // Because:desc_writes Create default material calls this function .
    struct material *default_mat = nullptr;

    // albedo
    {
        vulkan_texture *tex_state = nullptr;

        if (material->map.albedo)
        {
            tex_state = static_cast<vulkan_texture *>(material->map.albedo->vulkan_texture_state);
        }
        if (!tex_state)
        {
            if (default_mat)
            {
                DERROR("No aldbedo map found for material interal_id: %d", descriptor_set_index);
            }
            else
            {
                default_mat = material_system_get_default_material();
            }
            tex_state = static_cast<vulkan_texture *>(default_mat->map.albedo->vulkan_texture_state);
        }

        image_infos[0].sampler     = tex_state->sampler;
        image_infos[0].imageView   = tex_state->image.view;
        image_infos[0].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    }
    // alpha
    {
        vulkan_texture *tex_state = nullptr;

        if (material->map.alpha)
        {
            tex_state = static_cast<vulkan_texture *>(material->map.alpha->vulkan_texture_state);
        }
        if (!tex_state)
        {
            if (default_mat)
            {
                DERROR("No alpha map found for material interal_id: %d", descriptor_set_index);
            }
            else
            {
                default_mat = material_system_get_default_material();
            }
            tex_state = static_cast<vulkan_texture *>(default_mat->map.alpha->vulkan_texture_state);
        }

        image_infos[1].sampler     = tex_state->sampler;
        image_infos[1].imageView   = tex_state->image.view;
        image_infos[1].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    }

    VkWriteDescriptorSet desc_writes[2]{};
    // albdeo
    for (u32 i = 0; i < 2; i++)
    {
        desc_writes[i].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        desc_writes[i].pNext            = 0;
        desc_writes[i].dstSet           = vk_context->material_descriptor_sets[descriptor_set_index];
        desc_writes[i].dstBinding       = i;
        desc_writes[i].dstArrayElement  = 0;
        desc_writes[i].descriptorCount  = 1;
        desc_writes[i].descriptorType   = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        desc_writes[i].pBufferInfo      = 0;
        desc_writes[i].pImageInfo       = &image_infos[i];
        desc_writes[i].pTexelBufferView = 0;
    }

    vkUpdateDescriptorSets(vk_context->vk_device.logical, 2, desc_writes, 0, nullptr);
    return true;
}

bool vulkan_update_global_descriptor_sets(vulkan_context *vk_context, u32 frame_index)
{


    VkDescriptorBufferInfo desc_buffer_infos[2]{};
    VkWriteDescriptorSet desc_writes[2]{};
    VkDeviceSize ranges[2] = {sizeof(scene_global_uniform_buffer_object), sizeof(light_global_uniform_buffer_object)};
    vulkan_buffer buffers[2] = {vk_context->scene_global_uniform_buffers[frame_index], vk_context->light_global_uniform_buffers[frame_index]};

    for(u32 i = 0 ; i < 2 ; i++)
    {
        desc_buffer_infos[i].buffer = buffers[i].handle;
        desc_buffer_infos[i].offset = 0;
        desc_buffer_infos[i].range  = ranges[i];


        desc_writes[i].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        desc_writes[i].pNext            = 0;
        desc_writes[i].dstSet           = vk_context->global_descriptor_sets[frame_index];
        desc_writes[i].dstBinding       = i;
        desc_writes[i].dstArrayElement  = 0;
        desc_writes[i].descriptorCount  = 1;
        desc_writes[i].descriptorType   = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        desc_writes[i].pBufferInfo      = &desc_buffer_infos[i];
        desc_writes[i].pImageInfo       = 0;
        desc_writes[i].pTexelBufferView = 0;

    }
    vkUpdateDescriptorSets(vk_context->vk_device.logical, 2, desc_writes, 0, nullptr);
    return true;
}
