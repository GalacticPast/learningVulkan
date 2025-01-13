#include "vulkan_buffer.h"
#include "math/math_types.h"
#include <string.h>

b8 vulkan_destroy_buffer(vulkan_context *context, vulkan_buffer *buffer);

b8 create_buffer(vulkan_context *context, vulkan_buffer *out_buffer, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties)
{
    VkBufferCreateInfo out_buffer_info    = {};
    out_buffer_info.sType                 = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    out_buffer_info.pNext                 = 0;
    out_buffer_info.flags                 = 0;
    out_buffer_info.size                  = out_buffer->size;
    out_buffer_info.usage                 = usage;
    out_buffer_info.sharingMode           = VK_SHARING_MODE_EXCLUSIVE;
    out_buffer_info.queueFamilyIndexCount = 0;
    out_buffer_info.pQueueFamilyIndices   = VK_NULL_HANDLE;

    VK_CHECK(vkCreateBuffer(context->device.logical, &out_buffer_info, 0, &out_buffer->handle));

    // memory
    VkPhysicalDeviceMemoryProperties mem_properties;
    vkGetPhysicalDeviceMemoryProperties(context->device.physical, &mem_properties);

    VkMemoryRequirements mem_requirements;
    vkGetBufferMemoryRequirements(context->device.logical, out_buffer->handle, &mem_requirements);

    u32 type_filter = 0;
    s32 index       = -1;
    for (u32 i = 0; i < mem_properties.memoryTypeCount; i++)
    {
        if ((mem_requirements.memoryTypeBits & (1 << i)) && (mem_properties.memoryTypes[i].propertyFlags & properties) == properties)
        {
            index = i;
            break;
        }
    }
    if (index == -1)
    {
        return false;
    }

    VkMemoryAllocateInfo memory_alloc_info = {};
    memory_alloc_info.sType                = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    memory_alloc_info.allocationSize       = mem_requirements.size;
    memory_alloc_info.memoryTypeIndex      = index;

    VK_CHECK(vkAllocateMemory(context->device.logical, &memory_alloc_info, 0, &out_buffer->memory));
    VK_CHECK(vkBindBufferMemory(context->device.logical, out_buffer->handle, out_buffer->memory, 0));

    return true;
}

b8 vulkan_copy_buffer(vulkan_context *context, vulkan_buffer *src_buffer, vulkan_buffer *dest_buffer)
{
    VkCommandBufferAllocateInfo command_buffer_alloc_info = {};
    command_buffer_alloc_info.sType                       = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    command_buffer_alloc_info.pNext                       = 0;
    command_buffer_alloc_info.commandPool                 = context->command_pool;
    command_buffer_alloc_info.level                       = 0;
    command_buffer_alloc_info.commandBufferCount          = 1;

    VkCommandBuffer command_buffer = {};
    VK_CHECK(vkAllocateCommandBuffers(context->device.logical, &command_buffer_alloc_info, &command_buffer));

    VkCommandBufferBeginInfo begin_info = {};
    begin_info.sType                    = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    begin_info.flags                    = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    VK_CHECK(vkBeginCommandBuffer(command_buffer, &begin_info));

    VkBufferCopy copy_region = {};
    copy_region.srcOffset    = 0; // Optional
    copy_region.dstOffset    = 0; // Optional
    copy_region.size         = src_buffer->size;

    vkCmdCopyBuffer(command_buffer, src_buffer->handle, dest_buffer->handle, 1, &copy_region);

    VK_CHECK(vkEndCommandBuffer(command_buffer));

    VkSubmitInfo submit_info       = {};
    submit_info.sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers    = &command_buffer;

    VK_CHECK(vkQueueSubmit(context->device.graphics_queue, 1, &submit_info, VK_NULL_HANDLE));
    VK_CHECK(vkQueueWaitIdle(context->device.graphics_queue));

    vkFreeCommandBuffers(context->device.logical, context->command_pool, 1, &command_buffer);

    return true;
}

b8 vulkan_create_vertex_buffer(vulkan_context *context)
{

    vulkan_buffer staging_buffer = {};
    staging_buffer.size          = sizeof(vertex_3d) * 1024 * 1024;

    if (!create_buffer(context, &staging_buffer, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT))
    {
        ERROR("Buffer creation failed");
        return false;
    }

    vertex_3d vertices[3] = {};

    vertices[0].position.x = 0.0f;
    vertices[0].position.y = -0.5f;
    vertices[0].color      = (vec3){1.0f, 0.0f, 0.0f};

    vertices[1].position.x = 0.5f;
    vertices[1].position.y = 0.5f;
    vertices[1].color      = (vec3){0.0f, 1.0f, 0.0f};

    vertices[2].position.x = -0.5f;
    vertices[2].position.y = 0.5f;
    vertices[2].color      = (vec3){0.0f, 0.0f, 1.0f};

    void *data;
    vkMapMemory(context->device.logical, staging_buffer.memory, 0, staging_buffer.size, 0, &data);
    memcpy(data, vertices, sizeof(vertex_3d) * 3);
    vkUnmapMemory(context->device.logical, staging_buffer.memory);

    context->vertex_buffer.size = staging_buffer.size;
    if (!create_buffer(context, &context->vertex_buffer, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT))
    {
        ERROR("Buffer creation failed");
        return false;
    }

    if (!vulkan_copy_buffer(context, &staging_buffer, &context->vertex_buffer))
    {
        ERROR("Error copying buffers");
        return false;
    }

    vulkan_destroy_buffer(context, &staging_buffer);

    return true;
}

b8 vulkan_destroy_buffer(vulkan_context *context, vulkan_buffer *buffer)
{
    if (buffer->handle)
    {
        vkDestroyBuffer(context->device.logical, buffer->handle, 0);
        vkFreeMemory(context->device.logical, buffer->memory, 0);

        buffer->handle = 0;
        buffer->memory = 0;
        buffer->size   = 0;
    }

    return true;
}

b8 vulkan_create_buffers(vulkan_context *context)
{
    if (!vulkan_create_vertex_buffer(context))
    {
        return false;
    }
    return true;
}
