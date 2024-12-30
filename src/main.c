#include "containers/array.h"
#include "core/events.h"
#include "core/logger.h"
#include "defines.h"

// C stuff
#include <stdio.h>

// vulkan stuff
#include "platform/platform.h"
#include "vulkan/vulkan_backend.h"
#include "vulkan/vulkan_types.h"

void print_array(int *array)
{
    u32 length = array_get_length(array);
    for (s32 i = 0; i < length; i++)
    {
        printf("%d ", array[i]);
    }
    printf("\n");
}

b8 it_works(event_type type, event_context data)
{
    INFO("EVENT SYSTEM WORKS");
    return false;
}

s32 main(s32 argc, char **argv)
{

    char *application_name = "learningVulkan";
    u32   x = 0;
    u32   y = 0;
    u32   width = 1280;
    u32   height = 720;

    platform_state plat_state = {};
    vulkan_context vk_context = {};

    event_system_initialize();

    event_register(ON_KEY_PRESS, it_works);

    if (!platform_startup(&plat_state, application_name, x, y, width, height))
    {
        FATAL("Platform Initialization failed");
    }

    if (!initialize_vulkan(&vk_context))
    {
        FATAL("Vulkan Initialization failed");
    }
}
