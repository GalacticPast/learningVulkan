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
    STAGE_UNKNOWN  = 0x00000000,
};

enum shader_scope
{
    // global
    SHADER_PER_FRAME_UNIFORM  = 0,
    // local
    SHADER_PER_GROUP_UNIFORM  = 1,
    // object
    SHADER_PER_OBJECT_UNIFORM = 2,
    // unknown
    SHADER_SCOPE_UNKNOWN      = 3
};

enum attribute_types
{
    // sampler doesnt have a size
    SAMPLER_2D                = 8,
    MAT_4                     = 64,
    VEC_4                     = 16,
    // INFO: You have to align the size of vec3 to the same size as vec4 if you are using vec3's in the uniform buffer.
    //  if you dont know what this means. Do better future me :). In short For uniform buffers the vec3 has to be
    //  alligned to 16 bytes but for vertexInputAttribute the vec3 can be 12bytes large.
    VEC_3                     = 12,
    VEC_2                     = 8,
    MAX_SHADER_ATTRIBUTE_TYPE = 0,
};
// INFO: I just decided this arbitarily so....
#define MAX_UNIFORM_ATTRIBUTES 5

struct shader_uniform_config
{
    dstring         name;
    shader_stage    stage       = STAGE_UNKNOWN;
    shader_scope    scope       = SHADER_SCOPE_UNKNOWN;
    u32             set         = INVALID_ID;
    u32             binding     = INVALID_ID;
    u32             types_count = INVALID_ID;
    attribute_types types[MAX_UNIFORM_ATTRIBUTES];
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

    darray<shader_stage>            stages;
    darray<u32>                     per_frame_uniform_offsets;
    darray<u32>                     per_group_uniform_offsets;
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

#define DEFAULT_ALBEDO_TEXTURE_HANDLE   "DEFAULT_ALBDEO_TEXTURE"
#define DEFAULT_ALPHA_TEXTURE_HANDLE    "DEFAULT_ALPHA_TEXTURE"
#define DEFAULT_NORMAL_TEXTURE_HANDLE   "DEFAULT_NORMAL_TEXTURE"
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
// INFO: maybe name it differenlty
#define DEFAULT_LIGHT_MATERIAL_HANDLE "default_light_material"
#define MAX_MATERIALS_LOADED 1024
#define MATERIAL_NAME_MAX_LENGTH 256

struct texture_map
{
    texture *albedo = nullptr;
    texture *alpha  = nullptr;
    texture *normal = nullptr;
};

struct material_config
{
    char       mat_name[MATERIAL_NAME_MAX_LENGTH];       //        256
    char       albedo_map[TEXTURE_NAME_MAX_LENGTH];      // 512
    char       alpha_map[TEXTURE_NAME_MAX_LENGTH];       // 512
    char       normal_map[TEXTURE_NAME_MAX_LENGTH];      // 512
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
#define DEFAULT_PLANE_HANDLE "DEFAULT_PLANE_HANDLE"
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
