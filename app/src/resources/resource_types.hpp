#pragma once

#include "containers/darray.hpp"
#include "defines.hpp"

#include "core/dstring.hpp"
#include "math/dmath_types.hpp"

#define MAX_SHADER_COUNT 1024

enum shader_stage
{
    STAGE_VERTEX   = 0x00000001,
    STAGE_FRAGMENT = 0x00000010,
};

enum shader_scope
{
    // global
    SHADER_PER_FRAME_UNIFORM,
    // local
    SHADER_PER_GROUP_UNIFORM,
    // object
    SHADER_PER_OBJECT_UNIFORM,
};

enum attribute_types
{
    // sampler doesnt have a size
    SAMPLER_2D = 8,
    MAT_4      = 64,
    VEC_4      = 16,
    VEC_3      = 12,
    VEC_2      = 8,
};

struct shader_uniform_config
{
    dstring                 name;
    shader_stage            stage;
    shader_scope            scope;
    u32                     set;
    u32                     binding;
    darray<attribute_types> types;
};

// attributes can only be set for the vertex stage.
struct shader_attribute_config
{
    dstring         name;
    u32             location;
    attribute_types type;
};

struct shader_config
{
    // NOTE: for now
    bool has_per_frame;
    bool has_per_group;
    // NOTE: we will have a push constant regardless of this flag so idk.
    bool    has_per_object;
    dstring name;

    shader_stage                    stages;
    darray<shader_uniform_config>   uniforms;
    darray<shader_attribute_config> attributes;

    dstring vert_spv_full_path;
    dstring frag_spv_full_path;
};

struct shader
{
    u64 id = INVALID_ID_64;

    void *internal_vulkan_shader_state;
};

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
