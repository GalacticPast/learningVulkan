#include "core/dmemory.hpp"
#include "core/logger.hpp"
#include "platform/platform.hpp"
#include "renderer.hpp"
#include "vulkan/vulkan_backend.hpp"

struct renderer_system_state
{
    f64 delta_time;

    linear_allocator *linear_systems_allocator;

    u64   vulkan_backend_memory_requirements;
    void *vulkan_backend_state;
};

static renderer_system_state *renderer_system_state_ptr;

bool renderer_system_startup(arena *system_arena, arena *resource_arena, struct application_config *app_config)
{
    DINFO("Initializing renderer system...");
    renderer_system_state_ptr = reinterpret_cast<renderer_system_state *>(
        dallocate(system_arena, sizeof(renderer_system_state), MEM_TAG_APPLICATION));

    vulkan_backend_initialize(system_arena, &renderer_system_state_ptr->vulkan_backend_memory_requirements, 0, 0);
    renderer_system_state_ptr->vulkan_backend_state =
        dallocate(system_arena, renderer_system_state_ptr->vulkan_backend_memory_requirements, MEM_TAG_APPLICATION);
    bool result =
        vulkan_backend_initialize(resource_arena, &renderer_system_state_ptr->vulkan_backend_memory_requirements,
                                  app_config, renderer_system_state_ptr->vulkan_backend_state);

    if (!result)
    {
        DFATAL("Vukan backend initialization failed.");
        return false;
    }

    return true;
}

void renderer_system_shutdown()
{

    if (renderer_system_state_ptr)
    {
        DINFO("Shutting down renderer...");
        vulkan_backend_shutdown();
        renderer_system_state_ptr = 0;
    }
}

void renderer_draw_frame(render_data *data)
{
    bool result = vulkan_draw_frame(data);
    if (!result)
    {
        DERROR("Smth wrong with drawing frame");
        return;
    }
}
bool renderer_resize()
{
    if (renderer_system_state_ptr && renderer_system_state_ptr->vulkan_backend_state)
    {
        vulkan_backend_resize();
        return true;
    }
    else
    {
        DDEBUG("Vulkan Backend doesnt exist to accept resize");
        return false;
    }
}

bool renderer_update_global_data(shader *shader, u32 offset, u32 size, void *data)
{
    bool result = vulkan_update_global_uniform_buffer(shader, offset, size, data);
    DASSERT(result);
    return result;
}

bool renderer_update_globals(shader *shader, darray<u32> &sizes)
{
    bool result = vulkan_update_global_descriptor_sets(shader, sizes);
    DASSERT(result);
    return result;
}
