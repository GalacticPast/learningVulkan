#include "core/application.hpp"
#include "core/dasserts.hpp"
#include "core/dmemory.hpp"
#include "core/event.hpp"
#include "core/input.hpp"
#include "core/logger.hpp"

#include "defines.hpp"
#include "main.hpp"
#include "memory/arenas.hpp"
#include "platform/platform.hpp"

#include "renderer/renderer.hpp"

#include "resources/font_system.hpp"
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
    arena *system_arena          = app_state_ptr->system_arena;
    arena *resource_system_arena = app_state_ptr->resource_arena;

    u64  memory_system_memory_requirements = INVALID_ID_64;
    bool result                            = memory_system_startup(system_arena);
    DASSERT(result == true);

    result = event_system_startup(system_arena);
    DASSERT(result == true);

    event_system_register(EVENT_CODE_APPLICATION_QUIT, 0, event_callback_quit);
    event_system_register(EVENT_CODE_APPLICATION_RESIZED, 0, event_callback_resize);

    result = input_system_startup(system_arena);
    DASSERT(result);

    result = platform_system_startup(system_arena, app_state_ptr->application_config);
    DASSERT(result == true);

    result = shader_system_startup(system_arena, resource_system_arena);
    DASSERT(result == true);

    result = renderer_system_startup(system_arena, resource_system_arena, app_state_ptr->application_config);
    DASSERT(result == true);

    result = texture_system_initialize(system_arena, resource_system_arena);
    DASSERT(result == true);

    result = material_system_initialize(system_arena, resource_system_arena);
    DASSERT(result == true);

    result = font_system_initialize(system_arena,resource_system_arena);
    DASSERT(result == true);

    result = geometry_system_initialize(system_arena, resource_system_arena);
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

    texture_system_shutdown();
    renderer_system_shutdown();
    platform_system_shutdown();
    input_system_shutdown();
    event_system_shutdown();

    memory_system_shutdown();

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
