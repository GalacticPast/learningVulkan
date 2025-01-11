#include "vulkan_buffer.h"
#include "math/math_types.h"
#include <string.h>

b8 vulkan_create_vertex_buffer(vulkan_context *context, vulkan_buffer *out_buffer)
{
    VkBufferCreateInfo out_buffer_info    = {};
    out_buffer_info.sType                 = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    out_buffer_info.pNext                 = 0;
    out_buffer_info.flags                 = 0;
    out_buffer_info.size                  = out_buffer->size;
    out_buffer_info.usage                 = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    out_buffer_info.sharingMode           = VK_SHARING_MODE_EXCLUSIVE;
    out_buffer_info.queueFamilyIndexCount = 0;
    out_buffer_info.pQueueFamilyIndices   = VK_NULL_HANDLE;

    VK_CHECK(vkCreateBuffer(context->device.logical, &out_buffer_info, 0, &context->vertex_buffer.handle));

    // memory
    VkPhysicalDeviceMemoryProperties mem_properties;
    vkGetPhysicalDeviceMemoryProperties(context->device.physical, &mem_properties);

    VkMemoryRequirements mem_requirements;
    vkGetBufferMemoryRequirements(context->device.logical, out_buffer->handle, &mem_requirements);

    u32 type_filter = 0;
    u32 properties  = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
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

    vertex_3d vertices[3] = {};

    vertices[0].position = (vec3){0.0f, -0.5f, 0.0f};
    vertices[0].color    = (vec3){1.0f, 0.0f, 0.0f};

    vertices[1].position = (vec3){0.5f, 0.5f, 0.0f};
    vertices[1].color    = (vec3){0.0f, 1.0f, 0.0f};

    vertices[2].position = (vec3){-0.5f, 0.5f, 0.0f};
    vertices[2].color    = (vec3){0.0f, 0.0f, 1.0f};

    void *data;
    vkMapMemory(context->device.logical, out_buffer->memory, 0, out_buffer->size, 0, &data);
    memcpy(data, vertices, sizeof(vec3) * 6);
    vkUnmapMemory(context->device.logical, out_buffer->memory);

    return true;
}

b8 vulkan_destroy_buffer(vulkan_context *context, vulkan_buffer *buffer)
{
    if (buffer->handle)
    {
        vkDestroyBuffer(context->device.logical, buffer->handle, 0);
        buffer->handle = 0;
        buffer->size   = 0;
    }

    return true;
}

b8 vulkan_create_buffers(vulkan_context *context)
{
    context->vertex_buffer.size = sizeof(vertex_3d) * 1024 * 1024;
    if (!vulkan_create_vertex_buffer(context, &context->vertex_buffer))
    {

        return false;
    }
    return true;
}
