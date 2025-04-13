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

    VkPhysicalDevice physical;

    VkPhysicalDeviceProperties *physical_properties;
    VkPhysicalDeviceFeatures   *physical_features;

    u32                      queue_family_count;
    VkQueueFamilyProperties *queue_family_properties;

    // INFO: might need the selected graphics queue's QueueFamilyProperties.
    u32 enabled_queue_family_count = INVALID_ID;
    u32 graphics_family_index      = INVALID_ID;
    u32 present_family_index       = INVALID_ID;
};

struct vulkan_image
{
    VkImage     *handles;
    VkImageView *views;
    VkFormat     format;
};

struct vulkan_framebuffer
{
    VkFramebuffer handle;
};

struct vulkan_swapchain
{

    VkSwapchainKHR      handle;
    VkExtent2D          surface_extent;
    vulkan_framebuffer *buffers;

    u32 curr_img_index;

    u32          images_count = INVALID_ID;
    vulkan_image vk_images;

    VkSurfaceCapabilitiesKHR surface_capabilities;
    // TODO: linear alloc??
    u32                 surface_formats_count = INVALID_ID;
    VkSurfaceFormatKHR *surface_formats;

    u32               present_modes_count = INVALID_ID;
    VkPresentModeKHR *present_modes;
};

struct vulkan_pipeline
{
    VkPipeline       handle;
    VkPipelineLayout layout;
};

struct vulkan_context
{
    vulkan_device vk_device;

    vulkan_swapchain vk_swapchain;

    VkRenderPass vk_renderpass;

    vulkan_pipeline vk_graphics_pipeline;

    VkCommandBuffer command_buffer;

    VkCommandPool graphics_command_pool;
    VkSemaphore   image_available_semaphore;
    VkSemaphore   render_finished_semaphore;
    VkFence       in_flight_fence;

    // INFO: maybe should be inside vulkan_device??
    VkQueue vk_graphics_queue;
    VkQueue vk_present_queue;

    VkSurfaceKHR vk_surface;

    VkDebugUtilsMessengerEXT vk_dbg_messenger;
    VkAllocationCallbacks   *vk_allocator;

    VkInstance vk_instance;

    u32         enabled_layer_count = INVALID_ID;
    const char *enabled_layer_names[4];

    u32         enabled_extension_count = INVALID_ID;
    const char *enabled_extension_names[4];
};
