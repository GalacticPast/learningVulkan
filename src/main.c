#include "containers/array.h"
#include "core/events.h"
#include "core/logger.h"
#include "core/memory.h"
#include "defines.h"

// C stuff
#include <stdio.h>

// vulkan stuff
#include "platform/platform.h"
#include "vulkan/vulkan_backend.h"
#include "vulkan/vulkan_types.h"

static b8 is_running = false;

void print_array(int *array)
{
    s32 length = (s32)array_get_length(array);
    for (s32 i = 0; i < length; i++)
    {
        printf("%d ", array[i]);
    }
    printf("\n");
}

b8 appilcaion_quit(event_type type, event_context data, void *user_data)
{
    INFO("APPLCATION QUIT MESSAGE recived");
    is_running = false;
    return true;
}
b8 application_resize(event_type type, event_context data, void *user_data)
{
    vulkan_context *context = (vulkan_context *)user_data;

    if (context->frame_buffer_width != data.data.u32[0] && context->frame_buffer_height != data.data.u32[1])

    {
        context->frame_buffer_last_frame_width  = context->frame_buffer_width;
        context->frame_buffer_last_frame_height = context->frame_buffer_height;
        context->frame_buffer_width             = data.data.u32[0];
        context->frame_buffer_height            = data.data.u32[1];

        DEBUG("New width %d, new height %d.", context->frame_buffer_width, context->frame_buffer_height);
        context->recreate_swapchain = true;

        return true;
    }

    return false;
}

s32 main(s32 argc, char **argv)
{
    is_running                   = true;
    const char *application_name = "learningVulkan";
    u32         x                = 0;
    u32         y                = 0;
    u32         width            = 1280;
    u32         height           = 720;

    platform_state plat_state     = {};
    vulkan_context vulkan_context = {};

    FATAL("Test %f", 3.14159);
    ERROR("Test %f", 3.14159);
    DEBUG("Test %f", 3.14159);
    WARN("Test %f", 3.14159);
    INFO("Test %f", 3.14159);
    TRACE("Test %f", 3.14159);

    s32 value = 10;
    s32 max   = 8;
    s32 min   = 4;

    value = CLAMP(value, min, max);
    TRACE("Clamped value %d", value);

    event_system_initialize();
    event_register(ON_APPLICATION_QUIT, appilcaion_quit, (void *)0);
    event_register(ON_APPLICATION_RESIZE, application_resize, &vulkan_context);

    vulkan_context.frame_buffer_width  = width;
    vulkan_context.frame_buffer_height = height;
    if (!platform_startup(&plat_state, application_name, x, y, width, height))
    {
        FATAL("Platform Initialization failed");
        return -1;
    }

    if (!initialize_vulkan(&plat_state, &vulkan_context, application_name))
    {
        FATAL("Vulkan Initialization failed");
        return -1;
    }

    print_memory_sizes();

    while (platform_pump_messages(&plat_state) && is_running)
    {
        vulkan_draw_frame(&vulkan_context);
    }

    shutdown_vulkan(&vulkan_context);
    platform_shutdown(&plat_state);
    event_system_destroy();
}
