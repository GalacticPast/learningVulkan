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
    bool has_sampler_anisotropy;
};

bool vulkan_is_physical_device_suitable(vulkan_context *vk_context, VkPhysicalDevice physical_device,
                                        vulkan_physical_device_requirements *physical_device_requirements,
                                        u32 required_device_extensions_count, const char **required_device_extensions);
bool vulkan_choose_physical_device(vulkan_context                      *vk_context,
                                   vulkan_physical_device_requirements *physical_device_requirements,
                                   u32 required_device_extensions_count, const char **required_device_extensions);

bool vulkan_create_logical_device(vulkan_context *vk_context)
{
    DDEBUG("Creating Vulkan logical device...");

    vulkan_physical_device_requirements physical_device_requirements{};
    physical_device_requirements.is_discrete_gpu           = false;
    physical_device_requirements.has_geometry_shader       = true;
    physical_device_requirements.has_graphics_queue_family = true;
    physical_device_requirements.has_present_queue_family  = true;
    physical_device_requirements.has_swapchain_support     = true;
    physical_device_requirements.has_sampler_anisotropy    = true;

    // INFO: This should be the same amount as the number of char literals in required_device_extensions array. Im doing
    // this to avoid clang givimg me vla warnings.
    u32         required_device_extensions_count = 2;
    const char *required_device_extensions[8]    = {};
    required_device_extensions[0]                = VK_KHR_SWAPCHAIN_EXTENSION_NAME;
    required_device_extensions[1]                = VK_KHR_SHADER_NON_SEMANTIC_INFO_EXTENSION_NAME;

    vk_context->vk_device.physical_properties =
        static_cast<VkPhysicalDeviceProperties *>(dallocate(sizeof(VkPhysicalDeviceProperties), MEM_TAG_RENDERER));
    vk_context->vk_device.physical_features =
        static_cast<VkPhysicalDeviceFeatures *>(dallocate(sizeof(VkPhysicalDeviceFeatures), MEM_TAG_RENDERER));

    bool result = vulkan_choose_physical_device(vk_context, &physical_device_requirements,
                                                required_device_extensions_count, required_device_extensions);

    if (!result)
    {
        DERROR("Couldnt find a gpu with vulkan_surpport.");
        return false;
    }

    u32 queue_family_count           = 0;
    u32 vulkan_queue_family_index[4] = {vk_context->vk_device.graphics_family_index,
                                        vk_context->vk_device.present_family_index, 0, 0};

    if (physical_device_requirements.has_graphics_queue_family &&
        vk_context->vk_device.graphics_family_index != INVALID_ID)
    {
        queue_family_count++;
    }
    // if we have 2 different queue family index for each
    if (physical_device_requirements.has_present_queue_family &&
        (vk_context->vk_device.present_family_index != INVALID_ID) &&
        vk_context->vk_device.present_family_index != vk_context->vk_device.graphics_family_index)
    {
        queue_family_count++;
    }
    vk_context->vk_device.enabled_queue_family_count = queue_family_count;

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
    logical_device_create_info.enabledExtensionCount   = required_device_extensions_count;
    logical_device_create_info.ppEnabledExtensionNames = required_device_extensions;
    logical_device_create_info.pEnabledFeatures        = vk_context->vk_device.physical_features;

    VkResult res = vkCreateDevice(vk_context->vk_device.physical, &logical_device_create_info, vk_context->vk_allocator,
                                  &vk_context->vk_device.logical);
    VK_CHECK(res);

    // get the queue handles
    vkGetDeviceQueue(vk_context->vk_device.logical, vk_context->vk_device.graphics_family_index, 0,
                     &vk_context->vk_device.graphics_queue);
    vkGetDeviceQueue(vk_context->vk_device.logical, vk_context->vk_device.present_family_index, 0,
                     &vk_context->vk_device.present_queue);

    return true;
}

bool vulkan_choose_physical_device(vulkan_context                      *vk_context,
                                   vulkan_physical_device_requirements *physical_device_requirements,
                                   u32 required_device_extensions_count, const char **required_device_extensions)
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
        if (vulkan_is_physical_device_suitable(vk_context, physical_devices[i], physical_device_requirements,
                                               required_device_extensions_count, required_device_extensions))
        {
            vk_context->vk_device.physical = physical_devices[i];
            break;
        }
        else
        {
            dzero_memory(vk_context->vk_device.physical_properties, sizeof(VkPhysicalDeviceProperties));
            dzero_memory(vk_context->vk_device.physical_features, sizeof(VkPhysicalDeviceFeatures));
        }
    }

    if (vk_context->vk_device.physical == VK_NULL_HANDLE)
    {
        DERROR("Failed to find suitable gpu.");
        if (vk_context->vk_device.physical_properties)
        {
            dfree(vk_context->vk_device.physical_properties, sizeof(VkPhysicalDeviceProperties), MEM_TAG_RENDERER);
        }
        if (vk_context->vk_device.physical_features)
        {
            dfree(vk_context->vk_device.physical_features, sizeof(VkPhysicalDeviceFeatures), MEM_TAG_RENDERER);
        }
        return false;
    }

    DINFO("Vulkan: Found suitable gpu. GPU name: %s", vk_context->vk_device.physical_properties->deviceName);
    DINFO("Vulkan: Found Graphics Queue Family. Queue family Index: %d", vk_context->vk_device.graphics_family_index);
    DINFO("Vulkan: Found Present Queue Family. Queue family Index: %d", vk_context->vk_device.present_family_index);

    return true;
}

