#pragma once

#include "defines.hpp"
#include "math/dmath_types.hpp"

#define LIGHTGRAY  (vec4){0.85f, 0.85f, 0.85f, 1.0f}
#define GRAY       (vec4){0.40f, 0.40f, 0.40f, 1.0f}
#define DARKGRAY   (vec4){0.15f, 0.15f, 0.15f, 1.0f}

#define YELLOW     (vec4){1.00f, 0.90f, 0.00f, 1.0f}
#define GOLD       (vec4){1.00f, 0.68f, 0.00f, 1.0f}
#define ORANGE     (vec4){1.00f, 0.45f, 0.00f, 1.0f}
#define PINK       (vec4){1.00f, 0.25f, 0.60f, 1.0f}

#define RED        (vec4){0.85f, 0.05f, 0.15f, 1.0f}
#define MAROON     (vec4){0.60f, 0.00f, 0.10f, 1.0f}

#define GREEN      (vec4){0.00f, 0.80f, 0.20f, 1.0f}
#define LIME       (vec4){0.00f, 0.60f, 0.10f, 1.0f}
#define DARKGREEN  (vec4){0.00f, 0.40f, 0.10f, 1.0f}

#define SKYBLUE    (vec4){0.20f, 0.65f, 1.00f, 1.0f}
#define BLUE       (vec4){0.00f, 0.35f, 0.90f, 1.0f}
#define DARKBLUE   (vec4){0.00f, 0.20f, 0.50f, 1.0f}

#define PURPLE     (vec4){0.70f, 0.30f, 1.00f, 1.0f}
#define VIOLET     (vec4){0.50f, 0.10f, 0.75f, 1.0f}
#define DARKPURPLE (vec4){0.35f, 0.00f, 0.40f, 1.0f}

#define BEIGE      (vec4){0.90f, 0.75f, 0.50f, 1.0f}
#define BROWN      (vec4){0.40f, 0.25f, 0.10f, 1.0f}
#define DARKBROWN  (vec4){0.25f, 0.15f, 0.05f, 1.0f}

#define WHITE      (vec4){1.00f, 1.00f, 1.00f, 1.0f}
#define BLACK      (vec4){0.00f, 0.00f, 0.00f, 1.0f}
#define BLANK      (vec4){0.00f, 0.00f, 0.00f, 0.0f}
#define MAGENTA    (vec4){1.00f, 0.00f, 1.00f, 1.0f}

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
    vec4 color;
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

struct ui_global_uniform_buffer_object
{
    mat4 view;
    mat4 projection; // orthograpic
    s32 mode;
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

struct geometry;

struct render_data
{
    ui_global_uniform_buffer_object    ui_ubo;
    scene_global_uniform_buffer_object scene_ubo;
    light_global_uniform_buffer_object light_ubo;

    u32        geometry_count_3D = INVALID_ID;
    geometry **test_geometry_3D  = nullptr;

    u32        geometry_count_2D = INVALID_ID;
    geometry **test_geometry_2D  = nullptr;
};
