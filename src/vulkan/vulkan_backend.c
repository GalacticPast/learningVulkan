#include "containers/array.h"

#include "core/events.h"
#include "core/logger.h"
#include "core/memory.h"
#include "core/strings.h"

#include "platform/platform.h"

#include "vulkan/vulkan_buffer.h"
#include "vulkan_backend.h"
#include "vulkan_command_buffers.h"
#include "vulkan_device.h"
#include "vulkan_frame_buffers.h"
#include "vulkan_graphics_pipeline.h"
#include "vulkan_image.h"
#include "vulkan_renderpass.h"
#include "vulkan_swapchain.h"
#include "vulkan_sync_objects.h"

VkBool32 debug_messenger_callback(VkDebugUtilsMessageSeverityFlagBitsEXT message_severity, VkDebugUtilsMessageTypeFlagsEXT message_types, const VkDebugUtilsMessengerCallbackDataEXT *callback_data,
                                  void *user_data);
void     debug_messenger_destroy(vulkan_context *context);

b8 initialize_vulkan(struct platform_state *plat_state, vulkan_context *context, const char *application_name)
{

    INFO("Initializing Vulkan...");

    context->max_frames_in_flight = 2;

    VkApplicationInfo app_info  = {};
    app_info.sType              = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    app_info.pNext              = 0;
    app_info.pApplicationName   = application_name;
    app_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    app_info.pEngineName        = "No engine";
    app_info.engineVersion      = VK_MAKE_VERSION(0, 0, 0);
    app_info.apiVersion         = VK_API_VERSION_1_2;

    // INFO: vulkan layers
    INFO("Checking required vulkan layers...");
    char **required_layer_names = array_create(const char *);

    char *validation_layer_name = "VK_LAYER_KHRONOS_validation";
    required_layer_names        = array_push_value(required_layer_names, &validation_layer_name);

    u32 layer_count = 0;
    VK_CHECK(vkEnumerateInstanceLayerProperties(&layer_count, 0));
    VkLayerProperties layer_properties[layer_count];
    VK_CHECK(vkEnumerateInstanceLayerProperties(&layer_count, layer_properties));

    DEBUG("Avaliable layers: %d", layer_count);
    /*
    for (u32 i = 0; i < layer_count; i++)
    {
        DEBUG("Layer Name:%s | Spec Version:%d | implementation version:%d | description:%s", layer_properties[i].layerName,
              layer_properties[i].specVersion, layer_properties[i].implementationVersion, layer_properties[i].description);
    }*/

    // INFO: to check if we have the required layer present
    u32 required_layer_count = (u32)array_get_length(required_layer_names);
    for (u32 i = 0; i < required_layer_count; i++)
    {
        b8 found = false;
        for (u32 j = 0; j < layer_count; j++)
        {
            if (string_compare(required_layer_names[i], layer_properties[j].layerName))
            {
                found = true;
            }
        }
        if (!found)
        {
            ERROR("Layer: %s required but not found.", layer_properties[i].layerName);
            return false;
        }
    }
    INFO("Required vulkan layers found.");
    //

    INFO("Checking required vulkan extensions...");

    char **required_extensions_name = array_create(const char *);
    char  *debug_utils              = VK_EXT_DEBUG_UTILS_EXTENSION_NAME;
    char  *surface_name             = VK_KHR_SURFACE_EXTENSION_NAME;

    required_extensions_name = array_push_value(required_extensions_name, &debug_utils);
    required_extensions_name = array_push_value(required_extensions_name, &surface_name);          // generic surface extension
    required_extensions_name = platform_get_required_surface_extensions(required_extensions_name); // platform specific surface extensions

    u32 required_extensions_count = (u32)array_get_length(required_extensions_name);

    u32 all_extension_count = 0;
    VK_CHECK(vkEnumerateInstanceExtensionProperties(0, &all_extension_count, 0));
    VkExtensionProperties all_extension_properties[all_extension_count];
    VK_CHECK(vkEnumerateInstanceExtensionProperties(0, &all_extension_count, all_extension_properties));

    /*    for (u32 j = 0; j < all_extension_count; j++)
        {
            DEBUG("Available Extensions: %s", all_extension_properties[j].extensionName);
        }*/

    for (u32 i = 0; i < required_extensions_count; i++)
    {
        b8 found = false;
        for (u32 j = 0; j < all_extension_count; j++)
        {
            if (string_compare(required_extensions_name[i], all_extension_properties[j].extensionName))
            {
                found = true;
                DEBUG("Found %s", all_extension_properties[j].extensionName);
            }
        }
        if (!found)
        {
            ERROR("Required extension: %s required but not found", required_extensions_name[i]);
        }
    }
    INFO("Required vulkan layer extensions found.");

    // INFO: create debug messenger
    VkDebugUtilsMessengerCreateInfoEXT debug_messenger_create_info = {};
    debug_messenger_create_info.sType                              = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    debug_messenger_create_info.pNext                              = 0;
    debug_messenger_create_info.flags                              = 0;
    debug_messenger_create_info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT;
    debug_messenger_create_info.messageType     = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    debug_messenger_create_info.pfnUserCallback = debug_messenger_callback;
    debug_messenger_create_info.pUserData       = 0;

    VkInstanceCreateInfo instance_create_info    = {};
    instance_create_info.sType                   = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instance_create_info.pNext                   = (VkDebugUtilsMessengerCreateInfoEXT *)&debug_messenger_create_info;
    instance_create_info.flags                   = 0;
    instance_create_info.pApplicationInfo        = &app_info;
    instance_create_info.enabledLayerCount       = (u32)required_layer_count;
    instance_create_info.ppEnabledLayerNames     = (const char *const *)required_layer_names;
    instance_create_info.enabledExtensionCount   = (u32)required_extensions_count;
    instance_create_info.ppEnabledExtensionNames = (const char *const *)required_extensions_name;

    VK_CHECK(vkCreateInstance(&instance_create_info, 0, &context->instance));

    // free the arrays
    array_destroy(required_layer_names);
    required_layer_names = 0;
    array_destroy(required_extensions_name);
    required_extensions_name = 0;

    INFO("Vulkan instance created");

    INFO("Creating Debug messenger...");

    // INFO: get the fucntion address because its a extension function and since vulkan doesnt load it by default
    PFN_vkCreateDebugUtilsMessengerEXT create_debug_utils_messenger = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(context->instance, "vkCreateDebugUtilsMessengerEXT");

    if (!create_debug_utils_messenger)
    {
        ERROR("Unable to get the function pointer to PFN_vkCreateDebugUtilsMessengerEXT");
        return false;
    }
    VK_CHECK(create_debug_utils_messenger(context->instance, &debug_messenger_create_info, 0, &context->debug_messenger));
    INFO("Debug messenger created");

    if (!platform_create_vulkan_surface(plat_state, context))
    {
        ERROR("Vulkan platform surface creation failed");
        return false;
    }
    if (!vulkan_create_logical_device(context))
    {
        ERROR("Vulkan Logical device creation failed");
        return false;
    }

    if (!vulkan_create_swapchain(context))
    {
        ERROR("Vulkan swapchain creation failed");
        return false;
    }

    if (!vulkan_create_image_views(context))
    {
        ERROR("Vulkan image views creation failed");
        return false;
    }
    if (!vulkan_create_renderpass(context))
    {
        ERROR("Vulkan renderpass creation failed");
        return false;
    }
    if (!vulkan_create_graphics_pipeline(context))
    {
        ERROR("Vulkan graphics pipeline creation failed");
        return false;
    }
    if (!vulkan_create_frame_buffers(context))
    {
        ERROR("Vulkan frame buffers creation failed");
        return false;
    }
    if (!vulkan_create_command_pool(context))
    {
        ERROR("Vulkan command pool creation failed");
        return false;
    }
    if (!vulkan_create_buffers(context))
    {
        ERROR("Vulkan vertex/index buffer creation failed");
        return false;
    }
    if (!vulkan_create_command_buffers(context))
    {
        ERROR("Vulkan command buffers creation failed");
        return false;
    }

    if (!vulkan_create_sync_objects(context))
    {
        ERROR("Vulkan sync objects creation failed");
        return false;
    }

    return true;
}

