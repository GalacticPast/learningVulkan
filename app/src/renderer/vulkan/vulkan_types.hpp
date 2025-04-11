#pragma once
#include "core/dasserts.hpp"
#include <vulkan/vulkan.h>

#define VK_CHECK(expr)                                                                                                 \
    {                                                                                                                  \
        DASSERT(expr == VK_SUCCESS);                                                                                   \
    }

struct vulkan_device
{

    VkDevice logical;

    VkPhysicalDevice            physical;
    VkPhysicalDeviceProperties *physical_properties;
    VkPhysicalDeviceFeatures   *physical_features;

    u32                      queue_family_count;
    VkQueueFamilyProperties *queue_family_properties;

    // INFO: might need the selected graphics queue's QueueFamilyProperties.
    u32 graphics_family_index = INVALID_ID;
    u32 present_family_index  = INVALID_ID;
};

struct vulkan_swapchain
{
    VkSurfaceCapabilitiesKHR surface_capabilities;
    // TODO: linear alloc??
    u32                 surface_formats_count = INVALID_ID;
    VkSurfaceFormatKHR *surface_formats;

    u32               present_modes_count = INVALID_ID;
    VkPresentModeKHR *present_modes;

    VkExtent2D surface_extent;
};

struct vulkan_context
{
    vulkan_device device;

    vulkan_swapchain vk_swapchain;

    // INFO: maybe should be inside vulkan_device?
    VkQueue graphics_queue;
    VkQueue present_queue;

    VkSurfaceKHR vk_surface;

    VkDebugUtilsMessengerEXT vk_dbg_messenger;
    VkAllocationCallbacks   *vk_allocator;

    VkInstance vk_instance;

    u32         enabled_layer_count;
    const char *enabled_layer_names[4];

    u32         enabled_extension_count;
    const char *enabled_extension_names[4];
};