bool vulkan_find_suitable_depth_format(vulkan_device *device, vulkan_image *depth_image)
{
    // Format candidates
    const u64 candidate_count = 3;
    VkFormat  candidates[3]   = {VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT};

    u32 flags = VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT;
    for (u64 i = 0; i < candidate_count; ++i)
    {
        VkFormatProperties properties;
        vkGetPhysicalDeviceFormatProperties(device->physical, candidates[i], &properties);

        if ((properties.linearTilingFeatures & flags) == flags)
        {
            depth_image->format = candidates[i];
            return true;
        }
        else if ((properties.optimalTilingFeatures & flags) == flags)
        {
            depth_image->format = candidates[i];
            return true;
        }
    }

    return false;
}

u32 vulkan_find_memory_type_index(vulkan_context *vk_context, u32 mem_type_filter,
                                  VkMemoryPropertyFlags mem_properties_flags)
{

    VkPhysicalDeviceMemoryProperties mem_properties;
    vkGetPhysicalDeviceMemoryProperties(vk_context->vk_device.physical, &mem_properties);

    for (u32 i = 0; i < mem_properties.memoryTypeCount; i++)
    {
        if (mem_type_filter & (1 << i) &&
            (mem_properties.memoryTypes[i].propertyFlags & mem_properties_flags) == mem_properties_flags)
        {
            return i;
        }
    }
    DERROR("Couldn't find memory type index.");
    DASSERT(true == false);
    return INVALID_ID;
}

bool vulkan_is_physical_device_suitable(vulkan_context *vk_context, VkPhysicalDevice physical_device,
                                        vulkan_physical_device_requirements *physical_device_requirements,
                                        u32 required_device_extensions_count, const char **required_device_extensions)
{
    VkPhysicalDeviceProperties physical_properties;
    vkGetPhysicalDeviceProperties(physical_device, &physical_properties);

    VkPhysicalDeviceFeatures physical_features;
    vkGetPhysicalDeviceFeatures(physical_device, &physical_features);

    u32                     queue_family_count = INVALID_ID;
    VkQueueFamilyProperties queue_family_properties[8];

    u32 graphics_family_index = INVALID_ID;
    u32 present_family_index  = INVALID_ID;

    bool is_suitable = true;

    if(physical_device_requirements->is_discrete_gpu)
    {
        is_suitable &= physical_properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU;
    }
    if(physical_features.geometryShader )
    {
        is_suitable &= physical_device_requirements->has_geometry_shader;
    }

    if (is_suitable)
    {
        if (physical_device_requirements->has_graphics_queue_family &&
            physical_device_requirements->has_present_queue_family)
        {
            vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_family_count, queue_family_properties);

#ifdef DEBUG
            DDEBUG("GPU NAME: %s", physical_properties.deviceName);

            for (u32 i = 0; i < queue_family_count; i++)
            {
                u32  offset = 0;
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
                vk_context->vk_device.graphics_family_index = graphics_family_index;
            }
            else
            {
                DERROR("Graphics queue family required but not found. Skipping device...");
                return false;
            }
        }
        if (physical_device_requirements->has_present_queue_family)
        {
            if (present_family_index != INVALID_ID)
            {
                vk_context->vk_device.present_family_index = present_family_index;
            }
            else
            {
                DERROR("Present queue family required but not found. Skipping device...");
                return false;
            }
        }

        // swapchain extension support
        if (physical_device_requirements->has_swapchain_support)
        {
            u32                    device_extension_properties_count = 0;
            VkExtensionProperties *device_extension_properties;

            vkEnumerateDeviceExtensionProperties(physical_device, 0, &device_extension_properties_count, 0);
            device_extension_properties = static_cast<VkExtensionProperties *>(
                dallocate(sizeof(VkExtensionProperties) * device_extension_properties_count, MEM_TAG_RENDERER));
            vkEnumerateDeviceExtensionProperties(physical_device, 0, &device_extension_properties_count,
                                                 device_extension_properties);
#ifdef DEBUG
            // DDEBUG("Device Extension Properties Count %d", device_extension_properties_count);
            // for (u32 i = 0; i < device_extension_properties_count; i++)
            //{
            //     DDEBUG("Device Extension Name: %s", device_extension_properties[i].extensionName);
            // }
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
                    DERROR("Required device extension: '%s' but not found. Skipping device...",
                           required_device_extensions[i]);
                    return false;
                }
            }
            dfree(device_extension_properties, sizeof(VkExtensionProperties) * device_extension_properties_count,
                  MEM_TAG_RENDERER);
        }

        // check swapchain support

        vulkan_swapchain dummy_swapchain{};
        vulkan_device_query_swapchain_support(vk_context, physical_device, &dummy_swapchain);

        if (!(physical_features.samplerAnisotropy) && physical_device_requirements->has_sampler_anisotropy)
        {
            DERROR("Sampler Anisotropy requested but current device doesnt support it. Skipping...");
            return false;
        }

        if (dummy_swapchain.surface_formats_count == INVALID_ID || dummy_swapchain.present_modes_count == INVALID_ID)
        {
            DERROR("Device VkSurfaceFormatKHR is 0. Skipping device.. ");
            DERROR("Device VkPresentModeKHR is 0. Skipping device.. ");
            dfree(dummy_swapchain.surface_formats, sizeof(VkSurfaceFormatKHR) * dummy_swapchain.surface_formats_count,
                  MEM_TAG_RENDERER);
            dfree(dummy_swapchain.present_modes, sizeof(VkPresentModeKHR) * dummy_swapchain.present_modes_count,
                  MEM_TAG_RENDERER);

            return false;
        }

        // if all above is true, then copy the device physical properties and physical features and query physical
        // memory properties.
        vkGetPhysicalDeviceMemoryProperties(physical_device, &vk_context->vk_device.memory_properties);

        dcopy_memory(vk_context->vk_device.physical_properties, &physical_properties,
                     sizeof(VkPhysicalDeviceProperties));
        dcopy_memory(vk_context->vk_device.physical_features, &physical_features, sizeof(VkPhysicalDeviceFeatures));

        dfree(dummy_swapchain.surface_formats, sizeof(VkSurfaceFormatKHR) * dummy_swapchain.surface_formats_count,
              MEM_TAG_RENDERER);
        dfree(dummy_swapchain.present_modes, sizeof(VkPresentModeKHR) * dummy_swapchain.present_modes_count,
              MEM_TAG_RENDERER);

        return true;
    }
    return false;
}

