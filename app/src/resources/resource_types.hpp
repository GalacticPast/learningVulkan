#pragma once

#include "defines.hpp"

#include "core/dstring.hpp"
#include "math/dmath_types.hpp"

#define DEFAULT_ALBEDO_TEXTURE_HANDLE "DEFAULT_ALBDEO_TEXTURE"
#define DEFAULT_ALPHA_TEXTURE_HANDLE "DEFAULT_ALPHA_TEXTURE"
#define MAX_TEXTURES_LOADED 1024
#define TEXTURE_NAME_MAX_LENGTH 512

struct texture
{

    dstring name;
    u32     id           = INVALID_ID;
    u32     width        = INVALID_ID;
    u32     height       = INVALID_ID;
    u32     num_channels = INVALID_ID;

    void *vulkan_texture_state = nullptr;
};

#define DEFAULT_MATERIAL_HANDLE "default_material"
#define MAX_MATERIALS_LOADED 1024
#define MATERIAL_NAME_MAX_LENGTH 256

struct texture_map
{
    texture *albedo = nullptr;
    texture *alpha  = nullptr;
};

struct material_config
{
    char       mat_name[MATERIAL_NAME_MAX_LENGTH];       //        256
    char       albedo_map[TEXTURE_NAME_MAX_LENGTH];      // 512
    char       alpha_map[TEXTURE_NAME_MAX_LENGTH];       // 512
    math::vec4 diffuse_color = {1.0f, 1.0f, 1.0f, 1.0f}; //                 32 * 4 =    128
    // TODO: add normal maps, heightmap etc..
};

struct material
{

    dstring name;
    u32     id                  = INVALID_ID;
    // WARN: should never change this
    u32 internal_id             = INVALID_ID;
    //
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
    u64                          id              = INVALID_ID_64;
    u32                          reference_count = INVALID_ID;
    dstring                      name;
    material                    *material              = nullptr;
    void                        *vulkan_geometry_state = nullptr;
    object_uniform_buffer_object ubo;
};
