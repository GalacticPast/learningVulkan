#pragma once

#include "containers/darray.hpp"
#include "core/dstring.hpp"
#include "defines.hpp"
#include "main.hpp"

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
    SAMPLER_CUBE              = 88,
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

enum pipeline_cull_mode
{
    CULL_NONE_BIT = 0,
    CULL_FRONT_BIT,
    CULL_BACK_BIT,
};
enum pipeline_color_blend_options
{
    COLOR_BLEND_UNKNOWN                    = 0,
    COLOR_BLEND_FACTOR_ZERO                = 1,
    COLOR_BLEND_FACTOR_ONE                 = 2,
    COLOR_BLEND_FACTOR_SRC_ALPHA           = 3,
    COLOR_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA = 4,
};

struct pipeline_color_blend_state
{
    bool                         enable_color_blend = false;
    pipeline_color_blend_options src_color_blend_factor;
    pipeline_color_blend_options dst_color_blend_factor;
};

struct pipeline_depth_state
{
    bool enable_depth_blend = true;
};

struct shader_pipeline_configuration
{
    pipeline_cull_mode         mode;
    pipeline_color_blend_state color_blend;
    pipeline_depth_state       depth_state;
};

enum renderpass_types
{
    UNKNOWN_RENDERPASS_TYPE = 0,
    WORLD_RENDERPASS,
    UI_RENDERPASS,
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

    shader_pipeline_configuration pipeline_configuration;
    renderpass_types              renderpass_types;

    void init(arena *arena)
    {
        stages.c_init(arena);
        per_frame_uniform_offsets.c_init(arena);
        per_group_uniform_offsets.c_init(arena);
        uniforms.c_init(arena);
        attributes.c_init(arena);
    };
};

enum shader_type
{
    SHADER_TYPE_UNKNOWN  = 0,
    SHADER_TYPE_MATERIAL = 1,
    SHADER_TYPE_SKYBOX   = 2,
    SHADER_TYPE_GRID     = 3,
    SHADER_TYPE_UI       = 4,
};

struct shader
{
    shader_type type;
    void       *internal_vulkan_shader_state;
};

#define DEFAULT_ALBEDO_TEXTURE_HANDLE "DEFAULT_ALBDEO_TEXTURE"
#define DEFAULT_NORMAL_TEXTURE_HANDLE "DEFAULT_NORMAL_TEXTURE"
#define DEFAULT_CUBEMAP_TEXTURE_HANDLE "DEFAULT_CUBEMAP_TEXTURE"
#define DEFAULT_FONT_ATLAS_TEXTURE_HANDLE "DEFAULT_FONT_ATLAS_TEXTURE"

#define MAX_TEXTURES_LOADED 1024
#define TEXTURE_NAME_MAX_LENGTH 512

enum image_format
{
    IMG_FORMAT_UNKNOWN = 0,
    IMG_FORMAT_SRGB,
    IMG_FORMAT_UNORM,
};

struct texture
{

    dstring      name;
    u32          id           = INVALID_ID;
    u32          width        = INVALID_ID;
    u32          height       = INVALID_ID;
    u32          num_channels = INVALID_ID;
    u64          texure_size  = INVALID_ID_64;
    image_format format;
    void        *vulkan_texture_state = nullptr;
};

#define DEFAULT_TEXTURE_WIDTH 16
#define DEFAULT_TEXTURE_HEIGHT 16
#define DEFAULT_MATERIAL_HANDLE "default_material"
// INFO: maybe name it differenlty
#define DEFAULT_LIGHT_MATERIAL_HANDLE "default_light_material"
#define MAX_MATERIALS_LOADED 1024
#define MATERIAL_NAME_MAX_LENGTH 256

struct font_glyph_data
{
    u16 x0; // bounding box coords
    u16 y0; // uv coordinates for the quad
    u16 x1;
    u16 y1;

    u32 w;
    u32 h;

    f32 xoff; // adjust quad position on screen
    f32 yoff;
    f32 xadvance; // cursor movement or how much to move for the next atlas

    f32 xoff2;
    f32 yoff2;
};

struct texture_map
{
    texture *diffuse  = nullptr;
    texture *normal   = nullptr;
    texture *specular = nullptr;
};

struct material_config
{
    dstring mat_name;
    dstring albedo_map;
    dstring alpha_map;
    dstring normal_map;
    dstring specular_map;
    vec4    diffuse_color = {1.0f, 1.0f, 1.0f, 1.0f};
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
    vec4        diffuse_color = {1.0f, 1.0f, 1.0f, 1.0f};
};

#define DEFAULT_GEOMETRY_HANDLE "DEFAULT_GEOMETRY"
#define DEFAULT_PLANE_HANDLE "DEFAULT_PLANE_HANDLE"
#define MAX_GEOMETRIES_LOADED 1024
#define GEOMETRY_NAME_MAX_LENGTH 256

enum geometry_type
{
    GEO_TYPE_UNKNOWN = 0,
    GEO_TYPE_3D,
    GEO_TYPE_2D,
};

struct geometry_config
{
    dstring       name;
    geometry_type type;
    material     *material     = nullptr;
    u32           vertex_count = INVALID_ID;
    void         *vertices     = nullptr;
    u32           index_count  = INVALID_ID;
    u32          *indices      = nullptr;
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
