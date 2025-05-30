#pragma once
#include "containers/darray.hpp"
#include "core/application.hpp"
#include "core/dasserts.hpp"
#include "resources/resource_types.hpp"
#include <vulkan/vulkan.h>

#define VK_CHECK(expr)                                                                                                 \
    {                                                                                                                  \
        DASSERT(expr == VK_SUCCESS);                                                                                   \
    }

struct vulkan_geometry_data
{
    u32 id            = INVALID_ID;
    u32 indices_count = INVALID_ID;
    u32 vertex_count  = INVALID_ID;
};

#define MAX_FRAMES_IN_FLIGHT 3
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

    u32 width;
    u32 height;

    u32 mip_levels = INVALID_ID;
};

struct vulkan_texture
{
    u32          descriptor_id = INVALID_ID;
    vulkan_image image;
    VkSampler    sampler;
};

#define DEFAULT_MIP_LEVEL 1

struct vulkan_swapchain
{

    u32 width;
    u32 height;

    VkSwapchainKHR handle;
    VkExtent2D     surface_extent;
    VkFramebuffer *buffers = nullptr;

    u32          images_count = INVALID_ID;
    VkImage     *images       = nullptr;
    VkImageView *img_views    = nullptr;
    VkFormat     img_format;

    vulkan_image depth_image;

    VkSurfaceCapabilitiesKHR surface_capabilities;

    // TODO: linear alloc??
    u32                 surface_formats_count = INVALID_ID;
    VkSurfaceFormatKHR *surface_formats       = nullptr;

    u32               present_modes_count = INVALID_ID;
    VkPresentModeKHR *present_modes       = nullptr;
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

#define VULKAN_MAX_DESCRIPTOR_SET_COUNT 4096
struct vulkan_context
{
    u32 current_frame_index;

    vulkan_device vk_device;

    vulkan_swapchain vk_swapchain;

    VkRenderPass vk_renderpass;

    vulkan_pipeline vk_graphics_pipeline;

    vulkan_buffer *scene_global_uniform_buffers = nullptr;
    darray<void *> scene_global_ubo_data;
    //INFO: idk if its should be seperate
    vulkan_buffer *light_global_uniform_buffers = nullptr;
    darray<void *> light_global_ubo_data;


    darray<VkCommandBuffer> command_buffers;

    vulkan_buffer vertex_buffer;
    vulkan_buffer index_buffer;

    VkCommandPool graphics_command_pool;

    VkDescriptorSetLayout   global_descriptor_layout;
    VkDescriptorPool        global_descriptor_command_pool;
    darray<VkDescriptorSet> global_descriptor_sets;

    VkDescriptorSetLayout   material_descriptor_layout;
    VkDescriptorPool        material_descriptor_command_pool;
    darray<VkDescriptorSet> material_descriptor_sets;

    VkSemaphore *image_available_semaphores = nullptr;
    VkSemaphore *render_finished_semaphores = nullptr;
    VkFence     *in_flight_fences           = nullptr;

    VkSurfaceKHR vk_surface;

    VkDebugUtilsMessengerEXT vk_dbg_messenger;
    VkAllocationCallbacks   *vk_allocator = 0;

    VkInstance vk_instance;

    u32                  loaded_geometries_count = 0;
    vulkan_geometry_data internal_geometries[MAX_GEOMETRIES_LOADED];

    u32         enabled_layer_count = INVALID_ID;
    const char *enabled_layer_names[4];

    u32         enabled_extension_count = INVALID_ID;
    const char *enabled_extension_names[4];
};
