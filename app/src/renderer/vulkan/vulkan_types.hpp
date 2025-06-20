#pragma once
#include "containers/darray.hpp"
#include "core/dasserts.hpp"
#include "defines.hpp"
#include "resources/resource_types.hpp"
#include <vulkan/vulkan.h>

// tracy integration
#include "vendor/tracy/TracyVulkan.hpp"
//

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

struct push_constant // aka push constants
{
    mat4 data;  // 64 bytes
    mat4 data2; // 64 bytes
};
;

union vk_push_constant
{
    f32 hundred_and_twenty_eight[32];

    struct material_pc
    {
        mat4 model;
        vec4 color;
    }material_pc;

    struct ui_pc
    {

    }ui_pc;
};

#define MAX_FRAMES_IN_FLIGHT 3
struct vulkan_device
{

    VkDevice         logical;
    VkPhysicalDevice physical;

    VkQueue graphics_queue;
    VkQueue present_queue;
    VkQueue transfer_queue;

    VkPhysicalDeviceMemoryProperties memory_properties;

    VkPhysicalDeviceProperties *physical_properties;
    VkPhysicalDeviceFeatures   *physical_features;

    u32                      queue_family_count;
    VkQueueFamilyProperties *queue_family_properties;

    // INFO: might need the selected graphics queue's QueueFamilyProperties.
    darray<u32> enabled_queue_family_indicies;

    u32 graphics_family_index = INVALID_ID;
    u32 present_family_index  = INVALID_ID;
    u32 transfer_family_index = INVALID_ID;
};

struct vulkan_image
{
    // INFO: this is needed for cube maps. set to VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT if its a cubemap aka skybox. if
    // not set it to 0
    VkImageCreateFlags img_create_flags = 0;
    VkImage            handle;
    VkImageViewType    view_type;
    VkImageView        view;
    VkFormat           format;
    VkDeviceMemory     memory;

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
    VkFramebuffer *world_framebuffers = nullptr;
    VkFramebuffer *ui_framebuffers    = nullptr;

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

struct vulkan_shader
{

    u32 min_ubo_alignment;

    darray<char> vertex_shader_code;
    darray<char> fragment_shader_code;

    u32           per_frame_stride;
    vulkan_buffer per_frame_uniform_buffer;
    void         *per_frame_mapped_data;
    darray<u32>   per_frame_uniforms_size;

    u32           per_group_ubo_size;
    vulkan_buffer per_group_uniform_buffer;

    // INFO: for now first will always be vertex and second will always be fragment
    darray<shader_stage> stages;

    VkVertexInputBindingDescription           attribute_description;
    darray<VkVertexInputAttributeDescription> input_attribute_descriptions;

    VkDescriptorSetLayout                per_frame_descriptor_layout;
    darray<VkDescriptorSetLayoutBinding> per_frame_descriptor_sets_layout_bindings;
    VkDescriptorPool                     per_frame_descriptor_pool;
    VkDescriptorSet                      per_frame_descriptor_set;

    VkDescriptorSetLayout                per_group_descriptor_layout;
    darray<VkDescriptorSetLayoutBinding> per_group_descriptor_sets_layout_bindings;
    VkDescriptorPool                     per_group_descriptor_pool;
    u32                                  total_descriptors_allocated = INVALID_ID;
    darray<VkDescriptorSet>              per_group_descriptor_sets;

    renderpass_types              renderpass_type;
    shader_pipeline_configuration pipeline_configuration;
    vulkan_pipeline               pipeline;
};

struct vulkan_renderpass
{
    VkRenderPass  handle;
    vulkan_buffer vertex_buffer;
    vulkan_buffer index_buffer;
};

struct vulkan_context
{
    u64 frame_counter;
    u32 current_frame_index;

    vulkan_device vk_device;

    vulkan_swapchain vk_swapchain;

    vulkan_renderpass world_renderpass;
    vulkan_renderpass ui_renderpass;

    darray<VkCommandBuffer> command_buffers;

    u64 default_material_shader_id = INVALID_ID_64;
    u64 default_skybox_shader_id   = INVALID_ID_64;
    u64 default_grid_shader_id     = INVALID_ID_64;
    u64 default_ui_shader_id       = INVALID_ID_64;

    VkCommandPool graphics_command_pool;
    VkCommandPool transfer_command_pool;

    VkSemaphore *image_available_semaphores = nullptr;
    VkSemaphore *render_finished_semaphores = nullptr;
    VkFence     *in_flight_fences           = nullptr;

    VkSurfaceKHR vk_surface;

    VkDebugUtilsMessengerEXT vk_dbg_messenger;
    VkAllocationCallbacks   *vk_allocator = 0;

    VkInstance vk_instance;

    u32                  world_geometries_count = 0;
    vulkan_geometry_data world_internal_geometries[MAX_GEOMETRIES_LOADED];

    u32                  ui_geometries_count = 0;
    vulkan_geometry_data ui_internal_geometries[MAX_GEOMETRIES_LOADED];

    u32         enabled_layer_count = INVALID_ID;
    const char *enabled_layer_names[4];

    u32         enabled_extension_count = INVALID_ID;
    const char *enabled_extension_names[4];

    bool swapchain_recreated[MAX_FRAMES_IN_FLIGHT];

    arena *arena = nullptr;
};
