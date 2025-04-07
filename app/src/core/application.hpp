#pragma once
#include "defines.hpp"
#include "memory/linear_allocator.hpp"
#include <string>

struct application_config
{
    s32         x;
    s32         y;
    s32         height;
    s32         width;
    const char *application_name;
};

struct application_state
{
    bool                is_running;
    bool                is_minimized;
    application_config *application_config;

    u64              application_system_linear_allocator_memory_requirements; // 1 mega bytes
    linear_allocator application_system_linear_allocator;

    u64   platform_system_memory_requirements; // 1 mega bytes
    void *platform_system_state;
};

bool application_initialize(application_state *out_state, application_config *config);
void application_shutdown();
