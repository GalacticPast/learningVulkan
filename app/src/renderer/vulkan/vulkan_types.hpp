#pragma once
#include "core/dasserts.hpp"
#include <vulkan/vulkan.h>

#define VK_CHECK(expr)                                                                                                                                                                                 \
    {                                                                                                                                                                                                  \
        DASSERT(expr == VK_SUCCESS);                                                                                                                                                                   \
    }

struct vulkan_context
{
    VkAllocationCallbacks *vk_allocator;
    VkInstance vk_instance;
};