void vulkan_device_query_swapchain_support(vulkan_context *vk_context, VkPhysicalDevice physical_device,
                                           vulkan_swapchain *out_swapchain)
{

    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physical_device, vk_context->vk_surface,
                                              &out_swapchain->surface_capabilities);

    vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, vk_context->vk_surface, &out_swapchain->surface_formats_count,
                                         0);

    out_swapchain->surface_formats = static_cast<VkSurfaceFormatKHR *>(
        dallocate(sizeof(VkSurfaceFormatKHR) * out_swapchain->surface_formats_count, MEM_TAG_RENDERER));

    vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, vk_context->vk_surface, &out_swapchain->surface_formats_count,
                                         out_swapchain->surface_formats);

    vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device, vk_context->vk_surface,
                                              &out_swapchain->present_modes_count, 0);
    out_swapchain->present_modes = static_cast<VkPresentModeKHR *>(
        dallocate(sizeof(VkPresentModeKHR) * out_swapchain->present_modes_count, MEM_TAG_RENDERER));

    vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device, vk_context->vk_surface,
                                              &out_swapchain->present_modes_count, out_swapchain->present_modes);
}

bool vulkan_create_sync_objects(vulkan_context *vk_context)
{
    VkSemaphoreCreateInfo semaphore_create_info{};

    semaphore_create_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    semaphore_create_info.pNext = 0;
    semaphore_create_info.flags = 0;

    VkFenceCreateInfo fence_create_info{};

    fence_create_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fence_create_info.pNext = 0;
    fence_create_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    vk_context->image_available_semaphores =
        static_cast<VkSemaphore *>(dallocate(sizeof(VkSemaphore) * MAX_FRAMES_IN_FLIGHT, MEM_TAG_RENDERER));
    vk_context->render_finished_semaphores =
        static_cast<VkSemaphore *>(dallocate(sizeof(VkSemaphore) * MAX_FRAMES_IN_FLIGHT, MEM_TAG_RENDERER));

    vk_context->in_flight_fences =
        static_cast<VkFence *>(dallocate(sizeof(VkFence) * MAX_FRAMES_IN_FLIGHT, MEM_TAG_RENDERER));

    for (u32 i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        VkResult result = vkCreateSemaphore(vk_context->vk_device.logical, &semaphore_create_info,
                                            vk_context->vk_allocator, &vk_context->image_available_semaphores[i]);
        VK_CHECK(result);

        result = vkCreateSemaphore(vk_context->vk_device.logical, &semaphore_create_info, vk_context->vk_allocator,
                                   &vk_context->render_finished_semaphores[i]);
        VK_CHECK(result);

        result = vkCreateFence(vk_context->vk_device.logical, &fence_create_info, vk_context->vk_allocator,
                               &vk_context->in_flight_fences[i]);
        VK_CHECK(result);
    }

    return true;
}
