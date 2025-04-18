#include "vulkan_sync_objects.hpp"
#include "core/dmemory.hpp"

bool vulkan_create_sync_objects(vulkan_context *vk_context)
{
    VkSemaphoreCreateInfo semaphore_create_info{};

    semaphore_create_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    semaphore_create_info.pNext = 0;
    semaphore_create_info.flags = 0;

    VkFenceCreateInfo fence_create_info{};

    fence_create_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fence_create_info.pNext = 0;
    fence_create_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    vk_context->image_available_semaphores =
        (VkSemaphore *)dallocate(sizeof(VkSemaphore) * MAX_FRAMES_IN_FLIGHT, MEM_TAG_RENDERER);
    vk_context->render_finished_semaphores =
        (VkSemaphore *)dallocate(sizeof(VkSemaphore) * MAX_FRAMES_IN_FLIGHT, MEM_TAG_RENDERER);

    vk_context->in_flight_fences = (VkFence *)dallocate(sizeof(VkFence) * MAX_FRAMES_IN_FLIGHT, MEM_TAG_RENDERER);

    for (u32 i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        VkResult result = vkCreateSemaphore(vk_context->vk_device.logical, &semaphore_create_info,
                                            vk_context->vk_allocator, &vk_context->image_available_semaphores[i]);
        VK_CHECK(result);

        result = vkCreateSemaphore(vk_context->vk_device.logical, &semaphore_create_info, vk_context->vk_allocator,
                                   &vk_context->render_finished_semaphores[i]);
        VK_CHECK(result);

        result = vkCreateFence(vk_context->vk_device.logical, &fence_create_info, vk_context->vk_allocator,
                               &vk_context->in_flight_fences[i]);
        VK_CHECK(result);
    }

    return true;
}
