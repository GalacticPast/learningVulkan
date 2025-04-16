#include "renderer.hpp"
#include "core/logger.hpp"
#include "platform/platform.hpp"
#include "vulkan/vulkan_backend.hpp"

struct renderer_system_state
{
    f64 delta_time;

    linear_allocator *linear_systems_allocator;

    u64   vulkan_backend_memory_requirements;
    void *vulkan_backend_state;
};

static renderer_system_state *renderer_system_state_ptr;

bool renderer_system_startup(u64 *renderer_system_memory_requirements, struct application_config *app_config,
                             linear_allocator *linear_systems_allocator, void *state)
{
    *renderer_system_memory_requirements = sizeof(renderer_system_state);
    if (!state)
    {
        return true;
    }
    DINFO("Initializing renderer system...");
    renderer_system_state_ptr = (renderer_system_state *)state;

    vulkan_backend_initialize(&renderer_system_state_ptr->vulkan_backend_memory_requirements, 0, 0);
    renderer_system_state_ptr->vulkan_backend_state = linear_allocator_allocate(
        linear_systems_allocator, renderer_system_state_ptr->vulkan_backend_memory_requirements);
    bool result = vulkan_backend_initialize(&renderer_system_state_ptr->vulkan_backend_memory_requirements, app_config,
                                            renderer_system_state_ptr->vulkan_backend_state);

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
    result = platform_pump_messages();
    if (!result)
    {
        DERROR("Smth wrong with platform pumping messages");
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
        DERROR("Vulkan Backend doesnt exist to accept resize");
        return false;
    }
}
