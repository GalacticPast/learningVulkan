#include "core/dmemory.hpp"
#include "core/dstring.hpp"
#include "core/logger.hpp"
#include "defines.hpp"

#include <vector>

#include "renderer/vulkan/vulkan_renderpass.hpp"
#include "vulkan_backend.hpp"
#include "vulkan_command_buffers.hpp"
#include "vulkan_device.hpp"
#include "vulkan_framebuffer.hpp"
#include "vulkan_graphics_pipeline.hpp"
#include "vulkan_platform.hpp"
#include "vulkan_swapchain.hpp"
#include "vulkan_sync_objects.hpp"
#include "vulkan_types.hpp"

static vulkan_context *vulkan_context_ptr;

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
    vulkan_context_ptr               = (vulkan_context *)state;
    vulkan_context_ptr->vk_allocator = nullptr;

    DDEBUG("Creating vulkan instance...");

    VkApplicationInfo app_info{};

    app_info.sType                = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    app_info.pNext                = nullptr;
    app_info.pApplicationName     = app_config->application_name;
    app_info.applicationVersion   = VK_MAKE_VERSION(1, 0, 0);
    app_info.pEngineName          = app_config->application_name;
    app_info.engineVersion        = VK_MAKE_VERSION(1, 0, 0);
    app_info.apiVersion           = VK_API_VERSION_1_3;

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

    vulkan_context_ptr->enabled_layer_count = 0;

    u32 index                               = 0;
    if (enable_validation_layers && validation_layer_support)
    {
        vulkan_context_ptr->enabled_layer_count++;
        vulkan_context_ptr->enabled_layer_names[index++] = "VK_LAYER_KHRONOS_validation";
    }

    // INFO: get platform specifig extensions and extensions count
    std::vector<const char *> vulkan_instance_extensions;
    vulkan_instance_extensions.push_back(VK_KHR_SURFACE_EXTENSION_NAME);
    vulkan_platform_get_required_vulkan_extensions(vulkan_instance_extensions);

