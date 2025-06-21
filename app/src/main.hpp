#pragma once

#include "defines.hpp"
#include "math/dmath_types.hpp"

#define LIGHTGRAY  (vec4){0.7843f, 0.7843f, 0.7843f, 1.0f}
#define GRAY       (vec4){0.5098f, 0.5098f, 0.5098f, 1.0f}
#define DARKGRAY   (vec4){0.3137f, 0.3137f, 0.3137f, 1.0f}
#define YELLOW     (vec4){0.9922f, 0.9765f, 0.0000f, 1.0f}
#define GOLD       (vec4){1.0000f, 0.7961f, 0.0000f, 1.0f}
#define ORANGE     (vec4){1.0000f, 0.6314f, 0.0000f, 1.0f}
#define PINK       (vec4){1.0000f, 0.4275f, 0.7608f, 1.0f}
#define RED        (vec4){0.9020f, 0.1608f, 0.2157f, 1.0f}
#define MAROON     (vec4){0.7451f, 0.1294f, 0.2157f, 1.0f}
#define GREEN      (vec4){0.0000f, 0.8941f, 0.1882f, 1.0f}
#define LIME       (vec4){0.0000f, 0.6196f, 0.1843f, 1.0f}
#define DARKGREEN  (vec4){0.0000f, 0.4588f, 0.1725f, 1.0f}
#define SKYBLUE    (vec4){0.4000f, 0.7490f, 1.0000f, 1.0f}
#define BLUE       (vec4){0.0000f, 0.4745f, 0.9451f, 1.0f}
#define DARKBLUE   (vec4){0.0000f, 0.3216f, 0.6745f, 1.0f}
#define PURPLE     (vec4){0.7843f, 0.4784f, 1.0000f, 1.0f}
#define VIOLET     (vec4){0.5294f, 0.2353f, 0.7451f, 1.0f}
#define DARKPURPLE (vec4){0.4392f, 0.1216f, 0.4941f, 1.0f}
#define BEIGE      (vec4){0.8275f, 0.6902f, 0.5137f, 1.0f}
#define BROWN      (vec4){0.4980f, 0.4157f, 0.3098f, 1.0f}
#define DARKBROWN  (vec4){0.2980f, 0.2471f, 0.1843f, 1.0f}

#define WHITE      (vec4){1.0000f, 1.0000f, 1.0000f, 1.0f}
#define BLACK      (vec4){0.0000f, 0.0000f, 0.0000f, 1.0f}
#define BLANK      (vec4){0.0000f, 0.0000f, 0.0000f, 0.0f}
#define MAGENTA    (vec4){1.0000f, 0.0000f, 1.0000f, 1.0f}

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
