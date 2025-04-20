#pragma once

#include "containers/darray.hpp"
#include "defines.hpp"
#include "math/dmath.hpp"
#include "memory/linear_allocator.hpp"

struct application_config
{
    s32         x;
    s32         y;
    s32         height;
    s32         width;
    const char *application_name;
};

struct vertex
{
    vec3 position;
    vec3 color;
    vec2 tex_coord;
};

struct uniform_buffer_object
{
    mat4 model;
    mat4 view;
    mat4 projection;
};

struct render_data
{
    darray<vertex> vertices;
    darray<u32>    indices;

    uniform_buffer_object global_ubo;
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

    u64   memory_system_memory_requirements; // 1 mega bytes
    void *memory_system_state;

    u64   event_system_memory_requirements; // 1 mega bytes
    void *event_system_state;

    u64   input_system_memory_requirements; // 1 mega bytes
    void *input_system_state;

    u64   renderer_system_memory_requirements; // 1 mega bytes
    void *renderer_system_state;
};

bool application_initialize(application_state *out_state, application_config *config);
void application_shutdown();

// TODO: this a hack
void application_run(render_data *data);