VkBool32 debug_messenger_callback(VkDebugUtilsMessageSeverityFlagBitsEXT message_severity, VkDebugUtilsMessageTypeFlagsEXT message_types, const VkDebugUtilsMessengerCallbackDataEXT *callback_data,
                                  void *user_data)
{
    const char *msg = callback_data->pMessage;
    switch (message_severity)
    {
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT: {
            TRACE("%s", msg);
        }
        break;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT: {
            INFO("%s", msg);
        }
        break;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT: {
            WARN("%s", msg);
        }
        break;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT: {
            ERROR("%s", msg);
        }
        break;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_FLAG_BITS_MAX_ENUM_EXT: {
        }
        break;
    }
    return VK_TRUE;
}

void debug_messenger_destroy(vulkan_context *context)
{
    PFN_vkDestroyDebugUtilsMessengerEXT destroy_debug_utils_messenger = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(context->instance, "vkDestroyDebugUtilsMessengerEXT");
    if (!destroy_debug_utils_messenger)
    {
        ERROR("Unable to get the function pointer to PFN_vkDestroyDebugUtilsMessengerEXT");
        return;
    }
    destroy_debug_utils_messenger(context->instance, context->debug_messenger, 0);
}

b8 shutdown_vulkan(vulkan_context *context)
{
    vkDeviceWaitIdle(context->device.logical);

    INFO("Destroying vertex buffers...");
    vulkan_destroy_buffer(context, &context->vertex_buffer);

    INFO("Destroying index buffers...");
    vulkan_destroy_buffer(context, &context->index_buffer);

    INFO("Destroying semaphores and fences...");

    u32 sync_objects_size = context->max_frames_in_flight;
    for (u32 i = 0; i < sync_objects_size; i++)
    {
        vkDestroySemaphore(context->device.logical, context->image_available_semaphores[i], 0);
        vkDestroySemaphore(context->device.logical, context->render_finished_semaphores[i], 0);
        vkDestroyFence(context->device.logical, context->in_flight_fences[i], 0);
    }

    INFO("Destroying command pool and buffers...");
    vkDestroyCommandPool(context->device.logical, context->command_pool, 0);

    INFO("Destroying frame buffers...");
    u32 frame_buffers_count = context->swapchain.image_views_count;
    for (u32 i = 0; i < frame_buffers_count; i++)
    {
        vkDestroyFramebuffer(context->device.logical, context->swapchain.frame_buffers[i], 0);
    }

    INFO("Destroying graphics pipeline...");
    vkDestroyPipeline(context->device.logical, context->graphics_pipeline.handle, 0);

    INFO("Destroying graphics pipeline layout...");
    vkDestroyPipelineLayout(context->device.logical, context->graphics_pipeline.layout, 0);

    INFO("Destroying rendper pass...");
    vkDestroyRenderPass(context->device.logical, context->renderpass, 0);

    INFO("Destroying image views...");
    u32 image_view_count = context->swapchain.image_views_count;
    for (u32 i = 0; i < image_view_count; i++)
    {
        vkDestroyImageView(context->device.logical, context->swapchain.image_views[i], 0);
    }

    INFO("Destroying vulkan swapchain...");
    vkDestroySwapchainKHR(context->device.logical, context->swapchain.handle, 0);

    INFO("Destroying vulkan device...");
    vkDestroyDevice(context->device.logical, 0);

    INFO("Destroying vulkan surface...");
    vkDestroySurfaceKHR(context->instance, context->surface, 0);

    INFO("Destroying debug messenger...");
    debug_messenger_destroy(context);

    INFO("Destroying vulkan instance...");
    vkDestroyInstance(context->instance, 0);

    return true;
}

