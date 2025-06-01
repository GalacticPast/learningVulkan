#include "vulkan_buffers.hpp"
#include "defines.hpp"
#include "renderer/vulkan/vulkan_types.hpp"

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

bool vulkan_copy_buffer(vulkan_context *vk_context, VkCommandPool* cmd_pool,VkQueue* queue, vulkan_buffer *dst_buffer, u64 dst_offset,
                        vulkan_buffer *src_buffer, u64 buffer_size)
{

    VkCommandBuffer staging_command_buffer{};
    bool            result = vulkan_allocate_command_buffers(vk_context, cmd_pool,
                                                             &staging_command_buffer, 1, true);
    DASSERT_MSG(result == true, "Couldnt allocate command buffers");

    VkBufferCopy cpy_region{};
    cpy_region.srcOffset = 0;
    cpy_region.dstOffset = dst_offset;
    cpy_region.size      = buffer_size;

    vkCmdCopyBuffer(staging_command_buffer, src_buffer->handle, dst_buffer->handle, 1, &cpy_region);
    vulkan_end_command_buffer_single_use(vk_context, staging_command_buffer, false);

    VkSubmitInfo queue_submit_info{};
    queue_submit_info.sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    queue_submit_info.pNext                = 0;
    queue_submit_info.waitSemaphoreCount   = 0;
    queue_submit_info.pWaitSemaphores      = 0;
    queue_submit_info.pWaitDstStageMask    = 0;
    queue_submit_info.commandBufferCount   = 1;
    queue_submit_info.pCommandBuffers      = &staging_command_buffer;
    queue_submit_info.signalSemaphoreCount = 0;
    queue_submit_info.pSignalSemaphores    = 0;

    VkResult res = vkQueueSubmit(*queue, 1, &queue_submit_info, VK_NULL_HANDLE);
    VK_CHECK(res);
    result = vkQueueWaitIdle(*queue);
    VK_CHECK(res);

    vulkan_free_command_buffers(vk_context, cmd_pool, &staging_command_buffer, 1);
    return true;
}

bool vulkan_create_index_buffer(vulkan_context *vk_context)
{
    // 512 MBS
    u64 index_buffer_size = 1024 * 1024 * 1024;
    vulkan_create_buffer(vk_context, &vk_context->index_buffer,
                         VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                         VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, index_buffer_size);

    return true;
}

bool vulkan_create_vertex_buffer(vulkan_context *vk_context)
{
    // 512  mbs
    u64 vertex_buffer_size = 1024 * 1024 * 1024;
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

bool vulkan_copy_data_to_buffer(vulkan_context *vk_context, vulkan_buffer *src_buffer, void *to_be_mapped_data,
                                void *to_be_copied_data, u32 to_be_copied_data_size)
{
    VkResult result = vkMapMemory(vk_context->vk_device.logical, src_buffer->memory, 0, to_be_copied_data_size, 0,
                                  &to_be_mapped_data);
    VK_CHECK(result);
    dcopy_memory(to_be_mapped_data, to_be_copied_data, to_be_copied_data_size);
    vkUnmapMemory(vk_context->vk_device.logical, src_buffer->memory);
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
    command_buffer_begin_info.flags            = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT; // Optional
    command_buffer_begin_info.pInheritanceInfo = nullptr;                                     // Optional

    VkResult result = vkBeginCommandBuffer(command_buffer, &command_buffer_begin_info);
    VK_CHECK(result);
}

void vulkan_flush_command_buffer(vulkan_context *vk_context, VkQueue* queue,VkCommandBuffer command_buffer)
{
    // Create fence to ensure that the command buffer has finished executing
    VkFenceCreateInfo fence_info{};
    fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;

    VkFence fence;
    VK_CHECK(vkCreateFence(vk_context->vk_device.logical, &fence_info, nullptr, &fence));

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

    // Submit to the queue
    VkResult result = vkQueueSubmit(*queue, 1, &queue_submit_info, fence);
    // Wait for the fence to signal that command buffer has finished executing
    VK_CHECK(vkWaitForFences(vk_context->vk_device.logical, 1, &fence, VK_TRUE, INVALID_ID_64));
    vkQueueWaitIdle(*queue);

    vkDestroyFence(vk_context->vk_device.logical, fence, nullptr);
}

void vulkan_end_command_buffer_single_use(vulkan_context *vk_context, VkCommandBuffer command_buffer, bool free)
{
    VkResult result = vkEndCommandBuffer(command_buffer);
    if (free)
    {
        vulkan_free_command_buffers(vk_context, &vk_context->graphics_command_pool, &command_buffer, 1);
    }

    VK_CHECK(result);
}
void vulkan_free_command_buffers(vulkan_context *vk_context, VkCommandPool *command_pool, VkCommandBuffer *cmd_buffers,
                                 u32 cmd_buffers_count)
{
    vkFreeCommandBuffers(vk_context->vk_device.logical, *command_pool, cmd_buffers_count, cmd_buffers);
}
