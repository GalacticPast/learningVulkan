#include "vulkan_sync_objects.h"
#include "core/memory.h"

b8 vulkan_create_sync_objects(vulkan_context *context)
{
    INFO("Creating sync objects...");
    VkSemaphoreCreateInfo semaphore_create_info = {};

    semaphore_create_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    semaphore_create_info.pNext = 0;
    semaphore_create_info.flags = 0;

    VkFenceCreateInfo fence_create_info = {};
    fence_create_info.sType             = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fence_create_info.pNext             = 0;
    fence_create_info.flags             = VK_FENCE_CREATE_SIGNALED_BIT;

    u32 sync_objects_size               = context->max_frames_in_flight;
    context->image_available_semaphores = ALLOCATE_MEMORY_RENDERER(sizeof(VkSemaphore) * sync_objects_size);
    context->render_finished_semaphores = ALLOCATE_MEMORY_RENDERER(sizeof(VkSemaphore) * sync_objects_size);
    context->in_flight_fences           = ALLOCATE_MEMORY_RENDERER(sizeof(VkFence) * sync_objects_size);

    for (u32 i = 0; i < sync_objects_size; i++)
    {
        VK_CHECK(vkCreateSemaphore(context->device.logical, &semaphore_create_info, 0, &context->image_available_semaphores[i]));
        VK_CHECK(vkCreateSemaphore(context->device.logical, &semaphore_create_info, 0, &context->render_finished_semaphores[i]));
        VK_CHECK(vkCreateFence(context->device.logical, &fence_create_info, 0, &context->in_flight_fences[i]));
    }

    INFO("Sync objects created.");

    return true;
}
