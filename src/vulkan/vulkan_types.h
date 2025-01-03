#pragma once
#include "core/asserts.h"
#include "defines.h"
#include <vulkan/vulkan.h>

#define VK_CHECK(expr)                                                                                                                               \
    {                                                                                                                                                \
        ASSERT(expr == VK_SUCCESS);                                                                                                                  \
    }

typedef struct vulkan_device
{
    VkPhysicalDevice           physical;
    VkPhysicalDeviceProperties physical_device_properties;
    VkPhysicalDeviceFeatures   physical_device_features;

    VkDevice logical;

    u32     graphics_queue_index;
    VkQueue graphics_queue;

    u32     present_queue_index;
    VkQueue present_queue;

} vulkan_device;

typedef struct vulkan_context
{
    VkInstance               instance;
    VkDebugUtilsMessengerEXT debug_messenger;

    vulkan_device device;

    VkSurfaceKHR surface;

} vulkan_context;
