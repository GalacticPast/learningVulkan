#include "application.hpp"
#include "logger.hpp"
#include "platform/platform.hpp"

static application_state *app_state_ptr;

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
    app_state_ptr->is_running         = false;
    app_state_ptr->is_minimized       = false;

    app_state_ptr->application_system_linear_allocator_memory_requirements = 1 * 1024 * 1024; // 1 mega bytes
    linear_allocator_create(&app_state_ptr->application_system_linear_allocator, app_state_ptr->application_system_linear_allocator_memory_requirements);

    u64 platform_mem_requirements = 0;
    platform_system_startup(&platform_mem_requirements, 0, 0);
    app_state_ptr->platform_system_state = linear_allocator_allocate(&app_state_ptr->application_system_linear_allocator, platform_mem_requirements);
    platform_system_startup(&platform_mem_requirements, app_state_ptr->platform_system_state, app_state_ptr->application_config);

    return true;
}

void application_shutdown()
{
}
