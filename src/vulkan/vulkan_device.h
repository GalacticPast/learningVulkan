#pragma once
#include "defines.h"
#include "vulkan_types.h"

b8   vulkan_create_logical_device(vulkan_context *context);
void vulkan_query_swapchain_support_details(VkPhysicalDevice physical_device, VkSurfaceKHR surface, vulkan_swapchain_support_details *swapchain_support_details);
