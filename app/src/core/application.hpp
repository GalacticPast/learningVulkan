#pragma once

#include "containers/darray.hpp"
#include "defines.hpp"
#include "math/dmath.hpp"
#include "memory/linear_allocator.hpp"
#include "resources/texture_system.hpp"

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

struct camera
{
    vec3 euler;
    vec3 position;
    vec3 up = vec3(0, 1, 0);
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
    texture              *texture = nullptr;
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

    u64   texture_system_memory_requirements; // 1 mega bytes
    void *texture_system_state;

    u64   material_system_memory_requirements; // 1 mega bytes
    void *material_system_state;
};

bool application_initialize(application_state *out_state, application_config *config);
void application_shutdown();

// TODO: this a hack
void application_run(render_data *data);
