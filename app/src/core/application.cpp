#include "core/application.hpp"
#include "core/dasserts.hpp"
#include "core/dmemory.hpp"
#include "core/event.hpp"
#include "core/input.hpp"
#include "core/logger.hpp"

#include "defines.hpp"
#include "platform/platform.hpp"

#include "renderer/renderer.hpp"

#include "resources/geometry_system.hpp"
#include "resources/material_system.hpp"
#include "resources/texture_system.hpp"

static application_state *app_state_ptr;

bool event_callback_resize(event_context context, void *data);
bool event_callback_key_released(event_context context, void *data);
bool event_callback_quit(event_context context, void *data);

bool application_initialize(application_state *state, application_config *config)
{
    DINFO("Initializing application...");

    if (!state)
    {
        DERROR("Provided application_state is nullptr. Booting out");
        return false;
    }
    if (!config)
    {
        DERROR("Provided application_config is nullptr. Booting out");
        return false;
    }

    app_state_ptr                     = state;
    app_state_ptr->application_config = config;
    app_state_ptr->is_running         = true;
    app_state_ptr->is_minimized       = false;

    u64 memory_system_memory_requirements = INVALID_ID_64;
    memory_system_startup(&memory_system_memory_requirements, 0);
    app_state_ptr->memory_system_state = platform_allocate(memory_system_memory_requirements, false);
    DDEBUG("Allocated %dbytes", memory_system_memory_requirements);
    bool result = memory_system_startup(&memory_system_memory_requirements, app_state_ptr->memory_system_state);
    DASSERT(result == true);

    app_state_ptr->memory_system_memory_requirements = memory_system_memory_requirements;

    result                                                                 = false;
    app_state_ptr->application_system_linear_allocator_memory_requirements = 10 * 1024 * 1024; // 1 mega bytes
    result = linear_allocator_create(&app_state_ptr->application_system_linear_allocator,
                                     app_state_ptr->application_system_linear_allocator_memory_requirements);
    DASSERT(result == true);

    event_system_startup(&app_state_ptr->event_system_memory_requirements, 0);
    app_state_ptr->event_system_state = linear_allocator_allocate(&app_state_ptr->application_system_linear_allocator,
                                                                  app_state_ptr->event_system_memory_requirements);
    result = event_system_startup(&app_state_ptr->event_system_memory_requirements, app_state_ptr->event_system_state);
    DASSERT(result == true);

    event_system_register(EVENT_CODE_APPLICATION_QUIT, 0, event_callback_quit);
    event_system_register(EVENT_CODE_APPLICATION_RESIZED, 0, event_callback_resize);

    input_system_startup(&app_state_ptr->input_system_memory_requirements, 0);
    app_state_ptr->input_system_state = linear_allocator_allocate(&app_state_ptr->application_system_linear_allocator,
                                                                  app_state_ptr->input_system_memory_requirements);
    result = input_system_startup(&app_state_ptr->input_system_memory_requirements, app_state_ptr->input_system_state);
    DASSERT(result == true);

    platform_system_startup(&app_state_ptr->platform_system_memory_requirements, 0, 0);
    app_state_ptr->platform_system_state = linear_allocator_allocate(
        &app_state_ptr->application_system_linear_allocator, app_state_ptr->platform_system_memory_requirements);
    result = platform_system_startup(&app_state_ptr->platform_system_memory_requirements,
                                     app_state_ptr->platform_system_state, app_state_ptr->application_config);
    DASSERT(result == true);

    renderer_system_startup(&app_state_ptr->renderer_system_memory_requirements, 0, 0, 0);
    app_state_ptr->renderer_system_state = linear_allocator_allocate(
        &app_state_ptr->application_system_linear_allocator, app_state_ptr->renderer_system_memory_requirements);
    result = renderer_system_startup(
        &app_state_ptr->renderer_system_memory_requirements, app_state_ptr->application_config,
        &app_state_ptr->application_system_linear_allocator, app_state_ptr->renderer_system_state);
    DASSERT(result == true);

    texture_system_initialize(&app_state_ptr->texture_system_memory_requirements, 0);

    app_state_ptr->texture_system_state = linear_allocator_allocate(&app_state_ptr->application_system_linear_allocator,
                                                                    app_state_ptr->texture_system_memory_requirements);

    result = texture_system_initialize(&app_state_ptr->texture_system_memory_requirements,
                                       app_state_ptr->texture_system_state);
    DASSERT(result == true);

    material_system_initialize(&app_state_ptr->material_system_memory_requirements, 0);

    app_state_ptr->material_system_state = linear_allocator_allocate(
        &app_state_ptr->application_system_linear_allocator, app_state_ptr->material_system_memory_requirements);

    result = material_system_initialize(&app_state_ptr->material_system_memory_requirements,
                                        app_state_ptr->material_system_state);
    DASSERT(result == true);

    geometry_system_initialize(&app_state_ptr->geometry_system_memory_requirements, 0);

    app_state_ptr->geometry_system_state = linear_allocator_allocate(
        &app_state_ptr->application_system_linear_allocator, app_state_ptr->geometry_system_memory_requirements);

    result = geometry_system_initialize(&app_state_ptr->geometry_system_memory_requirements,
                                        app_state_ptr->geometry_system_state);
    DASSERT(result == true);

    u64 buffer_usg_mem_requirements = 0;
    get_memory_usg_str(&buffer_usg_mem_requirements, (char *)0);

    char buffer[8000];
    get_memory_usg_str(&buffer_usg_mem_requirements, buffer);
    DDEBUG("%s", buffer);

    return true;
}

void application_shutdown()
{
    // Destroy in opposite order of creation.

    event_system_unregister(EVENT_CODE_APPLICATION_QUIT, 0, event_callback_quit);
    event_system_unregister(EVENT_CODE_APPLICATION_RESIZED, 0, event_callback_resize);

    texture_system_shutdown(app_state_ptr->texture_system_state);
    renderer_system_shutdown();
    platform_system_shutdown(app_state_ptr->platform_system_state);
    input_system_shutdown(app_state_ptr->input_system_state);
    event_system_shutdown(app_state_ptr->event_system_state);
    linear_allocator_destroy(&app_state_ptr->application_system_linear_allocator);
    memory_system_shutdown(app_state_ptr->memory_system_state);
    DINFO("Appplication sucessfully shutdown.");
}

void application_run(render_data *data)
{
    renderer_draw_frame(data);
}

bool event_callback_quit(event_context context, void *data)
{
    if (app_state_ptr)
    {
        app_state_ptr->is_running = false;
        return true;
    }
    return false;
}

bool event_callback_resize(event_context context, void *data)
{
    if (app_state_ptr)
    {
        application_config *config = app_state_ptr->application_config;

        config->width  = context.data.u32[0];
        config->height = context.data.u32[1];

        DDEBUG("Resized event, new width: %d new height: %d", config->width, config->height);

        bool result = renderer_resize();
        if (!result)
        {
            DDEBUG("Renderer resize failed.");
        }

        return true;
    }
    return false;
}
