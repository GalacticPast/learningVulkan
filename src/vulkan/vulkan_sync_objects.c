#include "vulkan_sync_objects.h"

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

    VK_CHECK(vkCreateSemaphore(context->device.logical, &semaphore_create_info, 0, &context->image_available_semaphore));
    VK_CHECK(vkCreateSemaphore(context->device.logical, &semaphore_create_info, 0, &context->render_finished_semaphore));
    VK_CHECK(vkCreateFence(context->device.logical, &fence_create_info, 0, &context->in_flight_fence));

    INFO("Sync objects created.");

    return true;
}
