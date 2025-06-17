#pragma once

#include "defines.hpp"
#include "math/dmath_types.hpp"

struct application_config
{
    u32         x;
    u32         y;
    u32         height;
    u32         width;
    const char *application_name;
};

struct vertex_2D
{
    vec2 position;
    vec2 tex_coord;
};

struct vertex_3D
{
    vec3 position;
    vec3 normal;
    vec2 tex_coord;
    vec4 tangent;
};

struct camera
{
    vec3 euler;
    vec3 position;
    vec3 up = vec3(0, 1, 0);
};

struct scene_global_uniform_buffer_object
{
    mat4 view;
    mat4 projection;
    vec4 ambient_color;
    alignas(16) vec3 camera_pos;
};
struct light_global_uniform_buffer_object
{
    alignas(16) vec3 direction;
    alignas(16) vec4 color;
};

struct object_uniform_buffer_object
{
    mat4 model;
};

