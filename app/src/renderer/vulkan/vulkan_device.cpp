#include "vulkan_device.hpp"
#include "core/dmemory.hpp"
#include "core/dstring.hpp"
#include "core/logger.hpp"

struct vulkan_physical_device_requirements
{
    bool is_discrete_gpu;
    bool has_geometry_shader;
    bool has_graphics_queue_family;
    bool has_present_queue_family;
    // INFO: I mean we need this for every gpu right??
    bool has_swapchain_support;
};

bool vulkan_is_physical_device_suitable(vulkan_context *vk_context, VkPhysicalDevice physical_device, vulkan_physical_device_requirements *physical_device_requirements,
                                        u32 required_device_extensions_count, const char **required_device_extensions);
bool vulkan_choose_physical_device(vulkan_context *vk_context, vulkan_physical_device_requirements *physical_device_requirements, u32 required_device_extensions_count,
                                   const char **required_device_extensions);

bool vulkan_create_logical_device(vulkan_context *vk_context)
{
    DINFO("Creating Vulkan logical device...");

    vulkan_physical_device_requirements physical_device_requirements{};
    physical_device_requirements.is_discrete_gpu           = true;
    physical_device_requirements.has_geometry_shader       = true;
    physical_device_requirements.has_graphics_queue_family = true;
    physical_device_requirements.has_present_queue_family  = true;
    physical_device_requirements.has_swapchain_support     = true;

    // INFO: This should be the same amount as the number of char literals in required_device_extensions array. Im doing this to avoid clang givimg me vla warnings.
    u32 required_device_extensions_count      = 1;
    const char *required_device_extensions[8] = {};
    required_device_extensions[0]             = VK_KHR_SWAPCHAIN_EXTENSION_NAME;

    bool result = vulkan_choose_physical_device(vk_context, &physical_device_requirements, required_device_extensions_count, required_device_extensions);

    if (!result)
    {
        DERROR("Couldnt find a gpu with vulkan_surpport.");
        return false;
    }

    u32 queue_family_count           = 0;
    u32 vulkan_queue_family_index[4] = {vk_context->device.graphics_family_index, vk_context->device.present_family_index, 0, 0};

    if (physical_device_requirements.has_graphics_queue_family)
    {
        queue_family_count++;
    }
    if (physical_device_requirements.has_present_queue_family)
    {
        queue_family_count++;
    }

    VkDeviceQueueCreateInfo device_queue_create_infos[4];

    f32 queue_priorities = 1.0f;

    for (u32 i = 0; i < queue_family_count; i++)
    {
        device_queue_create_infos[i].sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        device_queue_create_infos[i].pNext            = 0;
        device_queue_create_infos[i].flags            = 0;
        device_queue_create_infos[i].queueFamilyIndex = vulkan_queue_family_index[i];
        device_queue_create_infos[i].queueCount       = 1;
        device_queue_create_infos[i].pQueuePriorities = &queue_priorities;
    }

    // Get Queue families
    VkDeviceCreateInfo logical_device_create_info{};
    logical_device_create_info.sType                   = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    logical_device_create_info.pNext                   = 0;
    logical_device_create_info.flags                   = 0;
    logical_device_create_info.queueCreateInfoCount    = queue_family_count;
    logical_device_create_info.pQueueCreateInfos       = device_queue_create_infos;
    logical_device_create_info.enabledExtensionCount   = 0;
    logical_device_create_info.ppEnabledExtensionNames = 0;
    logical_device_create_info.pEnabledFeatures        = vk_context->device.physical_features;

    VkResult res = vkCreateDevice(vk_context->device.physical, &logical_device_create_info, vk_context->vk_allocator, &vk_context->device.logical);
    VK_CHECK(res);

    // get the queue handles
    vkGetDeviceQueue(vk_context->device.logical, vk_context->device.graphics_family_index, 0, &vk_context->graphics_queue);
    vkGetDeviceQueue(vk_context->device.logical, vk_context->device.present_family_index, 0, &vk_context->present_queue);

    return true;
}

bool vulkan_choose_physical_device(vulkan_context *vk_context, vulkan_physical_device_requirements *physical_device_requirements, u32 required_device_extensions_count,
                                   const char **required_device_extensions)
{
    u32 physical_device_count = INVALID_ID;
    vkEnumeratePhysicalDevices(vk_context->vk_instance, &physical_device_count, 0);

    if (physical_device_count == 0)
    {
        DFATAL("No gpu found with vulkan support. Booting out.");
        return false;
    }

    VkPhysicalDevice physical_devices[6];
    vkEnumeratePhysicalDevices(vk_context->vk_instance, &physical_device_count, physical_devices);

    // TODO: Make this configurable by the application

    for (u32 i = 0; i < physical_device_count; i++)
    {
        if (vulkan_is_physical_device_suitable(vk_context, physical_devices[i], physical_device_requirements, required_device_extensions_count, required_device_extensions))
        {
            vk_context->device.physical = physical_devices[i];
            break;
        }
    }

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

    DINFO("Vulkan: Found suitable gpu. GPU name: %s", vk_context->device.physical_properties->deviceName);
    DINFO("Vulkan: Found Graphics Queue Family. Queue family Index: %d", vk_context->device.graphics_family_index);
    DINFO("Vulkan: Found Present Queue Family. Queue family Index: %d", vk_context->device.present_family_index);

    return true;
}

