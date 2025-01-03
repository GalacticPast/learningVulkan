#pragma once
#include "core/asserts.h"
#include "defines.h"
#include <vulkan/vulkan.h>

#define VK_CHECK(expr)                                                                                                                               \
    {                                                                                                                                                \
        ASSERT(expr == VK_SUCCESS);                                                                                                                  \
    }

typedef struct vulkan_swapchain
{
    VkSwapchainKHR     handle;
    VkSurfaceFormatKHR format;
    VkPresentModeKHR   present_mode;
    VkExtent2D         extent2D;
    VkImage           *images;
} vulkan_swapchain;

typedef struct vulkan_swapchain_support_details
{
    VkSurfaceCapabilitiesKHR capabilities;
    u32                      format_count;
    VkSurfaceFormatKHR      *formats;
    u32                      present_mode_count;
    VkPresentModeKHR        *present_modes;
} vulkan_swapchain_support_details;

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

    vulkan_swapchain_support_details swapchain_support_details;
} vulkan_device;

typedef struct vulkan_context
{
    VkInstance               instance;
    VkDebugUtilsMessengerEXT debug_messenger;

    vulkan_device device;

    VkSurfaceKHR surface;

    vulkan_swapchain swapchain;

    // TODO: idk where to put this so it stays here
    VkImageView *image_views;

} vulkan_context;
