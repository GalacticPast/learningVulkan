#include "vulkan_device.h"
#include "containers/array.h"
#include "core/logger.h"
#include "core/strings.h"

typedef struct physical_device_requirements
{
    b8 is_discrete;
    b8 geometry_shader;
    b8 present_queue_family;
    b8 swapchain_support;
} physical_device_requirements;

typedef struct queue_family_indicies
{
    u32 graphics_queue_index;
    u32 present_queue_index;
} queue_family_indicies;

b8 select_physical_device(vulkan_context *context, physical_device_requirements requirements, const char **required_device_extensions);

b8 device_meets_requirements(vulkan_context *context, VkSurfaceKHR surface, VkPhysicalDevice device, VkPhysicalDeviceProperties *physical_properties,
                             VkPhysicalDeviceFeatures *physical_features, physical_device_requirements *device_requirements,
                             const char **required_device_extensions, vulkan_swapchain_support_details *swapchain_support_details);
void query_swapchain_support_details(VkPhysicalDevice physical_device, VkSurfaceKHR surface,
                                     vulkan_swapchain_support_details *swapchain_support_details);

b8 vulkan_create_logical_device(vulkan_context *context)
{
    INFO("Creating vulkan logical device...");

    const char **required_device_extensions = array_create(const char *);
    const char  *swapchain_extension        = VK_KHR_SWAPCHAIN_EXTENSION_NAME;

    required_device_extensions = array_push_value(required_device_extensions, &swapchain_extension);

    physical_device_requirements device_requirements = {};

    device_requirements.is_discrete          = true;
    device_requirements.geometry_shader      = true;
    device_requirements.present_queue_family = true;
    device_requirements.swapchain_support    = true;

    if (!select_physical_device(context, device_requirements, required_device_extensions))
    {
        ERROR("Couldnt select a physcial device aka GPU");
        return false;
    }

    // create the queues create infos
    DEBUG("Queuering queue family indicies");
    u32 queue_indicies_count = 1;
    if (context->device.graphics_queue_index != context->device.present_queue_index)
    {
        queue_indicies_count++;
    }

    u32 queue_indicies[queue_indicies_count] = {};
    queue_indicies[0]                        = context->device.graphics_queue_index;

    if (context->device.graphics_queue_index != context->device.present_queue_index)
    {
        queue_indicies[1] = context->device.present_queue_index;
    }

    VkDeviceQueueCreateInfo *queue_create_infos = array_create_with_capacity(VkDeviceQueueCreateInfo, queue_indicies_count);

    for (u32 i = 0; i < queue_indicies_count; i++)
    {
        VkDeviceQueueCreateInfo queue_create_info = {};
        queue_create_info.sType                   = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queue_create_info.pNext                   = 0;
        queue_create_info.flags                   = 0;
        queue_create_info.queueFamilyIndex        = queue_indicies[i];
        queue_create_info.queueCount              = 1;
        f32 queue_priority                        = 1.0f;
        queue_create_info.pQueuePriorities        = &queue_priority;

        queue_create_infos = array_push_value(queue_create_infos, &queue_create_info);
    }

    VkDeviceCreateInfo device_create_info = {};
    device_create_info.sType              = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    device_create_info.pNext              = 0;
    device_create_info.flags              = 0;

    u32 queue_create_infos_count = (u32)array_get_length(queue_create_infos);
    DEBUG("Queue create infos count: %d", queue_create_infos_count);

    device_create_info.queueCreateInfoCount    = queue_create_infos_count;
    device_create_info.pQueueCreateInfos       = queue_create_infos;
    device_create_info.enabledExtensionCount   = (u32)array_get_length(required_device_extensions);
    device_create_info.ppEnabledExtensionNames = required_device_extensions;
    device_create_info.pEnabledFeatures        = 0;

    VK_CHECK(vkCreateDevice(context->device.physical, &device_create_info, 0, &context->device.logical));
    INFO("Vulkan Logical device created");

    INFO("Obtaining queue families...");

    vkGetDeviceQueue(context->device.logical, context->device.graphics_queue_index, 0, &context->device.graphics_queue);
    vkGetDeviceQueue(context->device.logical, context->device.present_queue_index, 0, &context->device.present_queue);

    INFO("Queues obtained");
    array_destroy(queue_create_infos);
    array_destroy(required_device_extensions);

    return true;
}

b8 select_physical_device(vulkan_context *context, physical_device_requirements device_requirements, const char **required_device_extensions)
{
    // INFO: enumerate physical devices aka GPU'S
    INFO("Selecting physical device (aka gpu)...");

    u32 gpu_count = 0;
    VK_CHECK(vkEnumeratePhysicalDevices(context->instance, &gpu_count, 0));
    DEBUG("No of GPU's: %d", gpu_count);

    if (gpu_count == 0)
    {
        FATAL("No gpus found!!!");
        return false;
    }

    VkPhysicalDevice *physical_devices = array_create_with_capacity(VkPhysicalDevice, gpu_count);
    VK_CHECK(vkEnumeratePhysicalDevices(context->instance, &gpu_count, physical_devices));

    for (u32 i = 0; i < gpu_count; i++)
    {

        VkPhysicalDeviceProperties physical_properties = {};
        vkGetPhysicalDeviceProperties(physical_devices[i], &physical_properties);

        VkPhysicalDeviceFeatures physical_features = {};
        vkGetPhysicalDeviceFeatures(physical_devices[i], &physical_features);

        VkPhysicalDeviceMemoryProperties physical_memory = {};
        vkGetPhysicalDeviceMemoryProperties(physical_devices[i], &physical_memory);

        b8 result = device_meets_requirements(context, context->surface, physical_devices[i], &physical_properties, &physical_features,
                                              &device_requirements, required_device_extensions, &context->device.swapchain_support_details);
        if (!result)
        {
            array_destroy(context->device.swapchain_support_details.formats);
            array_destroy(context->device.swapchain_support_details.present_modes);
            context->device.swapchain_support_details.format_count       = 0;
            context->device.swapchain_support_details.present_mode_count = 0;
            continue;
        }

        DEBUG("Physical Device Name: %s", physical_properties.deviceName);
        for (u32 i = 0; i < physical_memory.memoryHeapCount; i++)
        {
            DEBUG("Memory size: %.2f GB", ((f64)physical_memory.memoryHeaps[i].size / (1024 * 1024 * 1024)));
        }
        context->device.physical = physical_devices[i];
        break;
    }

    array_destroy(physical_devices);

    INFO("Physical device selected");
    return true;
}

