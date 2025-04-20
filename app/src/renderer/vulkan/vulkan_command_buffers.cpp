#include "vulkan_command_buffers.hpp"
#include "core/application.hpp"

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

bool vulkan_create_descriptor_command_pool_n_sets(vulkan_context *vk_context)
{
    VkDescriptorPoolSize pool_sizes[2]{};

    pool_sizes[0].type            = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    pool_sizes[0].descriptorCount = MAX_FRAMES_IN_FLIGHT;

    pool_sizes[1].type            = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    pool_sizes[1].descriptorCount = MAX_FRAMES_IN_FLIGHT;

    VkDescriptorPoolCreateInfo pool_create_info{};
    pool_create_info.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    pool_create_info.pNext         = 0;
    pool_create_info.flags         = 0;
    pool_create_info.maxSets       = MAX_FRAMES_IN_FLIGHT;
    pool_create_info.poolSizeCount = 2;
    pool_create_info.pPoolSizes    = pool_sizes;

    VkResult result = vkCreateDescriptorPool(vk_context->vk_device.logical, &pool_create_info, vk_context->vk_allocator,
                                             &vk_context->descriptor_command_pool);
    VK_CHECK(result);

    darray<VkDescriptorSetLayout> desc_set_layout(MAX_FRAMES_IN_FLIGHT);
    for (u32 i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        desc_set_layout[i] = vk_context->descriptor_layout;
    }

    VkDescriptorSetAllocateInfo desc_set_alloc_info{};
    desc_set_alloc_info.sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    desc_set_alloc_info.pNext              = 0;
    desc_set_alloc_info.descriptorPool     = vk_context->descriptor_command_pool;
    desc_set_alloc_info.descriptorSetCount = MAX_FRAMES_IN_FLIGHT;
    desc_set_alloc_info.pSetLayouts        = (const VkDescriptorSetLayout *)desc_set_layout.data;

    vk_context->descriptor_sets.resize(MAX_FRAMES_IN_FLIGHT);
    result = vkAllocateDescriptorSets(vk_context->vk_device.logical, &desc_set_alloc_info,
                                      (VkDescriptorSet *)vk_context->descriptor_sets.data);

    VK_CHECK(result);

    for (u32 i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        VkDescriptorBufferInfo desc_buffer_info{};

        desc_buffer_info.buffer = vk_context->global_uniform_buffers[i].handle;
        desc_buffer_info.offset = 0;
        desc_buffer_info.range  = sizeof(uniform_buffer_object);

        VkDescriptorImageInfo desc_image_info{};
        desc_image_info.sampler     = vk_context->default_tex_sampler;
        desc_image_info.imageView   = vk_context->default_texture.view;
        desc_image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        VkWriteDescriptorSet desc_writes[2]{};

        desc_writes[0].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        desc_writes[0].pNext            = 0;
        desc_writes[0].dstSet           = vk_context->descriptor_sets[i];
        desc_writes[0].dstBinding       = 0;
        desc_writes[0].dstArrayElement  = 0;
        desc_writes[0].descriptorCount  = 1;
        desc_writes[0].descriptorType   = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        desc_writes[0].pBufferInfo      = &desc_buffer_info;
        desc_writes[0].pImageInfo       = 0;
        desc_writes[0].pTexelBufferView = 0;

        desc_writes[1].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        desc_writes[1].pNext            = 0;
        desc_writes[1].dstSet           = vk_context->descriptor_sets[i];
        desc_writes[1].dstBinding       = 1;
        desc_writes[1].dstArrayElement  = 0;
        desc_writes[1].descriptorCount  = 1;
        desc_writes[1].descriptorType   = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        desc_writes[1].pBufferInfo      = 0;
        desc_writes[1].pImageInfo       = &desc_image_info;
        desc_writes[1].pTexelBufferView = 0;

        vkUpdateDescriptorSets(vk_context->vk_device.logical, 2, desc_writes, 0, nullptr);
    }

    return true;
}

bool vulkan_allocate_command_buffers(vulkan_context *vk_context, VkCommandPool *command_pool,
                                     VkCommandBuffer *cmd_buffers, u32 command_buffers_count, bool is_single_use)
{
    VkCommandBufferAllocateInfo command_buffer_alloc_info{};

    command_buffer_alloc_info.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    command_buffer_alloc_info.pNext              = nullptr;
    command_buffer_alloc_info.commandPool        = *command_pool;
    command_buffer_alloc_info.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    command_buffer_alloc_info.commandBufferCount = command_buffers_count;

    VkResult result = vkAllocateCommandBuffers(vk_context->vk_device.logical, &command_buffer_alloc_info, cmd_buffers);
    VK_CHECK(result);

    if (is_single_use)
    {
        vulkan_begin_command_buffer_single_use(vk_context, *cmd_buffers);
    }

    return true;
}

void vulkan_begin_command_buffer_single_use(vulkan_context *vk_context, VkCommandBuffer command_buffer)
{
    VkCommandBufferBeginInfo command_buffer_begin_info{};
    command_buffer_begin_info.sType            = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    command_buffer_begin_info.flags            = 0;       // Optional
    command_buffer_begin_info.pInheritanceInfo = nullptr; // Optional

    VkResult result = vkBeginCommandBuffer(command_buffer, &command_buffer_begin_info);
    VK_CHECK(result);
}
void vulkan_end_command_buffer_single_use(vulkan_context *vk_context, VkCommandBuffer command_buffer)
{
    VkResult result = vkEndCommandBuffer(command_buffer);
    VK_CHECK(result);
}
void vulkan_free_command_buffers(vulkan_context *vk_context, VkCommandPool *command_pool, VkCommandBuffer *cmd_buffers,
                                 u32 cmd_buffers_count)
{
    vkFreeCommandBuffers(vk_context->vk_device.logical, *command_pool, cmd_buffers_count, cmd_buffers);
}
