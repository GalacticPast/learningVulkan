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

b8 select_physical_device(vulkan_context *context);

b8 vulkan_create_logical_device(vulkan_context *context)
{
    INFO("Creating vulkan logical device...");
    if (!select_physical_device(context))
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
    queue_indicies[0] = context->device.graphics_queue_index;

    if (context->device.graphics_queue_index != context->device.present_queue_index)
    {
        queue_indicies[1] = context->device.present_queue_index;
    }

    VkDeviceQueueCreateInfo *queue_create_infos = array_create_with_capacity(VkDeviceQueueCreateInfo, queue_indicies_count);

    for (u32 i = 0; i < queue_indicies_count; i++)
    {
        VkDeviceQueueCreateInfo queue_create_info = {};
        queue_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queue_create_info.pNext = 0;
        queue_create_info.flags = 0;
        queue_create_info.queueFamilyIndex = queue_indicies[i];
        queue_create_info.queueCount = 1;
        f32 queue_priority = 1.0f;
        queue_create_info.pQueuePriorities = &queue_priority;

        queue_create_infos = array_push_value(queue_create_infos, &queue_create_info);
    }

    VkDeviceCreateInfo device_create_info = {};
    device_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    device_create_info.pNext = 0;
    device_create_info.flags = 0;

    u32 queue_create_infos_count = (u32)array_get_length(queue_create_infos);
    DEBUG("Queue create infos count: %d", queue_create_infos_count);

    device_create_info.queueCreateInfoCount = queue_create_infos_count;
    device_create_info.pQueueCreateInfos = queue_create_infos;
    device_create_info.enabledExtensionCount = 0;
    device_create_info.ppEnabledExtensionNames = 0;
    device_create_info.pEnabledFeatures = 0;

    VK_CHECK(vkCreateDevice(context->device.physical, &device_create_info, 0, &context->device.logical));
    INFO("Vulkan Logical device created");

    INFO("Obtaining queue families...");

    vkGetDeviceQueue(context->device.logical, context->device.graphics_queue_index, 0, &context->device.graphics_queue);
    vkGetDeviceQueue(context->device.logical, context->device.present_queue_index, 0, &context->device.present_queue);

    INFO("Queues obtained");
    array_destroy(queue_create_infos);

    return true;
}

b8 select_physical_device(vulkan_context *context)
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

    char **required_device_extensions = array_create(const char *);

    const char *swapchain_extension = VK_KHR_SWAPCHAIN_EXTENSION_NAME;
    required_device_extensions = array_push_value(required_device_extensions, &swapchain_extension);
    u32 required_device_extensions_count = (u32)array_get_length(required_device_extensions);

    physical_device_requirements device_requirements = {};

    device_requirements.is_discrete = true;
    device_requirements.geometry_shader = true;
    device_requirements.present_queue_family = true;
    device_requirements.swapchain_support = true;

    for (u32 i = 0; i < gpu_count; i++)
    {

        VkPhysicalDeviceProperties physical_properties = {};
        vkGetPhysicalDeviceProperties(physical_devices[i], &physical_properties);

        VkPhysicalDeviceFeatures physical_features = {};
        vkGetPhysicalDeviceFeatures(physical_devices[i], &physical_features);

        DEBUG("Physical Device Name: %s", physical_properties.deviceName);

        // INFO: check for device extensions support
        INFO("Checking device extensions support...");
        b8 swapchain_support = false;

        u32 device_extensions_count = 0;
        vkEnumerateDeviceExtensionProperties(physical_devices[i], 0, &device_extensions_count, 0);
        VkExtensionProperties *device_extension_properties = array_create_with_capacity(VkExtensionProperties, device_extensions_count);
        vkEnumerateDeviceExtensionProperties(physical_devices[i], 0, &device_extensions_count, device_extension_properties);

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
            continue;
        }
        INFO("All required device extensions found.");

        INFO("Checking for queue families support...");
        if ((device_requirements.is_discrete && physical_properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) &&
            (device_requirements.geometry_shader && physical_features.geometryShader))
        {
            // get the queue famiy indicies
            queue_family_indicies indicies = {};

            u32 queue_family_count = 0;

            vkGetPhysicalDeviceQueueFamilyProperties(physical_devices[i], &queue_family_count, 0);
            VkQueueFamilyProperties *queue_family_properties = array_create_with_capacity(VkQueueFamilyProperties, queue_family_count);

            vkGetPhysicalDeviceQueueFamilyProperties(physical_devices[i], &queue_family_count, queue_family_properties);
            array_set_length(queue_family_properties, queue_family_count);

            DEBUG("No of queue families %d", queue_family_count);

            b8 graphics_queue_family_found = false;
            b8 present_queue_family_found = false;

            for (u32 j = 0; j < queue_family_count; j++)
            {
                if (queue_family_properties[j].queueFlags & VK_QUEUE_GRAPHICS_BIT)
                {
                    graphics_queue_family_found = true;
                    indicies.graphics_queue_index = j;
                }
                VkBool32 present_queue = false;
                vkGetPhysicalDeviceSurfaceSupportKHR(physical_devices[i], j, context->surface, &present_queue);
                if (present_queue)
                {
                    present_queue_family_found = true;
                    indicies.present_queue_index = j;
                }
            }

            if (!graphics_queue_family_found)
            {
                ERROR("Graphics queue family required but not found. Skipping...");
                continue;
            }
            if (!present_queue_family_found)
            {
                ERROR("Present queue family required but not found. Skipping...");
                continue;
            }

            DEBUG("Graphics queue family index: %d", indicies.graphics_queue_index);
            DEBUG("Present queue family index: %d", indicies.present_queue_index);

            context->device.physical = physical_devices[i];
            context->device.physical_device_properties = physical_properties;
            context->device.physical_device_features = physical_features;
            context->device.graphics_queue_index = indicies.graphics_queue_index;
            context->device.present_queue_index = indicies.present_queue_index;

            // destroy the arrays
            array_destroy(queue_family_properties);
            break;
        }
        array_destroy(device_extension_properties);
        INFO("Found Queue families support");
    }
    array_destroy(physical_devices);
    array_destroy(required_device_extensions);
    INFO("Physical device selected");
    return true;
}