#if DEBUG
    vulkan_instance_extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    vulkan_instance_extensions.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
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
        bool found_ext = false;
        for (u32 j = 0; j < dbg_vulkan_instance_extensions_count; j++)
        {
            const char *str0 = vulkan_instance_extensions[i];
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
    inst_create_info.enabledLayerCount       = vulkan_context_ptr->enabled_layer_count;
    inst_create_info.ppEnabledLayerNames     = vulkan_context_ptr->enabled_layer_names;
    inst_create_info.enabledExtensionCount   = (u32)vulkan_instance_extensions.size();
    inst_create_info.ppEnabledExtensionNames = vulkan_instance_extensions.data();
    inst_create_info.pApplicationInfo        = &app_info;

    VkResult result =
        vkCreateInstance(&inst_create_info, vulkan_context_ptr->vk_allocator, &vulkan_context_ptr->vk_instance);
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

    if (!vulkan_platform_create_surface(vulkan_context_ptr))
    {
        DERROR("Vulkan surface creation failed.");
        return false;
    }

    if (!vulkan_create_logical_device(vulkan_context_ptr))
    {
        DERROR("Vulkan logical device creation failed.");
        return false;
    }

    if (!vulkan_create_swapchain(vulkan_context_ptr))
    {
        DERROR("Vulkan swapchain creation failed.");
        return false;
    }

    if (!vulkan_create_renderpass(vulkan_context_ptr))
    {
        DERROR("Vulkan renderpass creation failed.");
        return false;
    }

    if (!vulkan_create_graphics_pipeline(vulkan_context_ptr))
    {
        DERROR("Vulkan graphics pipline creation failed.");
        return false;
    }

    if (!vulkan_create_framebuffers(vulkan_context_ptr))
    {
        DERROR("Vulkan framebuffer creation failed.");
        return false;
    }

    if (!vulkan_create_command_pool(vulkan_context_ptr))
    {
        DERROR("Vulkan command pool creation failed.");
        return false;
    }

    if (!vulkan_create_command_buffer(vulkan_context_ptr, &vulkan_context_ptr->graphics_command_pool))
    {
        DERROR("Vulkan command buffer creation failed.");
        return false;
    }

    if (!vulkan_create_sync_objects(vulkan_context_ptr))
    {
        DERROR("Vulkan sync objects creation failed.");
        return false;
    }

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

    if (!vulkan_context_ptr->vk_instance)
    {
        DDEBUG("Populating vulkan_debug_utils_messenger struct");
        return true;
    }

    PFN_vkCreateDebugUtilsMessengerEXT func_create_dbg_utils_messenger_ext =
        (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(vulkan_context_ptr->vk_instance,
                                                                  "vkCreateDebugUtilsMessengerEXT");

    if (func_create_dbg_utils_messenger_ext == nullptr)
    {
        DERROR("Vulkan Create debug messenger ext called but fucn ptr to vkCreateDebugUtilsMessengerEXT is nullptr");
        return false;
    }
    if (vulkan_context_ptr->vk_instance)
    {
        VK_CHECK(func_create_dbg_utils_messenger_ext(vulkan_context_ptr->vk_instance, dbg_messenger_create_info,
                                                     vulkan_context_ptr->vk_allocator,
                                                     &vulkan_context_ptr->vk_dbg_messenger));
    }

    return true;
}

void vulkan_backend_shutdown()
{
    DDEBUG("Shutting down vulkan...");
    vkDeviceWaitIdle(vulkan_context_ptr->vk_device.logical);

    vkDestroyFence(vulkan_context_ptr->vk_device.logical, vulkan_context_ptr->in_flight_fence,
                   vulkan_context_ptr->vk_allocator);
    vkDestroySemaphore(vulkan_context_ptr->vk_device.logical, vulkan_context_ptr->render_finished_semaphore,
                       vulkan_context_ptr->vk_allocator);
    vkDestroySemaphore(vulkan_context_ptr->vk_device.logical, vulkan_context_ptr->image_available_semaphore,
                       vulkan_context_ptr->vk_allocator);

    vkDestroyCommandPool(vulkan_context_ptr->vk_device.logical, vulkan_context_ptr->graphics_command_pool,
                         vulkan_context_ptr->vk_allocator);

    for (u32 i = 0; i < vulkan_context_ptr->vk_swapchain.images_count; i++)
    {
        vkDestroyFramebuffer(vulkan_context_ptr->vk_device.logical, vulkan_context_ptr->vk_swapchain.buffers[i].handle,
                             vulkan_context_ptr->vk_allocator);
    }

    vkDestroyPipeline(vulkan_context_ptr->vk_device.logical, vulkan_context_ptr->vk_graphics_pipeline.handle,
                      vulkan_context_ptr->vk_allocator);

    vkDestroyRenderPass(vulkan_context_ptr->vk_device.logical, vulkan_context_ptr->vk_renderpass,
                        vulkan_context_ptr->vk_allocator);

    vkDestroyPipelineLayout(vulkan_context_ptr->vk_device.logical, vulkan_context_ptr->vk_graphics_pipeline.layout,
                            vulkan_context_ptr->vk_allocator);

    for (u32 i = 0; i < vulkan_context_ptr->vk_swapchain.images_count; i++)
    {
        vkDestroyImageView(vulkan_context_ptr->vk_device.logical, vulkan_context_ptr->vk_swapchain.vk_images.views[i],
                           vulkan_context_ptr->vk_allocator);
    }

    dfree(vulkan_context_ptr->vk_swapchain.vk_images.handles,
          sizeof(VkImage) * vulkan_context_ptr->vk_swapchain.images_count, MEM_TAG_RENDERER);

    vkDestroySwapchainKHR(vulkan_context_ptr->vk_device.logical, vulkan_context_ptr->vk_swapchain.handle,
                          vulkan_context_ptr->vk_allocator);

    vkDestroySurfaceKHR(vulkan_context_ptr->vk_instance, vulkan_context_ptr->vk_surface,
                        vulkan_context_ptr->vk_allocator);

    // destroy logical device
    if (vulkan_context_ptr->vk_device.physical_properties)
    {
        dfree(vulkan_context_ptr->vk_device.physical_properties, sizeof(VkPhysicalDeviceProperties), MEM_TAG_RENDERER);
    }
    if (vulkan_context_ptr->vk_device.physical_features)
    {
        dfree(vulkan_context_ptr->vk_device.physical_features, sizeof(VkPhysicalDeviceFeatures), MEM_TAG_RENDERER);
    }
    vkDestroyDevice(vulkan_context_ptr->vk_device.logical, vulkan_context_ptr->vk_allocator);

#if DEBUG
    PFN_vkDestroyDebugUtilsMessengerEXT func_destroy_dbg_utils_messenger_ext =
        (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(vulkan_context_ptr->vk_instance,
                                                                   "vkDestroyDebugUtilsMessengerEXT");

    if (func_destroy_dbg_utils_messenger_ext == nullptr)
    {
        DERROR("Vulkan destroy debug messenger ext called but fucn ptr to vkDestroyDebugUtilsMessengerEXT is nullptr");
    }
    else
    {
        func_destroy_dbg_utils_messenger_ext(vulkan_context_ptr->vk_instance, vulkan_context_ptr->vk_dbg_messenger,
                                             vulkan_context_ptr->vk_allocator);
    }
#endif
    vkDestroyInstance(vulkan_context_ptr->vk_instance, vulkan_context_ptr->vk_allocator);
}

bool vulkan_check_validation_layer_support()
{
    u32 inst_layer_properties_count = 0;
    vkEnumerateInstanceLayerProperties(&inst_layer_properties_count, 0);

    std::vector<VkLayerProperties> inst_layer_properties(inst_layer_properties_count);
    vkEnumerateInstanceLayerProperties(&inst_layer_properties_count, inst_layer_properties.data());

    const char *validation_layer_name = "VK_LAYER_KHRONOS_validation";

    bool validation_layer_found       = false;

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

bool vulkan_draw_frame()
{
    vkWaitForFences(vulkan_context_ptr->vk_device.logical, 1, &vulkan_context_ptr->in_flight_fence, VK_TRUE,
                    INVALID_ID_64);
    vkResetFences(vulkan_context_ptr->vk_device.logical, 1, &vulkan_context_ptr->in_flight_fence);

    u32      image_index = INVALID_ID;
    VkResult result      = vkAcquireNextImageKHR(
        vulkan_context_ptr->vk_device.logical, vulkan_context_ptr->vk_swapchain.handle, INVALID_ID_64,
        vulkan_context_ptr->image_available_semaphore, VK_NULL_HANDLE, &image_index);
    VK_CHECK(result);

    vkResetCommandBuffer(vulkan_context_ptr->command_buffer, 0);

    vulkan_record_command_buffer_and_use(vulkan_context_ptr, vulkan_context_ptr->command_buffer, image_index);

    VkSemaphore          wait_semaphores[]   = {vulkan_context_ptr->image_available_semaphore};
    VkSemaphore          signal_semaphores[] = {vulkan_context_ptr->render_finished_semaphore};
    VkPipelineStageFlags wait_stages[]       = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};

    VkSubmitInfo submit_info{};
    submit_info.sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.waitSemaphoreCount   = 1;
    submit_info.pWaitSemaphores      = wait_semaphores;
    submit_info.pWaitDstStageMask    = wait_stages;
    submit_info.commandBufferCount   = 1;
    submit_info.pCommandBuffers      = &vulkan_context_ptr->command_buffer;
    submit_info.signalSemaphoreCount = 1;
    submit_info.pSignalSemaphores    = signal_semaphores;

    result = vkQueueSubmit(vulkan_context_ptr->vk_graphics_queue, 1, &submit_info, vulkan_context_ptr->in_flight_fence);
    VK_CHECK(result);

    VkSwapchainKHR swapchains[] = {vulkan_context_ptr->vk_swapchain.handle};

    VkPresentInfoKHR present_info{};
    present_info.sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    present_info.waitSemaphoreCount = 1;
    present_info.pWaitSemaphores    = signal_semaphores;
    present_info.swapchainCount     = 1;
    present_info.pSwapchains        = &vulkan_context_ptr->vk_swapchain.handle;
    present_info.pImageIndices      = &image_index;

    result                          = vkQueuePresentKHR(vulkan_context_ptr->vk_present_queue, &present_info);
    VK_CHECK(result);

    return true;
}
