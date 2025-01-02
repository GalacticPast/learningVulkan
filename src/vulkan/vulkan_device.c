#include "vulkan_device.h"
#include "containers/array.h"
#include "core/logger.h"

typedef struct physical_device_requirements
{
    b8 is_discrete;
    b8 geometry_shader;
} physical_device_requirements;

typedef struct queue_family_indicies
{
    u32 graphics_queue_index;
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

    VkDeviceQueueCreateInfo graphics_queue_create_info = {};
    graphics_queue_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    graphics_queue_create_info.pNext = 0;
    graphics_queue_create_info.flags = 0;
    graphics_queue_create_info.queueFamilyIndex = context->device.graphics_queue_index;
    graphics_queue_create_info.queueCount = 1;
    f32 graphics_queue_priority = 1.0f;
    graphics_queue_create_info.pQueuePriorities = &graphics_queue_priority;

    VkDeviceCreateInfo device_create_info = {};
    device_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    device_create_info.pNext = 0;
    device_create_info.flags = 0;

    // INFO: hard coded to one because we only need one queue
    device_create_info.queueCreateInfoCount = 1;
    device_create_info.pQueueCreateInfos = &graphics_queue_create_info;
    device_create_info.enabledExtensionCount = 0;
    device_create_info.ppEnabledExtensionNames = 0;
    device_create_info.pEnabledFeatures = 0;

    VK_CHECK(vkCreateDevice(context->device.physical, &device_create_info, 0, &context->device.logical));
    INFO("Vulkan Logical device created");
    
    INFO("Obtaining queue families...");
    vkGetDeviceQueue(context->device.logical, context->device.graphics_queue_index, 0, &context->device.graphics_queue);
    INFO("Queues obtained");

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

    physical_device_requirements device_requirements = {};

    device_requirements.is_discrete = true;
    device_requirements.geometry_shader = true;

    for (u32 i = 0; i < gpu_count; i++)
    {
        VkPhysicalDeviceProperties physical_properties = {};
        vkGetPhysicalDeviceProperties(physical_devices[i], &physical_properties);

        VkPhysicalDeviceFeatures physical_features = {};
        vkGetPhysicalDeviceFeatures(physical_devices[i], &physical_features);

        DEBUG("Physical Device Name: %s", physical_properties.deviceName);

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
            for (u32 j = 0; j < queue_family_count; j++)
            {
                if (queue_family_properties[j].queueFlags & VK_QUEUE_GRAPHICS_BIT)
                {
                    graphics_queue_family_found = true;
                    indicies.graphics_queue_index = j;
                    break;
                }
            }
            if (!graphics_queue_family_found)
            {
                ERROR("Graphics quueue family required but not found");
                return false;
            }
            DEBUG("Graphics queue family index: %d", indicies.graphics_queue_index);

            context->device.physical = physical_devices[i];
            context->device.physical_device_properties = physical_properties;
            context->device.physical_device_features = physical_features;
            context->device.graphics_queue_index = indicies.graphics_queue_index;

            // destroy the arrays
            array_destroy(queue_family_properties);
            break;
        }
    }
    array_destroy(physical_devices);
    INFO("Physical device selected");
    return true;
}