void vulkan_draw_frame(vulkan_context *context)
{
    u32 current_frame = context->current_frame;

    if (context->recreate_swapchain)
    {
        context->recreate_swapchain = false;
        if (!vulkan_recreate_swapchain(context))
        {
            ERROR("Failed to recreate swapchain");
            return;
        }
    }

    VK_CHECK(vkWaitForFences(context->device.logical, 1, &context->in_flight_fences[current_frame], VK_TRUE, UINT64_MAX));

    u32 image_index = 0;

    VkResult next_image_result =
        vkAcquireNextImageKHR(context->device.logical, context->swapchain.handle, UINT64_MAX, context->image_available_semaphores[current_frame], VK_NULL_HANDLE, &image_index);

    if (next_image_result == VK_ERROR_OUT_OF_DATE_KHR)
    {
        DEBUG("next image result is out of date");
        if (!vulkan_recreate_swapchain(context))
        {
            ERROR("Failed to recreate swapchain");
            return;
        }
    }
    else if (next_image_result != VK_SUCCESS && next_image_result != VK_SUBOPTIMAL_KHR)
    {
        ERROR("Failed to acquire next image");
        debugbreak();
    }
    VK_CHECK(vkResetFences(context->device.logical, 1, &context->in_flight_fences[current_frame]));

    VK_CHECK(vkResetCommandBuffer(context->command_buffers[current_frame], 0));

    b8 result = vulkan_record_command_buffer(context, context->command_buffers[current_frame], image_index);

    if (!result)
    {
        ERROR("Couldnt record command buffer");
        return;
    }

    VkSubmitInfo queue_submit_info = {};

    queue_submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    queue_submit_info.pNext = 0;

    queue_submit_info.waitSemaphoreCount   = 1;
    VkPipelineStageFlags wait_stages[]     = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    queue_submit_info.pWaitSemaphores      = &context->image_available_semaphores[current_frame];
    queue_submit_info.pWaitDstStageMask    = wait_stages;
    queue_submit_info.commandBufferCount   = 1;
    queue_submit_info.pCommandBuffers      = &context->command_buffers[current_frame];
    queue_submit_info.signalSemaphoreCount = 1;
    queue_submit_info.pSignalSemaphores    = &context->render_finished_semaphores[current_frame];

    VK_CHECK(vkQueueSubmit(context->device.graphics_queue, 1, &queue_submit_info, context->in_flight_fences[current_frame]));

    VkPresentInfoKHR present_info = {};

    present_info.sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    present_info.pNext              = 0;
    present_info.waitSemaphoreCount = 1;
    present_info.pWaitSemaphores    = &context->render_finished_semaphores[current_frame];
    present_info.swapchainCount     = 1;
    present_info.pSwapchains        = &context->swapchain.handle;
    present_info.pImageIndices      = &image_index;
    present_info.pResults           = VK_NULL_HANDLE;

    VkResult queue_present_result = vkQueuePresentKHR(context->device.present_queue, &present_info);

    if (queue_present_result == VK_ERROR_OUT_OF_DATE_KHR || queue_present_result == VK_SUBOPTIMAL_KHR)
    {
        DEBUG("queue present result is out of date or suboptimal");
        if (!vulkan_recreate_swapchain(context))
        {
            ERROR("Failed to recreate swapchain");
            return;
        }
    }
    else if (queue_present_result != VK_SUCCESS)
    {
        ERROR("Failed to acquire next image");
        debugbreak();
    }

    context->current_frame = (current_frame + 1) % context->max_frames_in_flight;
}
