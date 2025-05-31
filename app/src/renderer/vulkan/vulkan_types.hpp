#pragma once
#include "containers/darray.hpp"
#include "containers/dhashtable.hpp"
#include "core/dasserts.hpp"
#include "defines.hpp"
#include "resources/resource_types.hpp"
#include <vulkan/vulkan.h>

const char *vk_result_to_string(VkResult result);

#define VK_CHECK(expr)                                                                                                 \
    {                                                                                                                  \
        if (expr != VK_SUCCESS)                                                                                        \
        {                                                                                                              \
            DFATAL("%s", vk_result_to_string(expr));                                                                   \
            debugBreak();                                                                                              \
        }                                                                                                              \
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

enum renderpass_types
{
    WORLD_RENDERPASS,
};

#define VULKAN_MAX_DESCRIPTOR_SET_COUNT 4096

struct vulkan_shader
{

    darray<char> vertex_shader_code;
    darray<char> fragment_shader_code;

    u32 per_frame_ubo_size;
    //FIXME: should figure out how to do this using dynamic buffers
    darray<void*> scene_ubo;
    darray<void*> light_ubo;
    darray<vulkan_buffer> per_frame_scene_uniform_buffers;
    darray<vulkan_buffer> per_frame_light_uniform_buffers;
    //
    //darray<vulkan_buffer> per_frame_uniform_buffers;
    //darray<void *> per_frame_buffer_data;

    u32 per_group_ubo_size;
    vulkan_buffer per_group_uniform_buffer;
    darray<void *> per_group_buffer_data;


    // INFO: for now first will always be vertex and second will always be fragment
    shader_stage stages;

    VkVertexInputBindingDescription           attribute_description;
    darray<VkVertexInputAttributeDescription> input_attribute_descriptions;

    VkDescriptorSetLayout                per_frame_descriptor_layout;
    darray<VkDescriptorSetLayoutBinding> per_frame_descriptor_sets_layout_bindings;
    VkDescriptorPool                     per_frame_descriptor_command_pool;
    darray<VkDescriptorSet>              per_frame_descriptor_sets;

    VkDescriptorSetLayout                per_group_descriptor_layout;
    darray<VkDescriptorSetLayoutBinding> per_group_descriptor_sets_layout_bindings;
    VkDescriptorPool                     per_group_descriptor_command_pool;
    darray<VkDescriptorSet>              per_group_descriptor_sets;

    renderpass_types type;
    vulkan_pipeline  pipeline;
};

struct vulkan_context
{
    u32 current_frame_index;

    vulkan_device vk_device;

    vulkan_swapchain vk_swapchain;

    VkRenderPass vk_renderpass;

    darray<VkCommandBuffer> command_buffers;

    vulkan_buffer vertex_buffer;
    vulkan_buffer index_buffer;

    u64 default_shader_id = INVALID_ID_64;
    shader *default_shader;

    VkCommandPool graphics_command_pool;

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
