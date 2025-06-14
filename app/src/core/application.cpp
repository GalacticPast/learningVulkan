#include "core/application.hpp"
#include "core/dasserts.hpp"
#include "core/dmemory.hpp"
#include "core/event.hpp"
#include "core/input.hpp"
#include "core/logger.hpp"

#include "defines.hpp"
#include "memory/arenas.hpp"
#include "platform/platform.hpp"

#include "renderer/renderer.hpp"

#include "resources/geometry_system.hpp"
#include "resources/material_system.hpp"
#include "resources/shader_system.hpp"
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

    u64 arena_pool_size = GB(1);
    u32 num_arenas      = 8;
    arena_allocate_arena_pool(arena_pool_size, num_arenas);

    app_state_ptr->system_arena   = arena_get_arena();
    app_state_ptr->resource_arena = arena_get_arena();
    DASSERT(app_state_ptr->system_arena);
    arena *system_arena = app_state_ptr->system_arena;
    arena *resource_system_arena = app_state_ptr->resource_arena;

    u64 memory_system_memory_requirements = INVALID_ID_64;
    memory_system_startup(&memory_system_memory_requirements, 0);
    app_state_ptr->memory_system_state =
        dallocate(app_state_ptr->system_arena, memory_system_memory_requirements, MEM_TAG_APPLICATION);
    DDEBUG("Allocated %dbytes", memory_system_memory_requirements);
    bool result = memory_system_startup(&memory_system_memory_requirements, app_state_ptr->memory_system_state);
    DASSERT(result == true);

    app_state_ptr->memory_system_memory_requirements = memory_system_memory_requirements;

    result                                                                 = false;
    app_state_ptr->application_system_linear_allocator_memory_requirements = 10 * 1024 * 1024; // 1 mega bytes
    result = linear_allocator_create(system_arena, &app_state_ptr->application_system_linear_allocator,
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

    shader_system_startup(resource_system_arena, &app_state_ptr->shader_system_memory_requirements, 0);
    app_state_ptr->shader_system_state = linear_allocator_allocate(&app_state_ptr->application_system_linear_allocator,
                                                                   app_state_ptr->shader_system_memory_requirements);
    result = shader_system_startup(resource_system_arena, &app_state_ptr->shader_system_memory_requirements,
                                   app_state_ptr->shader_system_state);
    DASSERT(result == true);

    // why are we passing the linear allocator to the renderer system ?? are we allocating something? THis should not be
    // the case because the systems linear allocator should only be used for system level abstractions
    renderer_system_startup(system_arena, &app_state_ptr->renderer_system_memory_requirements, 0, 0, 0);
    app_state_ptr->renderer_system_state = linear_allocator_allocate(
        &app_state_ptr->application_system_linear_allocator, app_state_ptr->renderer_system_memory_requirements);
    result = renderer_system_startup(
        system_arena, &app_state_ptr->renderer_system_memory_requirements, app_state_ptr->application_config,
        &app_state_ptr->application_system_linear_allocator, app_state_ptr->renderer_system_state);
    DASSERT(result == true);

    texture_system_initialize(resource_system_arena, &app_state_ptr->texture_system_memory_requirements, 0);

    app_state_ptr->texture_system_state = linear_allocator_allocate(&app_state_ptr->application_system_linear_allocator,
                                                                    app_state_ptr->texture_system_memory_requirements);

    result = texture_system_initialize(resource_system_arena, &app_state_ptr->texture_system_memory_requirements,
                                       app_state_ptr->texture_system_state);
    DASSERT(result == true);

    material_system_initialize(resource_system_arena, &app_state_ptr->material_system_memory_requirements, 0);

    app_state_ptr->material_system_state = linear_allocator_allocate(
        &app_state_ptr->application_system_linear_allocator, app_state_ptr->material_system_memory_requirements);

    result = material_system_initialize(resource_system_arena, &app_state_ptr->material_system_memory_requirements,
                                        app_state_ptr->material_system_state);
    DASSERT(result == true);

    geometry_system_initialize(resource_system_arena, &app_state_ptr->geometry_system_memory_requirements, 0);

    app_state_ptr->geometry_system_state = linear_allocator_allocate(
        &app_state_ptr->application_system_linear_allocator, app_state_ptr->geometry_system_memory_requirements);

    result = geometry_system_initialize(resource_system_arena, &app_state_ptr->geometry_system_memory_requirements,
                                        app_state_ptr->geometry_system_state);
    DASSERT(result == true);

    u64 buffer_usg_mem_requirements = 0;
    get_memory_usg_str(&buffer_usg_mem_requirements, static_cast<char *>(0));

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

    arena_free_arena(app_state_ptr->resource_arena);
    arena_free_arena(app_state_ptr->system_arena);

    arena_free_arena_pool();
    DINFO("Appplication sucessfully shutdown.");
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