b8 device_meets_requirements(vulkan_context *context, VkSurfaceKHR surface, VkPhysicalDevice device, VkPhysicalDeviceProperties *physical_properties,
                             VkPhysicalDeviceFeatures *physical_features, physical_device_requirements *device_requirements,
                             const char **required_device_extensions, vulkan_swapchain_support_details *swapchain_support_details)
{
    INFO("Checking device extensions support...");

    DEBUG("Checking Swapchain support...");
    b8 swapchain_support = false;

    u32 device_extensions_count          = 0;
    u32 required_device_extensions_count = (u32)array_get_length(required_device_extensions);

    vkEnumerateDeviceExtensionProperties(device, 0, &device_extensions_count, 0);
    VkExtensionProperties *device_extension_properties = array_create_with_capacity(VkExtensionProperties, device_extensions_count);
    vkEnumerateDeviceExtensionProperties(device, 0, &device_extensions_count, device_extension_properties);

    DEBUG("Device Extensions count %d", device_extensions_count);

    // INFO: check for swapchain support
    for (u32 j = 0; j < required_device_extensions_count; j++)
    {
        for (u32 k = 0; k < device_extensions_count; k++)
        {
            // DEBUG("Available device extensions: %s", extension_properties[k].extensionName);
            if (string_compare(required_device_extensions[j], device_extension_properties[k].extensionName))
            {
                swapchain_support = true;
                break;
            }
        }
    }
    if (!swapchain_support)
    {
        ERROR("Swapchain support requested but not found. Skipping...");
        return false;
    }
    // INFO: check for swapchain support details

    query_swapchain_support_details(device, surface, swapchain_support_details);

    if (swapchain_support_details->format_count == 0 || swapchain_support_details->present_mode_count == 0)
    {
        ERROR("Swapchain deosnt support minimum format and present modes");
        return false;
    }

    //

    INFO("All required device extensions found.");

    INFO("Checking for queue families support...");
    if ((device_requirements->is_discrete && physical_properties->deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) &&
        (device_requirements->geometry_shader && physical_features->geometryShader))
    {
        // get the queue famiy indicies
        queue_family_indicies indicies = {};

        u32 queue_family_count = 0;

        vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, 0);
        VkQueueFamilyProperties *queue_family_properties = array_create_with_capacity(VkQueueFamilyProperties, queue_family_count);

        vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, queue_family_properties);
        array_set_length(queue_family_properties, queue_family_count);

        DEBUG("No of queue families %d", queue_family_count);

        b8 graphics_queue_family_found = false;
        b8 present_queue_family_found  = false;

        for (u32 j = 0; j < queue_family_count; j++)
        {
            if (queue_family_properties[j].queueFlags & VK_QUEUE_GRAPHICS_BIT)
            {
                graphics_queue_family_found   = true;
                indicies.graphics_queue_index = j;
            }
            VkBool32 present_queue = false;
            VK_CHECK(vkGetPhysicalDeviceSurfaceSupportKHR(device, j, surface, &present_queue));
            if (present_queue)
            {
                present_queue_family_found   = true;
                indicies.present_queue_index = j;
            }
        }

        if (!graphics_queue_family_found)
        {
            ERROR("Graphics queue family required but not found. Skipping...");
            return false;
        }
        if (!present_queue_family_found)
        {
            ERROR("Present queue family required but not found. Skipping...");
            return false;
        }

        context->device.graphics_queue_index = indicies.graphics_queue_index;
        context->device.present_queue_index  = indicies.present_queue_index;

        // destroy the arrays
        array_destroy(queue_family_properties);
    }
    return true;
}

void query_swapchain_support_details(VkPhysicalDevice physical_device, VkSurfaceKHR surface,
                                     vulkan_swapchain_support_details *swapchain_support_details)
{
    VK_CHECK(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physical_device, surface, &swapchain_support_details->capabilities));

    u32 format_count = 0;
    VK_CHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, surface, &format_count, 0));

    swapchain_support_details->formats = array_create_with_capacity(VkSurfaceFormatKHR, format_count);
    VK_CHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, surface, &format_count, swapchain_support_details->formats));
    array_set_length(swapchain_support_details->formats, format_count);
    swapchain_support_details->format_count = format_count;

    u32 present_mode_count = 0;
    VK_CHECK(vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device, surface, &present_mode_count, 0));

    swapchain_support_details->present_modes = array_create_with_capacity(VkPresentModeKHR, present_mode_count);
    VK_CHECK(vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device, surface, &present_mode_count, swapchain_support_details->present_modes));
    array_set_length(swapchain_support_details->present_modes, present_mode_count);
    swapchain_support_details->present_mode_count = present_mode_count;
}
