#include "vulkan_device.hpp"
#include "core/dmemory.hpp"
#include "core/logger.hpp"

struct vulkan_physical_device_requirements
{
    bool is_discrete_gpu;
    bool has_geometry_shader;
};

bool vulkan_is_device_suitable(vulkan_context *vk_context, VkPhysicalDevice physical_device, vulkan_physical_device_requirements *device_requirements);

bool vulkan_create_logical_device(vulkan_context *vk_context)
{
    u32 physical_device_count = 0;
    vkEnumeratePhysicalDevices(vk_context->vk_instance, &physical_device_count, 0);

    if (physical_device_count == 0)
    {
        DFATAL("No gpu found with vulkan support. Booting out.");
        return false;
    }

    VkPhysicalDevice physical_devices[8];
    vkEnumeratePhysicalDevices(vk_context->vk_instance, &physical_device_count, physical_devices);

    // TODO: Make this configurable by the application
    vulkan_physical_device_requirements physical_device_requirements{};
    physical_device_requirements.is_discrete_gpu     = true;
    physical_device_requirements.has_geometry_shader = true;

    for (u32 i = 0; i < physical_device_count; i++)
    {
        if (vulkan_is_device_suitable(vk_context, physical_devices[i], &physical_device_requirements))
        {
            vk_context->device.physical = physical_devices[i];
            break;
        }
    }

    DINFO("Found suitable gpu. GPU name: %s", vk_context->device.physical_properties->deviceName);

    if (vk_context->device.physical == VK_NULL_HANDLE)
    {
        DERROR("Failed to find suitable gpu.");
        if (vk_context->device.physical_properties)
        {
            dfree(vk_context->device.physical_properties, sizeof(VkPhysicalDeviceProperties), MEM_TAG_RENDERER);
        }
        if (vk_context->device.physical_features)
        {
            dfree(vk_context->device.physical_features, sizeof(VkPhysicalDeviceFeatures), MEM_TAG_RENDERER);
        }
        return false;
    }

    return true;
}

bool vulkan_is_device_suitable(vulkan_context *vk_context, VkPhysicalDevice physical_device, vulkan_physical_device_requirements *device_requirements)
{
    VkPhysicalDeviceProperties physical_properties;
    vkGetPhysicalDeviceProperties(physical_device, &physical_properties);

    VkPhysicalDeviceFeatures physical_features;
    vkGetPhysicalDeviceFeatures(physical_device, &physical_features);

    if ((physical_properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU && device_requirements->is_discrete_gpu) &&
        (physical_features.geometryShader && device_requirements->has_geometry_shader))
    {
        vk_context->device.physical_properties = (VkPhysicalDeviceProperties *)dallocate(sizeof(VkPhysicalDeviceProperties), MEM_TAG_RENDERER);
        dcopy_memory(vk_context->device.physical_properties, &physical_properties, sizeof(VkPhysicalDeviceProperties));

        vk_context->device.physical_features = (VkPhysicalDeviceFeatures *)dallocate(sizeof(VkPhysicalDeviceFeatures), MEM_TAG_RENDERER);
        dcopy_memory(vk_context->device.physical_features, &physical_features, sizeof(VkPhysicalDeviceFeatures));
        return true;
    }
    return false;
}