bool vulkan_is_physical_device_suitable(vulkan_context *vk_context, VkPhysicalDevice physical_device, vulkan_physical_device_requirements *physical_device_requirements,
                                        u32 required_device_extensions_count, const char **required_device_extensions)
{
    VkPhysicalDeviceProperties physical_properties;
    vkGetPhysicalDeviceProperties(physical_device, &physical_properties);

    VkPhysicalDeviceFeatures physical_features;
    vkGetPhysicalDeviceFeatures(physical_device, &physical_features);

    u32 queue_family_count = INVALID_ID;
    VkQueueFamilyProperties queue_family_properties[8];

    u32 graphics_family_index = INVALID_ID;
    u32 present_family_index  = INVALID_ID;

    if ((physical_properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU && physical_device_requirements->is_discrete_gpu) &&
        (physical_features.geometryShader && physical_device_requirements->has_geometry_shader))
    {
        if (physical_device_requirements->has_graphics_queue_family && physical_device_requirements->has_present_queue_family)
        {
            vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_family_count, queue_family_properties);

#ifdef DEBUG
            DDEBUG("GPU NAME: %s", physical_properties.deviceName);

            for (u32 i = 0; i < queue_family_count; i++)
            {
                u32 offset = 0;
                char buffer[2000]{};

                offset += string_copy_format(buffer, "Queue Index: %d", offset, i);

                // INFO: putting offset - 1 because the string_copy_format null terminates the string
                // WARN: might be dangerous but idk :)
                buffer[offset - 1] = ' ';

                if (queue_family_properties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
                {
                    offset += string_copy(buffer, "VK_QUEUE_GRAPHICS_BIT | ", offset);
                }
                if (queue_family_properties[i].queueFlags & VK_QUEUE_COMPUTE_BIT)
                {
                    offset += string_copy(buffer, "VK_QUEUE_COMPUTE_BIT | ", offset);
                }
                if (queue_family_properties[i].queueFlags & VK_QUEUE_TRANSFER_BIT)
                {
                    offset += string_copy(buffer, "VK_QUEUE_TRANSFER_BIT | ", offset);
                }
                if (queue_family_properties[i].queueFlags & VK_QUEUE_PROTECTED_BIT)
                {
                    offset += string_copy(buffer, "VK_QUEUE_PROTECTED_BIT | ", offset);
                }
                DDEBUG("%s", buffer);
            }
#endif

            for (u32 i = 0; i < queue_family_count; i++)
            {
                if (queue_family_properties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
                {
                    graphics_family_index = i;
                }

                // INFO: check for vulkan surface support
                VkBool32 present_support = false;
                vkGetPhysicalDeviceSurfaceSupportKHR(physical_device, i, vk_context->vk_surface, &present_support);
                if (present_support)
                {
                    present_family_index = i;
                }
            }
        }

        if (physical_device_requirements->has_graphics_queue_family)
        {
            if (graphics_family_index != INVALID_ID)
            {
                vk_context->device.graphics_family_index = graphics_family_index;
            }
            else
            {
                DERROR("Graphics queue family required but not found.");
                return false;
            }
        }
        if (physical_device_requirements->has_present_queue_family)
        {
            if (present_family_index != INVALID_ID)
            {
                vk_context->device.present_family_index = present_family_index;
            }
            else
            {
                DERROR("Present queue family required but not found.");
                return false;
            }
        }

        // swapchain_support
        if (physical_device_requirements->has_swapchain_support)
        {
            u32 device_extension_properties_count = 0;
            VkExtensionProperties *device_extension_properties;

            vkEnumerateDeviceExtensionProperties(physical_device, 0, &device_extension_properties_count, 0);
            device_extension_properties = (VkExtensionProperties *)dallocate(sizeof(VkExtensionProperties) * device_extension_properties_count, MEM_TAG_RENDERER);
            vkEnumerateDeviceExtensionProperties(physical_device, 0, &device_extension_properties_count, device_extension_properties);
#ifdef DEBUG
            DDEBUG("Device Extension Properties Count %d", device_extension_properties_count);
            for (u32 i = 0; i < device_extension_properties_count; i++)
            {
                DDEBUG("Device Extension Name: %s", device_extension_properties[i].extensionName);
            }
#endif
            for (u32 i = 0; i < required_device_extensions_count; i++)
            {
                bool found_extension = false;
                for (u32 j = 0; j < device_extension_properties_count; j++)
                {
                    if (string_compare(required_device_extensions[i], device_extension_properties[j].extensionName))
                    {
                        found_extension = true;
                    }
                }
                if (found_extension == false)
                {
                    DERROR("Required device extension: '%s' but not found.", required_device_extensions[i]);
                    return false;
                }
            }
            dfree(device_extension_properties, sizeof(VkExtensionProperties) * device_extension_properties_count, MEM_TAG_RENDERER);
        }

        vk_context->device.physical_properties = (VkPhysicalDeviceProperties *)dallocate(sizeof(VkPhysicalDeviceProperties), MEM_TAG_RENDERER);
        dcopy_memory(vk_context->device.physical_properties, &physical_properties, sizeof(VkPhysicalDeviceProperties));

        vk_context->device.physical_features = (VkPhysicalDeviceFeatures *)dallocate(sizeof(VkPhysicalDeviceFeatures), MEM_TAG_RENDERER);
        dcopy_memory(vk_context->device.physical_features, &physical_features, sizeof(VkPhysicalDeviceFeatures));

        return true;
    }
    return false;
}
