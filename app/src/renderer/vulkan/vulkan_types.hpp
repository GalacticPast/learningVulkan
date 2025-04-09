#pragma once
#include "core/dasserts.hpp"
#include <vulkan/vulkan.h>

#define VK_CHECK(expr)                                                                                                                                                                                 \
    {                                                                                                                                                                                                  \
        DASSERT(expr == VK_SUCCESS);                                                                                                                                                                   \
    }

struct vulkan_device
{
    VkPhysicalDevice physical;
    VkPhysicalDeviceProperties *physical_properties;
    VkPhysicalDeviceFeatures *physical_features;
};

struct vulkan_context
{
    vulkan_device device;

    VkDebugUtilsMessengerEXT vk_dbg_messenger;

    VkAllocationCallbacks *vk_allocator;
    VkInstance vk_instance;
};
