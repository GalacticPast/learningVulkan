#pragma once
#include "core/asserts.h"
#include <vulkan/vulkan.h>

#define VK_CHECK(expr)                                                                                                                               \
    {                                                                                                                                                \
        ASSERT(expr == VK_SUCCESS);                                                                                                                  \
    }

typedef struct vulkan_device
{
    VkPhysicalDevice physical;
} vulkan_device;

typedef struct vulkan_context
{
    VkInstance               instance;
    VkDebugUtilsMessengerEXT debug_messenger;

    vulkan_device device;

} vulkan_context;
