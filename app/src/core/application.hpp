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

    u64              application_system_linear_allocator_memory_requirements; // 1 mega bytes
    linear_allocator application_system_linear_allocator;

    arena *system_arena   = nullptr;
    arena *resource_arena = nullptr;

    u64   platform_system_memory_requirements; // 1 mega bytes
    void *platform_system_state;

    u64   memory_system_memory_requirements; // 1 mega bytes
    void *memory_system_state;

    u64   event_system_memory_requirements; // 1 mega bytes
    void *event_system_state;

    u64   input_system_memory_requirements; // 1 mega bytes
    void *input_system_state;

    u64   renderer_system_memory_requirements; // 1 mega bytes
    void *renderer_system_state;

    u64   texture_system_memory_requirements; // 1 mega bytes
    void *texture_system_state;

    u64   material_system_memory_requirements; // 1 mega bytes
    void *material_system_state;

    u64   geometry_system_memory_requirements; // 1 mega bytes
    void *geometry_system_state;

    u64   shader_system_memory_requirements; // 1 mega bytes
    void *shader_system_state;
};

bool application_initialize(application_state *out_state, application_config *config);
void application_shutdown();
