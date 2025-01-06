#pragma once
#include "core/asserts.h"
#include "defines.h"
#include <vulkan/vulkan.h>

#define VK_CHECK(expr)                                                                                                                                                                                 \
    {                                                                                                                                                                                                  \
        ASSERT(expr == VK_SUCCESS);                                                                                                                                                                    \
    }

typedef struct vulkan_graphics_pipeline
{
    VkPipeline       handle;
    VkPipelineLayout layout;
} vulkan_graphics_pipeline;

typedef struct vulkan_swapchain
{
    VkSwapchainKHR     handle;
    VkSurfaceFormatKHR format;
    VkPresentModeKHR   present_mode;
    VkExtent2D         extent2D;
    u32                images_count;
    VkImage           *images;
    u32                image_views_count;
    VkImageView       *image_views;

    VkFramebuffer *frame_buffers;
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

    VkRenderPass             renderpass;
    vulkan_graphics_pipeline graphics_pipeline;

    // TODO: idk where to put this so it stays here
    VkCommandPool    command_pool;
    VkCommandBuffer *command_buffers;

    // TODO: idk where to put this so it stays here
    VkSemaphore *image_available_semaphores;
    VkSemaphore *render_finished_semaphores;

    VkFence *in_flight_fences;

    u32 max_frames_in_flight;

    u32 current_frame;
} vulkan_context;
