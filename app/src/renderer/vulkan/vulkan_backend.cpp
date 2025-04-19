#include "core/dmemory.hpp"
#include "core/dstring.hpp"
#include "core/logger.hpp"
#include "defines.hpp"

#include "vulkan_backend.hpp"
#include "vulkan_buffer.hpp"
#include "vulkan_command_buffers.hpp"
#include "vulkan_device.hpp"
#include "vulkan_framebuffer.hpp"
#include "vulkan_graphics_pipeline.hpp"
#include "vulkan_image.hpp"
#include "vulkan_platform.hpp"
#include "vulkan_renderpass.hpp"
#include "vulkan_swapchain.hpp"
#include "vulkan_sync_objects.hpp"
#include "vulkan_types.hpp"

static vulkan_context *vk_context;

VkBool32 vulkan_dbg_msg_rprt_callback(VkDebugUtilsMessageSeverityFlagBitsEXT      messageSeverity,
                                      VkDebugUtilsMessageTypeFlagsEXT             messageTypes,
                                      const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData, void *pUserData);

bool vulkan_check_validation_layer_support();
bool vulkan_create_debug_messenger(VkDebugUtilsMessengerCreateInfoEXT *dbg_messenger_create_info);
bool vulkan_create_default_texture();

bool vulkan_backend_initialize(u64 *vulkan_backend_memory_requirements, application_config *app_config, void *state)
{
    *vulkan_backend_memory_requirements = sizeof(vulkan_context);
    if (!state)
    {
        return true;
    }
    DDEBUG("Initializing Vulkan backend...");
    vk_context               = (vulkan_context *)state;
    vk_context->vk_allocator = nullptr;
    {
        darray<void *> global_ubo_mem_data;
        vk_context->global_uniform_buffers_memory_data = global_ubo_mem_data;
    }
    {
        darray<VkDescriptorSet> descriptor_sets;
        vk_context->descriptor_sets = descriptor_sets;
    }
    {
        darray<VkCommandBuffer> command_buffers;
        vk_context->command_buffers = command_buffers;
    }

    DDEBUG("Creating vulkan instance...");

    VkApplicationInfo app_info{};

    app_info.sType              = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    app_info.pNext              = nullptr;
    app_info.pApplicationName   = app_config->application_name;
    app_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    app_info.pEngineName        = app_config->application_name;
    app_info.engineVersion      = VK_MAKE_VERSION(1, 0, 0);
    app_info.apiVersion         = VK_API_VERSION_1_3;

    bool enable_validation_layers = false;
    bool validation_layer_support = false;

#ifdef DEBUG

    enable_validation_layers = true;

    // vulkan debug messenger
    validation_layer_support = vulkan_check_validation_layer_support();
    if (!validation_layer_support && enable_validation_layers)
    {
        DERROR("Validation layer support requsted, but not available.");
        return false;
    }
#endif

    vk_context->enabled_layer_count = 0;

    u32 index = 0;
    if (enable_validation_layers && validation_layer_support)
    {
        vk_context->enabled_layer_count++;
        vk_context->enabled_layer_names[index++] = "VK_LAYER_KHRONOS_validation";
    }

    // INFO: get platform specifig extensions and extensions count
    darray<const char *> vulkan_instance_extensions;
    const char          *vk_generic_surface_ext = VK_KHR_SURFACE_EXTENSION_NAME;

    vulkan_instance_extensions.push_back(vk_generic_surface_ext);
    vulkan_platform_get_required_vulkan_extensions(vulkan_instance_extensions);

#if DEBUG
    const char *vk_debug_uitls  = VK_EXT_DEBUG_UTILS_EXTENSION_NAME;
    const char *vk_debug_report = VK_EXT_DEBUG_REPORT_EXTENSION_NAME;

    vulkan_instance_extensions.push_back(vk_debug_uitls);
    vulkan_instance_extensions.push_back(vk_debug_report);
#endif

    u32                   dbg_vulkan_instance_extensions_count = 0;
    VkExtensionProperties dbg_instance_extension_properties[256];

    vkEnumerateInstanceExtensionProperties(0, &dbg_vulkan_instance_extensions_count, 0);
    vkEnumerateInstanceExtensionProperties(0, &dbg_vulkan_instance_extensions_count, dbg_instance_extension_properties);

    // check for extensions support
#ifdef DEBUG
    //{

    //    for (u32 i = 0; i < dbg_vulkan_instance_extensions_count; i++)
    //    {
    //        DDEBUG("Extension Name: %s", dbg_instance_extension_properties[i].extensionName);
    //    }
    //}
#endif

    for (u32 i = 0; i < vulkan_instance_extensions.size(); i++)
    {
        bool        found_ext = false;
        const char *str0      = vulkan_instance_extensions[i];

        for (u32 j = 0; j < dbg_vulkan_instance_extensions_count; j++)
        {
            if (string_compare(str0, dbg_instance_extension_properties[j].extensionName))
            {
                found_ext = true;
                break;
            }
        }
        if (found_ext == false)
        {
            DERROR("Extension Name: %s not found", vulkan_instance_extensions[i]);
            return false;
        }
    }

    // Instance info
    VkDebugUtilsMessengerCreateInfoEXT dbg_messenger_create_info{};

#ifdef DEBUG
    vulkan_create_debug_messenger(&dbg_messenger_create_info);
#endif

    VkInstanceCreateInfo inst_create_info{};

    inst_create_info.sType                   = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    inst_create_info.pNext                   = 0;
    inst_create_info.flags                   = 0;
    inst_create_info.enabledLayerCount       = vk_context->enabled_layer_count;
    inst_create_info.ppEnabledLayerNames     = vk_context->enabled_layer_names;
    inst_create_info.enabledExtensionCount   = (u32)vulkan_instance_extensions.size();
    inst_create_info.ppEnabledExtensionNames = (const char **)vulkan_instance_extensions.data;
    inst_create_info.pApplicationInfo        = &app_info;

    VkResult result = vkCreateInstance(&inst_create_info, vk_context->vk_allocator, &vk_context->vk_instance);
    VK_CHECK(result);

    if (enable_validation_layers && validation_layer_support)
    {
        bool result = vulkan_create_debug_messenger(&dbg_messenger_create_info);
        if (!result)
        {
            DERROR("Couldn't setup Vulkan Create debug messenger.");
            return false;
        }
    }
    if (!vulkan_platform_create_surface(vk_context))
    {
        DERROR("Vulkan surface creation failed.");
        return false;
    }

    if (!vulkan_create_logical_device(vk_context))
    {
        DERROR("Vulkan logical device creation failed.");
        return false;
    }

    if (!vulkan_create_swapchain(vk_context))
    {
        DERROR("Vulkan swapchain creation failed.");
        return false;
    }

    if (!vulkan_create_renderpass(vk_context))
    {
        DERROR("Vulkan renderpass creation failed.");
        return false;
    }
    if (!vulkan_create_graphics_pipeline(vk_context))
    {
        DERROR("Vulkan graphics pipline creation failed.");
        return false;
    }

    if (!vulkan_create_framebuffers(vk_context))
    {
        DERROR("Vulkan framebuffer creation failed.");
        return false;
    }
    if (!vulkan_create_graphics_command_pool(vk_context))
    {
        DERROR("Vulkan command pool creation failed.");
        return false;
    }
    if (!vulkan_create_vertex_buffer(vk_context))
    {
        DERROR("Vulkan Vertex buffer creation failed.");
        return false;
    }
    if (!vulkan_create_index_buffer(vk_context))
    {
        DERROR("Vulkan Vertex buffer creation failed.");
        return false;
    }
    if (!vulkan_create_global_uniform_buffers(vk_context))
    {
        DERROR("Vulkan global uniform buffer creation failed.");
        return false;
    }
    if (!vulkan_create_descriptor_command_pool_n_sets(vk_context))
    {
        DERROR("Vulkan descriptor command pool and sets creation failed.");
        return false;
    }

    vk_context->command_buffers.resize(MAX_FRAMES_IN_FLIGHT);
    if (!vulkan_allocate_command_buffers(vk_context, &vk_context->graphics_command_pool,
                                         (VkCommandBuffer *)vk_context->command_buffers.data, MAX_FRAMES_IN_FLIGHT,
                                         false))
    {
        DERROR("Vulkan command buffer creation failed.");
        return false;
    }
    if (!vulkan_create_sync_objects(vk_context))
    {
        DERROR("Vulkan sync objects creation failed.");
        return false;
    }

    // INFO:  creating default textrue
    if (!vulkan_create_default_texture())
    {
    }

    vk_context->current_frame_index = 0;

    return true;
}

