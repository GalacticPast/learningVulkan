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

    if (!vulkan_create_command_pool(vk_context))
    {
        DERROR("Vulkan command pool creation failed.");
        return false;
    }

    if (!vulkan_create_vertex_buffer(vk_context))
    {
        DERROR("Vulkan Vertex buffer creation failed.");
        return false;
    }

    if (!vulkan_create_command_buffer(vk_context, &vk_context->graphics_command_pool))
    {
        DERROR("Vulkan command buffer creation failed.");
        return false;
    }

    if (!vulkan_create_sync_objects(vk_context))
    {
        DERROR("Vulkan sync objects creation failed.");
        return false;
    }

    vk_context->current_frame_index = 0;

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

    vulkan_destroy_buffer(vk_context, &vk_context->vertex_buffer);

    vkDestroyCommandPool(device, vk_context->graphics_command_pool, allocator);

    dfree(vk_context->command_buffers, sizeof(VkCommandBuffer) * MAX_FRAMES_IN_FLIGHT, MEM_TAG_RENDERER);

    for (u32 i = 0; i < vk_context->vk_swapchain.images_count; i++)
    {
        vkDestroyFramebuffer(device, vk_context->vk_swapchain.buffers[i].handle, allocator);
    }
    dfree(vk_context->vk_swapchain.buffers, sizeof(VkFramebuffer) * vk_context->vk_swapchain.images_count,
          MEM_TAG_RENDERER);

    vkDestroyPipeline(device, vk_context->vk_graphics_pipeline.handle, allocator);

    vkDestroyRenderPass(device, vk_context->vk_renderpass, allocator);

    vkDestroyPipelineLayout(device, vk_context->vk_graphics_pipeline.layout, allocator);

    for (u32 i = 0; i < vk_context->vk_swapchain.images_count; i++)
    {
        vkDestroyImageView(device, vk_context->vk_swapchain.vk_images.views[i], allocator);
    }

    dfree(vk_context->vk_swapchain.vk_images.handles, sizeof(VkImage) * vk_context->vk_swapchain.images_count,
          MEM_TAG_RENDERER);
    dfree(vk_context->vk_swapchain.vk_images.views, sizeof(VkImageView) * vk_context->vk_swapchain.images_count,
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

    return VK_TRUE;
}

bool vulkan_draw_frame(render_data *render_data)
{
    u32 current_frame = vk_context->current_frame_index;
    vkWaitForFences(vk_context->vk_device.logical, 1, &vk_context->in_flight_fences[current_frame], VK_TRUE,
                    INVALID_ID_64);
    vkResetFences(vk_context->vk_device.logical, 1, &vk_context->in_flight_fences[current_frame]);

    void *data;
    u32   buffer_size = (u32)render_data->vertices->capacity;

    vulkan_buffer staging_buffer;
    vulkan_create_buffer(vk_context, &staging_buffer, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                         VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, buffer_size);

    VkResult result = vkMapMemory(vk_context->vk_device.logical, staging_buffer.memory, 0, buffer_size, 0, &data);
    VK_CHECK(result);
    dcopy_memory(data, render_data->vertices->data, buffer_size);
    vkUnmapMemory(vk_context->vk_device.logical, staging_buffer.memory);
    vulkan_copy_buffer(vk_context, &vk_context->vertex_buffer, &staging_buffer, buffer_size);

    vulkan_destroy_buffer(vk_context, &staging_buffer);

    u32 image_index = INVALID_ID;
    result = vkAcquireNextImageKHR(vk_context->vk_device.logical, vk_context->vk_swapchain.handle, INVALID_ID_64,
                                   vk_context->image_available_semaphores[current_frame], VK_NULL_HANDLE, &image_index);

    if (result == VK_ERROR_OUT_OF_DATE_KHR)
    {
        vulkan_recreate_swapchain(vk_context);
        DDEBUG("VK_ERROR_OUT_OF_DATE_KHR");
        return false;
    }
    VK_CHECK(result);

    vkResetCommandBuffer(vk_context->command_buffers[current_frame], 0);

    vulkan_record_command_buffer_and_use(vk_context, vk_context->command_buffers[current_frame], render_data,
                                         image_index);

    VkSemaphore          wait_semaphores[]   = {vk_context->image_available_semaphores[current_frame]};
    VkSemaphore          signal_semaphores[] = {vk_context->render_finished_semaphores[current_frame]};
    VkPipelineStageFlags wait_stages[]       = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};

    VkSubmitInfo submit_info{};
    submit_info.sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.waitSemaphoreCount   = 1;
    submit_info.pWaitSemaphores      = wait_semaphores;
    submit_info.pWaitDstStageMask    = wait_stages;
    submit_info.commandBufferCount   = 1;
    submit_info.pCommandBuffers      = &vk_context->command_buffers[current_frame];
    submit_info.signalSemaphoreCount = 1;
    submit_info.pSignalSemaphores    = signal_semaphores;

    result = vkQueueSubmit(vk_context->vk_graphics_queue, 1, &submit_info, vk_context->in_flight_fences[current_frame]);
    VK_CHECK(result);

    VkSwapchainKHR swapchains[] = {vk_context->vk_swapchain.handle};

    VkPresentInfoKHR present_info{};
    present_info.sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    present_info.waitSemaphoreCount = 1;
    present_info.pWaitSemaphores    = signal_semaphores;
    present_info.swapchainCount     = 1;
    present_info.pSwapchains        = &vk_context->vk_swapchain.handle;
    present_info.pImageIndices      = &image_index;

    result = vkQueuePresentKHR(vk_context->vk_present_queue, &present_info);
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
