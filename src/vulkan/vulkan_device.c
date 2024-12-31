#include "vulkan_device.h"
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
    vkEnumeratePhysicalDevices(context->instance, &gpu_count, 0);
    INFO("No of GPU's: %d", gpu_count);

    return false;
}
