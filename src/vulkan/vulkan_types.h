#pragma once
#include <vulkan/vulkan.h>

typedef struct vulkan_context
{
    VkInstance               instance;
    VkDebugUtilsMessengerEXT debug_messenger;
} vulkan_context;
