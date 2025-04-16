#include "vulkan_buffer.hpp"

bool vulkan_create_buffer(vulkan_context *vk_context, vulkan_buffer *out_buffer, VkBufferUsageFlags usg_flags,
                          VkMemoryPropertyFlags memory_properties_flags, u64 buffer_size)
{
    VkBufferCreateInfo buffer_create_info{};

    buffer_create_info.sType       = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    buffer_create_info.pNext       = 0;
    buffer_create_info.flags       = 0;
    buffer_create_info.size        = buffer_size;
    buffer_create_info.usage       = usg_flags;
    buffer_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VkResult result = vkCreateBuffer(vk_context->vk_device.logical, &buffer_create_info, vk_context->vk_allocator,
                                     &out_buffer->handle);
    VK_CHECK(result);

    VkMemoryRequirements mem_req;
    vkGetBufferMemoryRequirements(vk_context->vk_device.logical, out_buffer->handle, &mem_req);

    // find suitable memory type suitable for buffer
    VkPhysicalDeviceMemoryProperties *memory_properties = &vk_context->vk_device.memory_properties;

    u32 memory_type_index = INVALID_ID;
    for (u32 i = 0; i < memory_properties->memoryTypeCount; i++)
    {
        if (mem_req.memoryTypeBits & (1 << i) &&
            (memory_properties->memoryTypes[i].propertyFlags & memory_properties_flags) == memory_properties_flags)
        {
            memory_type_index = i;
            break;
        }
    }

    VkMemoryAllocateInfo memory_allocate_info{};
    memory_allocate_info.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    memory_allocate_info.pNext           = 0;
    memory_allocate_info.allocationSize  = mem_req.size;
    memory_allocate_info.memoryTypeIndex = memory_type_index;

    result = vkAllocateMemory(vk_context->vk_device.logical, &memory_allocate_info, vk_context->vk_allocator,
                              &out_buffer->memory);
    VK_CHECK(result);

    result = vkBindBufferMemory(vk_context->vk_device.logical, out_buffer->handle, out_buffer->memory, 0);
    VK_CHECK(result);
    return true;
}

bool vulkan_copy_buffer(vulkan_context *vk_context, vulkan_buffer *dst_buffer, vulkan_buffer *src_buffer,
                        u64 buffer_size)
{
    VkCommandBufferAllocateInfo buffer_alloc_info{};

    buffer_alloc_info.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    buffer_alloc_info.pNext              = 0;
    buffer_alloc_info.commandPool        = vk_context->graphics_command_pool;
    buffer_alloc_info.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    buffer_alloc_info.commandBufferCount = 1;

    VkCommandBuffer command_buffer;

    VkResult result = vkAllocateCommandBuffers(vk_context->vk_device.logical, &buffer_alloc_info, &command_buffer);
    VK_CHECK(result);

    VkCommandBufferBeginInfo cmd_buffer_begin_info{};
    cmd_buffer_begin_info.sType            = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    cmd_buffer_begin_info.pNext            = 0;
    cmd_buffer_begin_info.flags            = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    cmd_buffer_begin_info.pInheritanceInfo = 0;

    result = vkBeginCommandBuffer(command_buffer, &cmd_buffer_begin_info);
    VK_CHECK(result);

    VkBufferCopy cpy_region{};
    cpy_region.srcOffset = 0;
    cpy_region.dstOffset = 0;
    cpy_region.size      = buffer_size;

    vkCmdCopyBuffer(command_buffer, src_buffer->handle, dst_buffer->handle, 1, &cpy_region);
    vkEndCommandBuffer(command_buffer);

    VkSubmitInfo queue_submit_info{};
    queue_submit_info.sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    queue_submit_info.pNext                = 0;
    queue_submit_info.waitSemaphoreCount   = 0;
    queue_submit_info.pWaitSemaphores      = 0;
    queue_submit_info.pWaitDstStageMask    = 0;
    queue_submit_info.commandBufferCount   = 1;
    queue_submit_info.pCommandBuffers      = &command_buffer;
    queue_submit_info.signalSemaphoreCount = 0;
    queue_submit_info.pSignalSemaphores    = 0;

    result = vkQueueSubmit(vk_context->vk_graphics_queue, 1, &queue_submit_info, VK_NULL_HANDLE);
    VK_CHECK(result);
    result = vkQueueWaitIdle(vk_context->vk_graphics_queue);
    VK_CHECK(result);

    vkFreeCommandBuffers(vk_context->vk_device.logical, vk_context->graphics_command_pool, 1, &command_buffer);
    return true;
}

bool vulkan_create_index_buffer(vulkan_context *vk_context)
{
    // 32 mbs
    u64 index_buffer_size = 1 * 1024 * 1024;
    vulkan_create_buffer(vk_context, &vk_context->index_buffer,
                         VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                         VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, index_buffer_size);

    return true;
}

bool vulkan_create_vertex_buffer(vulkan_context *vk_context)
{
    // 32 mbs
    u64 vertex_buffer_size = 1 * 1024 * 1024;
    vulkan_create_buffer(vk_context, &vk_context->vertex_buffer,
                         VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                         VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, vertex_buffer_size);

    return true;
}
bool vulkan_destroy_buffer(vulkan_context *vk_context, vulkan_buffer *buffer)
{
    vkDestroyBuffer(vk_context->vk_device.logical, buffer->handle, vk_context->vk_allocator);
    vkFreeMemory(vk_context->vk_device.logical, buffer->memory, nullptr);

    buffer->handle = 0;
    buffer->memory = 0;

    return true;
}