bool vulkan_create_default_texture()
{
    u32 default_tex_dimensions = 256;
    u32 channel_count          = 4;
    u32 pixel_count            = default_tex_dimensions * default_tex_dimensions;

    u32 texture_size                   = pixel_count * channel_count;
    vk_context->default_texture.width  = default_tex_dimensions;
    vk_context->default_texture.height = default_tex_dimensions;

    vulkan_buffer staging_buffer{};

    darray<u32> pixels(pixel_count);

    for (u32 y = 0; y < default_tex_dimensions; y++)
    {
        for (u32 x = 0; x < default_tex_dimensions; x++)
        {
            u32 *pixel = &pixels[(y * default_tex_dimensions) + x];
            u8   red   = 123;
            u8   green = 123;
            u8   blue  = 123;
            *pixel     = (red << 16 | green << 8 | blue << 0);
        }
    }

    bool result =
        vulkan_create_buffer(vk_context, &staging_buffer, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                             VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, texture_size);
    DASSERT(result == true);

    void *data;
    vulkan_copy_data_to_buffer(vk_context, &staging_buffer, data, pixels.data, pixel_count);

    result = vulkan_create_image(vk_context, &vk_context->default_texture, default_tex_dimensions,
                                 default_tex_dimensions, VK_FORMAT_R8G8B8A8_SRGB, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                                 VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_IMAGE_TILING_OPTIMAL);
    DASSERT(result == true);

    vulkan_transition_image_layout(vk_context, &vk_context->default_texture, VK_IMAGE_LAYOUT_UNDEFINED,
                                   VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

    vulkan_copy_buffer_data_to_image(vk_context, &staging_buffer, &vk_context->default_texture);
    vulkan_transition_image_layout(vk_context, &vk_context->default_texture, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                   VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    vulkan_destroy_buffer(vk_context, &staging_buffer);

    return true;
}

bool vulkan_create_debug_messenger(VkDebugUtilsMessengerCreateInfoEXT *dbg_messenger_create_info)
{

    dbg_messenger_create_info->sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    dbg_messenger_create_info->pNext = 0;
    dbg_messenger_create_info->flags = 0;
    dbg_messenger_create_info->messageSeverity =
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    dbg_messenger_create_info->messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                                             VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                                             VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    dbg_messenger_create_info->pfnUserCallback = vulkan_dbg_msg_rprt_callback;
    dbg_messenger_create_info->pUserData       = nullptr;

    if (!vk_context->vk_instance)
    {
        DDEBUG("Populating vulkan_debug_utils_messenger struct");
        return true;
    }

    PFN_vkCreateDebugUtilsMessengerEXT func_create_dbg_utils_messenger_ext =
        (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(vk_context->vk_instance,
                                                                  "vkCreateDebugUtilsMessengerEXT");

    if (func_create_dbg_utils_messenger_ext == nullptr)
    {
        DERROR("Vulkan Create debug messenger ext called but fucn ptr to vkCreateDebugUtilsMessengerEXT is nullptr");
        return false;
    }
    if (vk_context->vk_instance)
    {
        VK_CHECK(func_create_dbg_utils_messenger_ext(vk_context->vk_instance, dbg_messenger_create_info,
                                                     vk_context->vk_allocator, &vk_context->vk_dbg_messenger));
    }

    return true;
}

void vulkan_backend_shutdown()
{
    DDEBUG("Shutting down vulkan...");

    VkDevice              &device    = vk_context->vk_device.logical;
    VkAllocationCallbacks *allocator = vk_context->vk_allocator;

    vkDeviceWaitIdle(device);
    for (u32 i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        vkDestroyFence(device, vk_context->in_flight_fences[i], allocator);
        vkDestroySemaphore(device, vk_context->render_finished_semaphores[i], allocator);
        vkDestroySemaphore(device, vk_context->image_available_semaphores[i], allocator);
    }
    dfree(vk_context->image_available_semaphores, sizeof(VkSemaphore) * MAX_FRAMES_IN_FLIGHT, MEM_TAG_RENDERER);
    dfree(vk_context->render_finished_semaphores, sizeof(VkSemaphore) * MAX_FRAMES_IN_FLIGHT, MEM_TAG_RENDERER);
    dfree(vk_context->in_flight_fences, sizeof(VkFence) * MAX_FRAMES_IN_FLIGHT, MEM_TAG_RENDERER);

    vulkan_destroy_image(vk_context, &vk_context->default_texture);

    vulkan_destroy_buffer(vk_context, &vk_context->vertex_buffer);
    vulkan_destroy_buffer(vk_context, &vk_context->index_buffer);

    vkDestroyDescriptorPool(device, vk_context->descriptor_command_pool, allocator);
    vkDestroyDescriptorSetLayout(device, vk_context->global_uniform_descriptor_layout, allocator);

    vulkan_free_command_buffers(vk_context, &vk_context->graphics_command_pool,
                                (VkCommandBuffer *)vk_context->command_buffers.data, MAX_FRAMES_IN_FLIGHT);
    vkDestroyCommandPool(device, vk_context->graphics_command_pool, allocator);

    for (u32 i = 0; i < vk_context->vk_swapchain.images_count; i++)
    {
        vkDestroyFramebuffer(device, vk_context->vk_swapchain.buffers[i].handle, allocator);
    }
    dfree(vk_context->vk_swapchain.buffers, sizeof(VkFramebuffer) * vk_context->vk_swapchain.images_count,
          MEM_TAG_RENDERER);

    for (u32 i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        vulkan_destroy_buffer(vk_context, &vk_context->global_uniform_buffers[i]);
    }
    dfree(vk_context->global_uniform_buffers, sizeof(vulkan_buffer) * MAX_FRAMES_IN_FLIGHT, MEM_TAG_RENDERER);

    vkDestroyPipeline(device, vk_context->vk_graphics_pipeline.handle, allocator);

    vkDestroyRenderPass(device, vk_context->vk_renderpass, allocator);

    vkDestroyPipelineLayout(device, vk_context->vk_graphics_pipeline.layout, allocator);

    for (u32 i = 0; i < vk_context->vk_swapchain.images_count; i++)
    {
        vkDestroyImageView(device, vk_context->vk_swapchain.img_views[i], allocator);
    }
    vulkan_destroy_image(vk_context, &vk_context->vk_swapchain.depth_image);

    dfree(vk_context->vk_swapchain.images, sizeof(VkImage) * vk_context->vk_swapchain.images_count, MEM_TAG_RENDERER);
    dfree(vk_context->vk_swapchain.img_views, sizeof(VkImageView) * vk_context->vk_swapchain.images_count,
          MEM_TAG_RENDERER);

    dfree(vk_context->vk_swapchain.surface_formats, sizeof(VkSurfaceFormatKHR) * vk_context->vk_swapchain.images_count,
          MEM_TAG_RENDERER);
    dfree(vk_context->vk_swapchain.present_modes, sizeof(VkPresentModeKHR) * vk_context->vk_swapchain.images_count,
          MEM_TAG_RENDERER);

    vkDestroySwapchainKHR(device, vk_context->vk_swapchain.handle, allocator);

    vkDestroySurfaceKHR(vk_context->vk_instance, vk_context->vk_surface, allocator);

    // destroy logical device
    if (vk_context->vk_device.physical_properties)
    {
        dfree(vk_context->vk_device.physical_properties, sizeof(VkPhysicalDeviceProperties), MEM_TAG_RENDERER);
    }
    if (vk_context->vk_device.physical_features)
    {
        dfree(vk_context->vk_device.physical_features, sizeof(VkPhysicalDeviceFeatures), MEM_TAG_RENDERER);
    }
    vkDestroyDevice(device, allocator);

#if DEBUG
    PFN_vkDestroyDebugUtilsMessengerEXT func_destroy_dbg_utils_messenger_ext =
        (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(vk_context->vk_instance,
                                                                   "vkDestroyDebugUtilsMessengerEXT");

    if (func_destroy_dbg_utils_messenger_ext == nullptr)
    {
        DERROR("Vulkan destroy debug messenger ext called but fucn ptr to vkDestroyDebugUtilsMessengerEXT is nullptr");
    }
    else
    {
        func_destroy_dbg_utils_messenger_ext(vk_context->vk_instance, vk_context->vk_dbg_messenger, allocator);
    }
#endif
    vkDestroyInstance(vk_context->vk_instance, allocator);
}

bool vulkan_check_validation_layer_support()
{
    u32 inst_layer_properties_count = 0;
    vkEnumerateInstanceLayerProperties(&inst_layer_properties_count, 0);

    darray<VkLayerProperties> inst_layer_properties(inst_layer_properties_count);
    vkEnumerateInstanceLayerProperties(&inst_layer_properties_count, (VkLayerProperties *)inst_layer_properties.data);

    const char *validation_layer_name = "VK_LAYER_KHRONOS_validation";

    bool validation_layer_found = false;

#ifdef DEBUG
    // for (u32 i = 0; i < inst_layer_properties_count; i++)
    //{
    //     DDEBUG("Layer Name: %s | Desc: %s", inst_layer_properties[i].layerName,
    //     inst_layer_properties[i].description);
    // }
#endif

    for (u32 i = 0; i < inst_layer_properties_count; i++)
    {
        if (string_compare(inst_layer_properties[i].layerName, validation_layer_name))
        {
            validation_layer_found = true;
            break;
        }
    }
    return validation_layer_found;
}

VkBool32 vulkan_dbg_msg_rprt_callback(VkDebugUtilsMessageSeverityFlagBitsEXT      messageSeverity,
                                      VkDebugUtilsMessageTypeFlagsEXT             messageTypes,
                                      const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData, void *pUserData)
{
    switch (messageSeverity)
    {
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT: {
        DTRACE("%s", pCallbackData->pMessage);
    }
    break;
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT: {
        DINFO("%s", pCallbackData->pMessage);
    }
    break;
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT: {
        DWARN("%s", pCallbackData->pMessage);
    }
    break;
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT: {
        DERROR("%s", pCallbackData->pMessage);
    }
    break;
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_FLAG_BITS_MAX_ENUM_EXT: {
    }
    break;
    }

    return VK_FALSE;
}

void vulkan_update_global_uniform_buffer(uniform_buffer_object *global_ubo, u32 current_frame_index)
{
    dcopy_memory(vk_context->global_uniform_buffers_memory_data[current_frame_index], global_ubo,
                 sizeof(uniform_buffer_object));
}

bool vulkan_draw_frame(render_data *render_data)
{
    u32      current_frame = vk_context->current_frame_index;
    VkResult result;

    vkWaitForFences(vk_context->vk_device.logical, 1, &vk_context->in_flight_fences[current_frame], VK_TRUE,
                    INVALID_ID_64);
    vkResetFences(vk_context->vk_device.logical, 1, &vk_context->in_flight_fences[current_frame]);

    // vertex
    void *vertex_data;
    u32   buffer_size = (u32)render_data->vertices.size() * sizeof(vertex);

    vulkan_buffer vertex_staging_buffer;
    vulkan_create_buffer(vk_context, &vertex_staging_buffer, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                         VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, buffer_size);

    vulkan_copy_data_to_buffer(vk_context, &vertex_staging_buffer, vertex_data, render_data->vertices.data,
                               buffer_size);
    vulkan_copy_buffer(vk_context, &vk_context->vertex_buffer, &vertex_staging_buffer, buffer_size);

    vulkan_destroy_buffer(vk_context, &vertex_staging_buffer);

    // index
    void *index_data;
    buffer_size = (u32)render_data->indices.size() * sizeof(u32);

    vulkan_buffer index_staging_buffer;
    vulkan_create_buffer(vk_context, &index_staging_buffer, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                         VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, buffer_size);

    vulkan_copy_data_to_buffer(vk_context, &index_staging_buffer, index_data, render_data->indices.data, buffer_size);
    vulkan_copy_buffer(vk_context, &vk_context->index_buffer, &index_staging_buffer, buffer_size);

    vulkan_destroy_buffer(vk_context, &index_staging_buffer);
    //

    vulkan_update_global_uniform_buffer(&render_data->global_ubo, current_frame);

    u32 image_index = INVALID_ID;
    result = vkAcquireNextImageKHR(vk_context->vk_device.logical, vk_context->vk_swapchain.handle, INVALID_ID_64,
                                   vk_context->image_available_semaphores[current_frame], VK_NULL_HANDLE, &image_index);

    if (result & VK_ERROR_OUT_OF_DATE_KHR)
    {
        vulkan_recreate_swapchain(vk_context);
        DDEBUG("VK_ERROR_OUT_OF_DATE_KHR");
        return false;
    }
    VK_CHECK(result);

    VkCommandBuffer &curr_command_buffer = vk_context->command_buffers[current_frame];

    vkResetCommandBuffer(curr_command_buffer, 0);
    vulkan_begin_command_buffer_single_use(vk_context, curr_command_buffer);
    vulkan_begin_frame_renderpass(vk_context, curr_command_buffer, render_data, image_index);
    vulkan_end_command_buffer_single_use(vk_context, curr_command_buffer);

    VkSemaphore          wait_semaphores[]   = {vk_context->image_available_semaphores[current_frame]};
    VkSemaphore          signal_semaphores[] = {vk_context->render_finished_semaphores[current_frame]};
    VkPipelineStageFlags wait_stages[]       = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};

    VkSubmitInfo submit_info{};
    submit_info.sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.waitSemaphoreCount   = 1;
    submit_info.pWaitSemaphores      = wait_semaphores;
    submit_info.pWaitDstStageMask    = wait_stages;
    submit_info.commandBufferCount   = 1;
    submit_info.pCommandBuffers      = &curr_command_buffer;
    submit_info.signalSemaphoreCount = 1;
    submit_info.pSignalSemaphores    = signal_semaphores;

    result = vkQueueSubmit(vk_context->vk_device.graphics_queue, 1, &submit_info,
                           vk_context->in_flight_fences[current_frame]);
    VK_CHECK(result);
    vkQueueWaitIdle(vk_context->vk_device.graphics_queue);
    VkSwapchainKHR swapchains[] = {vk_context->vk_swapchain.handle};

    VkPresentInfoKHR present_info{};
    present_info.sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    present_info.waitSemaphoreCount = 1;
    present_info.pWaitSemaphores    = signal_semaphores;
    present_info.swapchainCount     = 1;
    present_info.pSwapchains        = &vk_context->vk_swapchain.handle;
    present_info.pImageIndices      = &image_index;

    result = vkQueuePresentKHR(vk_context->vk_device.present_queue, &present_info);
    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
    {
        DDEBUG("VK_ERROR_OUT_OF_DATE_KHR or VK_SUBOTIMAL_KHR");
        vulkan_recreate_swapchain(vk_context);
    }
    VK_CHECK(result);

    vk_context->current_frame_index++;
    vk_context->current_frame_index %= MAX_FRAMES_IN_FLIGHT;

    return true;
}
bool vulkan_backend_resize()
{
    bool result = vulkan_recreate_swapchain(vk_context);
    return result;
}
