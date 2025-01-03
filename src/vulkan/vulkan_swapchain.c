#include "vulkan_swapchain.h"

b8 vulkan_create_swapchain(vulkan_context *context)
{

    VkSwapchainCreateInfoKHR swapchain_create_info = {};

    swapchain_create_info.sType   = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swapchain_create_info.pNext   = 0;
    swapchain_create_info.flags   = 0;
    swapchain_create_info.surface = context->surface;
    // swapchain_create_info.minImageCount;
    // swapchain_create_info.imageFormat;
    // swapchain_create_info.imageColorSpace;
    // swapchain_create_info.imageExtent;
    // swapchain_create_info.imageArrayLayers;
    // swapchain_create_info.imageUsage;
    // swapchain_create_info.imageSharingMode;
    // swapchain_create_info.queueFamilyIndexCount;
    // swapchain_create_info.pQueueFamilyIndices;
    // swapchain_create_info.preTransform;
    // swapchain_create_info.compositeAlpha;
    // swapchain_create_info.presentMode;
    // swapchain_create_info.clipped;
    // swapchain_create_info.oldSwapchain;

    // vkCreateSwapchainKHR();
    return false;
}
