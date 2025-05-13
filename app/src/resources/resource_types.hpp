#pragma once

#include "defines.hpp"

#include "core/dstring.hpp"
#include "math/dmath_types.hpp"

#define DEFAULT_TEXTURE_HANDLE "DEFAULT_TEXTURE"
#define MAX_TEXTURES_LOADED 1024
#define TEXTURE_NAME_MAX_LENGTH 512

struct texture
{

    dstring name;
    u32     id           = INVALID_ID;
    u32     tex_width    = INVALID_ID;
    u32     tex_height   = INVALID_ID;
    u32     num_channels = INVALID_ID;

    void *vulkan_texture_state = nullptr;
};

#define DEFAULT_MATERIAL_HANDLE "default_material"
#define MAX_MATERIALS_LOADED 1024
#define MATERIAL_NAME_MAX_LENGTH 256

struct texture_map
{
    texture *diffuse_tex = nullptr;
};

struct material_config
{
    char       mat_name[MATERIAL_NAME_MAX_LENGTH];        //        256
    char       diffuse_tex_name[TEXTURE_NAME_MAX_LENGTH]; // 512
    math::vec4 diffuse_color = {1.0f, 1.0f, 1.0f, 1.0f};  //                 32 * 4 =    128
    // TODO: add normal maps, heightmap etc..
};

struct material
{

    dstring     name;
    u32         id              = INVALID_ID;
    u32         reference_count = INVALID_ID;
    texture_map map;
    math::vec4  diffuse_color = {1.0f, 1.0f, 1.0f, 1.0f};
};

#define DEFAULT_GEOMETRY_HANDLE "DEFAULT_GEOMETRY"
#define MAX_GEOMETRIES_LOADED 1024
#define GEOMETRY_NAME_MAX_LENGTH 256

struct geometry_config
{
    char      name[GEOMETRY_NAME_MAX_LENGTH];
    material *material     = nullptr;
    u32       vertex_count = INVALID_ID;
    vertex   *vertices     = nullptr;
    u32       index_count  = INVALID_ID;
    u32      *indices      = nullptr;
};

struct geometry
{
    u32       id              = INVALID_ID;
    u32       reference_count = INVALID_ID;
    dstring   name;
    material *material              = nullptr;
    void     *vulkan_geometry_state = nullptr;
};
