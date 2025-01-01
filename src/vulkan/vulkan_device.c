#include "vulkan_device.h"
#include "containers/array.h"
#include "core/logger.h"

b8 select_physical_device(vulkan_context *context);

b8 vulkan_create_logical_device(vulkan_context *context)
{
    if (!select_physical_device(context))
    {
    }
    return false;
}

b8 select_physical_device(vulkan_context *context)
{
    // INFO: enumerate physical devices aka GPU'S

    u32 gpu_count = 0;
    VK_CHECK(vkEnumeratePhysicalDevices(context->instance, &gpu_count, 0));
    INFO("No of GPU's: %d", gpu_count);

    if (gpu_count == 0)
    {
        FATAL("No gpus found!!!");
        return false;
    }

    VkPhysicalDevice *physicalDevices = array_create_with_capacity(VkPhysicalDevice, gpu_count);
    VK_CHECK(vkEnumeratePhysicalDevices(context->instance, &gpu_count, physicalDevices));

    for (u32 i = 0; i < gpu_count; i++)
    {
        VkPhysicalDeviceProperties physical_properties = {};
        vkGetPhysicalDeviceProperties(physicalDevices[i], &physical_properties);
        VkPhysicalDeviceFeatures physical_features = {};
        vkGetPhysicalDeviceFeatures(physicalDevices[i], &physical_features);
        INFO("Physical Device Name: %s", physical_properties.deviceName);
    }

    return false;
}
