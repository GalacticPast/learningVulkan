#pragma once

#include "containers/darray.hpp"
#include "defines.hpp"
#include "memory/linear_allocator.hpp"
#include "resources/resource_types.hpp"


struct application_state
{
    bool                is_running;
    bool                is_minimized;
    application_config *application_config;


    arena *system_arena   = nullptr;
    arena *resource_arena = nullptr;

};

bool application_initialize(application_state *out_state, application_config *config);
void application_shutdown();
