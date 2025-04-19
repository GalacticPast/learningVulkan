#pragma once
#include "containers/darray.hpp"
#include "core/dasserts.hpp"
#include <vulkan/vulkan.h>

#define VK_CHECK(expr)                                                                                                 \
    {                                                                                                                  \
        DASSERT(expr == VK_SUCCESS);                                                                                   \
    }

#define MAX_FRAMES_IN_FLIGHT 2

struct vulkan_device
{

    VkDevice         logical;
    VkPhysicalDevice physical;

    VkQueue graphics_queue;
    VkQueue present_queue;

    VkPhysicalDeviceMemoryProperties memory_properties;

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
    VkImage        handle;
    VkImageView    view;
    VkFormat       format;
    VkDeviceMemory memory;
};

struct vulkan_framebuffer
{
    VkFramebuffer handle;
};

struct vulkan_swapchain
{

    u32 width;
    u32 height;

    VkSwapchainKHR      handle;
    VkExtent2D          surface_extent;
    vulkan_framebuffer *buffers;

    u32          images_count = INVALID_ID;
    VkImage     *images;
    VkImageView *img_views;
    VkFormat     img_format;

    vulkan_image depth_image;

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

struct vulkan_buffer
{
    VkBuffer       handle;
    VkDeviceMemory memory;
};
struct vulkan_context
{
    u32 current_frame_index;

    vulkan_device vk_device;

    vulkan_swapchain vk_swapchain;

    VkRenderPass vk_renderpass;

    vulkan_pipeline vk_graphics_pipeline;

    vulkan_buffer *global_uniform_buffers;
    darray<void *> global_uniform_buffers_memory_data;

    VkCommandBuffer *command_buffers;

    vulkan_buffer vertex_buffer;
    vulkan_buffer index_buffer;

    // HACK: remove this later
    vulkan_image default_texture;
    //
    VkCommandPool           graphics_command_pool;
    VkDescriptorPool        descriptor_command_pool;
    VkDescriptorSetLayout   global_uniform_descriptor_layout;
    darray<VkDescriptorSet> descriptor_sets;

    VkSemaphore *image_available_semaphores;
    VkSemaphore *render_finished_semaphores;
    VkFence     *in_flight_fences;

    VkSurfaceKHR vk_surface;

    VkDebugUtilsMessengerEXT vk_dbg_messenger;
    VkAllocationCallbacks   *vk_allocator;

    VkInstance vk_instance;

    u32         enabled_layer_count = INVALID_ID;
    const char *enabled_layer_names[4];

    u32         enabled_extension_count = INVALID_ID;
    const char *enabled_extension_names[4];
};
