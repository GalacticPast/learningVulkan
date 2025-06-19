#include "defines.hpp"

#include "containers/dhashtable.hpp"

#include "core/dclock.hpp"
#include "core/dfile_system.hpp"
#include "core/dmemory.hpp"

#include "core/dstring.hpp"

#include "geometry_system.hpp"

#include "math/dmath.hpp"

#include "memory/arenas.hpp"
#include "renderer/vulkan/vulkan_backend.hpp"
#include "resources/material_system.hpp"
#include "resources/resource_types.hpp"
#include "resources/texture_system.hpp"

#include <cstring>
#include <stdio.h>

struct geometry_system_state
{
    darray<dstring>      loaded_geometry;
    dhashtable<geometry> hashtable;
    u64                  default_geo_id;
    arena               *arena;

    // HACK:
    u64             font_id;
    u32             vertex_offset_ind = INVALID_ID;
    u32             index_offset_ind  = INVALID_ID;
    geometry_config font_geometry_config;
};

static geometry_system_state *geo_sys_state_ptr;
bool                          geometry_system_create_default_geometry();

// this will allocate and write size back, assumes the caller will call free once the data is processed
static void geometry_system_parse_obj(arena *arena, const char *obj_file_full_path, u32 *num_of_objects,
                                      geometry_config **geo_configs);
static bool destroy_geometry_config(geometry_config *config);
static bool geometry_system_write_configs_to_file(dstring *file_full_path, u32 geometry_config_count,
                                                  geometry_config *configs);
static bool geometry_system_parse_bin_file(dstring *file_name, u32 *geometry_config_count, geometry_config **configs);
static void calculate_tangents(geometry_config *config);

bool geometry_system_initialize(arena *arena, u64 *geometry_system_mem_requirements, void *state)
{
    *geometry_system_mem_requirements = sizeof(geometry_system_state);
    if (!state)
    {
        return true;
    }
    DASSERT(arena);
    geo_sys_state_ptr = static_cast<geometry_system_state *>(state);

    geo_sys_state_ptr->hashtable.c_init(arena, MAX_GEOMETRIES_LOADED);
    geo_sys_state_ptr->hashtable.is_non_resizable = true;
    geo_sys_state_ptr->loaded_geometry.reserve(arena);
    geo_sys_state_ptr->arena = arena;

    geo_sys_state_ptr->vertex_offset_ind             = 0;
    geo_sys_state_ptr->index_offset_ind              = 0;
    geo_sys_state_ptr->font_id                       = INVALID_ID_64;
    geo_sys_state_ptr->font_geometry_config.vertices = dallocate(arena, sizeof(vertex_2D) * MB(1), MEM_TAG_GEOMETRY);
    geo_sys_state_ptr->font_geometry_config.indices =
        static_cast<u32 *>(dallocate(arena, sizeof(u32) * MB(1), MEM_TAG_GEOMETRY));

    geometry_system_create_default_geometry();

    return true;
}

bool geometry_system_shutdowm(void *state)
{
    u32       loaded_geometry_count = geo_sys_state_ptr->loaded_geometry.size();
    geometry *geo                   = nullptr;
    bool      result                = false;
    for (u32 i = 0; i < loaded_geometry_count; i++)
    {
        const char *geo_name = geo_sys_state_ptr->loaded_geometry[i].c_str();
        geo                  = geo_sys_state_ptr->hashtable.find(geo_name);

        result = vulkan_destroy_geometry(geo);
        if (!result)
        {
            DERROR("Failed to destroy %s geometry", geo_name);
        }
    }

    geo_sys_state_ptr->loaded_geometry.~darray();
    geo_sys_state_ptr = 0;
    return true;
}

bool geometry_system_update_geometry(geometry_config *config, u64 id)
{
    geometry *geo           = geometry_system_get_geometry(id);
    u32       tris_count    = config->vertex_count;
    u32       indices_count = config->index_count;
    bool      result        = vulkan_create_geometry(UI_RENDERPASS, geo, tris_count, sizeof(vertex_2D),
                                                     static_cast<void *>(config->vertices), indices_count, config->indices);
    geo_sys_state_ptr->hashtable.update(id, *geo);

    return result;
}

u64 geometry_system_create_geometry(geometry_config *config, bool use_name)
{
    DASSERT(config->type != GEO_TYPE_UNKNOWN);

    geometry geo{};
    u32      tris_count    = config->vertex_count;
    u32      indices_count = config->index_count;
    bool     result        = false;
    if (config->type == GEO_TYPE_3D)
    {
        calculate_tangents(config);
        result = vulkan_create_geometry(WORLD_RENDERPASS, &geo, tris_count, sizeof(vertex_3D), config->vertices,
                                        indices_count, config->indices);
    }
    else if (config->type == GEO_TYPE_2D)
    {
        result = vulkan_create_geometry(UI_RENDERPASS, &geo, tris_count, sizeof(vertex_2D),
                                        static_cast<void *>(config->vertices), indices_count, config->indices);
    }
    geo.name            = config->name;
    geo.reference_count = 0;
    geo.material        = config->material;
    // default model
    geo.ubo.model       = mat4();
    // we are setting this id to INVALID_ID_64 because the hashtable will genereate an ID for us.

    if (!result)
    {
        DERROR("Couldnt create geometry %s.", config->name.c_str());
        return false;
    }
    // the hashtable will create a unique id for us
    if (use_name)
    {
        geo_sys_state_ptr->hashtable.insert(config->name.c_str(), geo);
    }
    else
    {
        geo.id = geo_sys_state_ptr->hashtable.insert(INVALID_ID_64, geo);
    }

    geo_sys_state_ptr->loaded_geometry.push_back(geo.name);

    return geo.id;
}

geometry_config geometry_system_generate_quad_config(f32 width, f32 height, f32 posx, f32 posy, dstring *name)
{
    DASSERT(name);
    if (width == 0)
    {
        DWARN("Width must be nonzero. Defaulting to one.");
        width = 1.0f;
    }
    if (height == 0)
    {
        DWARN("Height must be nonzero. Defaulting to one.");
        height = 1.0f;
    }

    arena          *arena = geo_sys_state_ptr->arena;
    geometry_config config;

    config.vertex_count = 4;
    config.type         = GEO_TYPE_2D;
    config.vertices =
        static_cast<vertex_3D *>(dallocate(arena, sizeof(vertex_2D) * config.vertex_count, MEM_TAG_GEOMETRY));
    config.index_count = 6;
    config.indices     = static_cast<u32 *>(dallocate(arena, sizeof(u32) * config.index_count, MEM_TAG_GEOMETRY));

    vertex_2D *vertices = reinterpret_cast<vertex_2D *>(config.vertices);

    vertices[0].position  = {posx, posy};
    vertices[0].tex_coord = {0.0f, 0.0f};

    vertices[1].position  = {posx + width, posy};
    vertices[1].tex_coord = {1.0f, 0.0f};

    vertices[2].position  = {posx, posy + height};
    vertices[2].tex_coord = {0.0f, 1.0f};

    vertices[3].position  = {width + posx, posy + height};
    vertices[3].tex_coord = {1.0f, 1.0f};

    config.indices[0] = 0;
    config.indices[1] = 2;
    config.indices[2] = 1;

    config.indices[3] = 1;
    config.indices[4] = 2;
    config.indices[5] = 3;

    return config;
}

geometry_config geometry_system_generate_plane_config(f32 width, f32 height, u32 x_segment_count, u32 y_segment_count,
                                                      f32 tile_x, f32 tile_y, const char *name,
                                                      const char *material_name)
{
    if (width == 0)
    {
        DWARN("Width must be nonzero. Defaulting to one.");
        width = 1.0f;
    }
    if (height == 0)
    {
        DWARN("Height must be nonzero. Defaulting to one.");
        height = 1.0f;
    }
    if (x_segment_count < 1)
    {
        DWARN("x_segment_count must be a positive number. Defaulting to one.");
        x_segment_count = 1;
    }
    if (y_segment_count < 1)
    {
        DWARN("y_segment_count must be a positive number. Defaulting to one.");
        y_segment_count = 1;
    }

    if (tile_x == 0)
    {
        DWARN("tile_x must be nonzero. Defaulting to one.");
        tile_x = 1.0f;
    }
    if (tile_y == 0)
    {
        DWARN("tile_y must be nonzero. Defaulting to one.");
        tile_y = 1.0f;
    }

    arena          *arena = geo_sys_state_ptr->arena;
    geometry_config config;

    config.vertex_count = x_segment_count * y_segment_count * 4;
    config.vertices =
        static_cast<vertex_3D *>(dallocate(arena, sizeof(vertex_3D) * config.vertex_count, MEM_TAG_GEOMETRY));
    config.index_count = x_segment_count * y_segment_count * 6;
    config.indices     = static_cast<u32 *>(dallocate(arena, sizeof(u32) * config.index_count, MEM_TAG_GEOMETRY));

    // TODO: This generates extra vertices, but we can always deduplicate them later.
    f32 seg_width   = width / x_segment_count;
    f32 seg_height  = height / y_segment_count;
    f32 half_width  = width * 0.5f;
    f32 half_height = height * 0.5f;
    for (u32 y = 0; y < y_segment_count; ++y)
    {
        for (u32 x = 0; x < x_segment_count; ++x)
        {
            // Generate vertices
            f32 min_x   = (x * seg_width) - half_width;
            f32 min_z   = (y * seg_height) - half_height;
            f32 max_x   = min_x + seg_width;
            f32 max_z   = min_z + seg_height;
            f32 min_uvx = (x / static_cast<f32>(x_segment_count)) * tile_x;
            f32 min_uvy = (y / static_cast<f32>(y_segment_count)) * tile_y;
            f32 max_uvx = ((x + 1) / static_cast<f32>(x_segment_count)) * tile_x;
            f32 max_uvy = ((y + 1) / static_cast<f32>(y_segment_count)) * tile_y;

            u32        v_offset = ((y * x_segment_count) + x) * 4;
            vertex_3D *v0       = &(static_cast<vertex_3D *>(config.vertices))[v_offset + 0];
            vertex_3D *v1       = &(static_cast<vertex_3D *>(config.vertices))[v_offset + 1];
            vertex_3D *v2       = &(static_cast<vertex_3D *>(config.vertices))[v_offset + 2];
            vertex_3D *v3       = &(static_cast<vertex_3D *>(config.vertices))[v_offset + 3];

            v0->position.x  = min_x;
            v0->position.z  = -min_z;
            v0->normal.x    = 0;
            v0->normal.y    = 1;
            v0->normal.z    = 0;
            v0->tex_coord.x = min_uvx;
            v0->tex_coord.y = min_uvy;

            v1->position.x  = max_x;
            v1->position.z  = -max_z;
            v1->normal.x    = 0;
            v1->normal.y    = 1;
            v1->normal.z    = 0;
            v1->tex_coord.x = max_uvx;
            v1->tex_coord.y = max_uvy;

            v2->position.x  = min_x;
            v2->position.z  = -max_z;
            v2->normal.x    = 0;
            v2->normal.y    = 1;
            v2->normal.z    = 0;
            v2->tex_coord.x = min_uvx;
            v2->tex_coord.y = max_uvy;

            v3->position.x  = max_x;
            v3->position.z  = -min_z;
            v3->normal.x    = 0;
            v3->normal.y    = 1;
            v3->normal.z    = 0;
            v3->tex_coord.x = max_uvx;
            v3->tex_coord.y = min_uvy;

            // Generate indices
            u32 i_offset                 = ((y * x_segment_count) + x) * 6;
            config.indices[i_offset + 0] = v_offset + 0;
            config.indices[i_offset + 1] = v_offset + 1;
            config.indices[i_offset + 2] = v_offset + 2;
            config.indices[i_offset + 3] = v_offset + 0;
            config.indices[i_offset + 4] = v_offset + 3;
            config.indices[i_offset + 5] = v_offset + 1;
        }
    }

    string_ncopy(config.name.string, name, GEOMETRY_NAME_MAX_LENGTH);
    config.name.str_len = strlen(name);
    dstring mat_name    = material_name;
    config.material     = material_system_acquire_from_name(&mat_name);
    config.type         = GEO_TYPE_3D;
    return config;
}

geometry_config *geometry_system_generate_config(dstring obj_file_name)
{
    dstring file_full_path;
    dstring bin_file_full_path;

    const char *prefix = "../assets/meshes/";
    const char *suffix = ".bin";

    string_copy_format(file_full_path.string, "%s%s", 0, prefix, obj_file_name.c_str());
    string_copy_format(bin_file_full_path.string, "%s%s", 0, file_full_path.c_str(), suffix);

    geometry_config *config      = nullptr;
    u32              num_objects = INVALID_ID;

    bool write_to_file = false;

    bool result = file_exists(&bin_file_full_path);

    if (result)
    {
        result = geometry_system_parse_bin_file(&bin_file_full_path, &num_objects, &config);
        DASSERT(result);
    }
    else
    {

        geometry_system_parse_obj(geo_sys_state_ptr->arena, file_full_path.c_str(), &num_objects, &config);
        write_to_file = true;
    }

    if (write_to_file)
    {
        geometry_system_write_configs_to_file(&bin_file_full_path, 1, config);
    }

    DASSERT(config);
    config[0].material = material_system_get_default_material();

    return config;
}

geometry_config geometry_system_generate_cube_config()
{
    u32 width  = 2;
    u32 height = 2;
    u32 depth  = 2;
    f32 tile_x = 1;
    f32 tile_y = 1;

    geometry_config config;
    config.name         = DEFAULT_GEOMETRY_HANDLE;
    config.vertex_count = 4 * 6; // 4 verts per side, 6 sides
    config.vertices     = static_cast<vertex_3D *>(
        dallocate(geo_sys_state_ptr->arena, sizeof(vertex_3D) * config.vertex_count, MEM_TAG_GEOMETRY));
    config.index_count = 6 * 6; // 6 indices per side, 6 sides
    config.indices =
        static_cast<u32 *>(dallocate(geo_sys_state_ptr->arena, sizeof(u32) * config.index_count, MEM_TAG_GEOMETRY));
    config.type = GEO_TYPE_3D;

    f32 half_width  = width * 0.5f;
    f32 half_height = height * 0.5f;
    f32 half_depth  = depth * 0.5f;
    f32 min_x       = -half_width;
    f32 min_y       = -half_height;
    f32 min_z       = -half_depth;
    f32 max_x       = half_width;
    f32 max_y       = half_height;
    f32 max_z       = half_depth;
    f32 min_uvx     = 0.0f;
    f32 min_uvy     = 0.0f;
    f32 max_uvx     = tile_x;
    f32 max_uvy     = tile_y;

    vertex_3D verts[24];

    // back face
    verts[(0 * 4) + 0].position  = (vec3){min_x, min_y, max_z};
    verts[(0 * 4) + 1].position  = (vec3){max_x, max_y, max_z};
    verts[(0 * 4) + 2].position  = (vec3){min_x, max_y, max_z};
    verts[(0 * 4) + 3].position  = (vec3){max_x, min_y, max_z};
    verts[(0 * 4) + 0].tex_coord = (vec2){min_uvx, min_uvy};
    verts[(0 * 4) + 1].tex_coord = (vec2){max_uvx, max_uvy};
    verts[(0 * 4) + 2].tex_coord = (vec2){min_uvx, max_uvy};
    verts[(0 * 4) + 3].tex_coord = (vec2){max_uvx, min_uvy};
    verts[(0 * 4) + 0].normal    = (vec3){0.0f, 0.0f, 1.0f};
    verts[(0 * 4) + 1].normal    = (vec3){0.0f, 0.0f, 1.0f};
    verts[(0 * 4) + 2].normal    = (vec3){0.0f, 0.0f, 1.0f};
    verts[(0 * 4) + 3].normal    = (vec3){0.0f, 0.0f, 1.0f};

    // front face
    verts[(1 * 4) + 0].position  = (vec3){max_x, min_y, min_z};
    verts[(1 * 4) + 1].position  = (vec3){min_x, max_y, min_z};
    verts[(1 * 4) + 2].position  = (vec3){max_x, max_y, min_z};
    verts[(1 * 4) + 3].position  = (vec3){min_x, min_y, min_z};
    verts[(1 * 4) + 0].tex_coord = (vec2){min_uvx, min_uvy};
    verts[(1 * 4) + 1].tex_coord = (vec2){max_uvx, max_uvy};
    verts[(1 * 4) + 2].tex_coord = (vec2){min_uvx, max_uvy};
    verts[(1 * 4) + 3].tex_coord = (vec2){max_uvx, min_uvy};
    verts[(1 * 4) + 0].normal    = (vec3){0.0f, 0.0f, -1.0f};
    verts[(1 * 4) + 1].normal    = (vec3){0.0f, 0.0f, -1.0f};
    verts[(1 * 4) + 2].normal    = (vec3){0.0f, 0.0f, -1.0f};
    verts[(1 * 4) + 3].normal    = (vec3){0.0f, 0.0f, -1.0f};

    // Left
    verts[(2 * 4) + 0].position  = (vec3){min_x, min_y, min_z};
    verts[(2 * 4) + 1].position  = (vec3){min_x, max_y, max_z};
    verts[(2 * 4) + 2].position  = (vec3){min_x, max_y, min_z};
    verts[(2 * 4) + 3].position  = (vec3){min_x, min_y, max_z};
    verts[(2 * 4) + 0].tex_coord = (vec2){min_uvx, min_uvy};
    verts[(2 * 4) + 1].tex_coord = (vec2){max_uvx, max_uvy};
    verts[(2 * 4) + 2].tex_coord = (vec2){min_uvx, max_uvy};
    verts[(2 * 4) + 3].tex_coord = (vec2){max_uvx, min_uvy};
    verts[(2 * 4) + 0].normal    = (vec3){-1.0f, 0.0f, 0.0f};
    verts[(2 * 4) + 1].normal    = (vec3){-1.0f, 0.0f, 0.0f};
    verts[(2 * 4) + 2].normal    = (vec3){-1.0f, 0.0f, 0.0f};
    verts[(2 * 4) + 3].normal    = (vec3){-1.0f, 0.0f, 0.0f};

    // Right face
    verts[(3 * 4) + 0].position  = (vec3){max_x, min_y, max_z};
    verts[(3 * 4) + 1].position  = (vec3){max_x, max_y, min_z};
    verts[(3 * 4) + 2].position  = (vec3){max_x, max_y, max_z};
    verts[(3 * 4) + 3].position  = (vec3){max_x, min_y, min_z};
    verts[(3 * 4) + 0].tex_coord = (vec2){min_uvx, min_uvy};
    verts[(3 * 4) + 1].tex_coord = (vec2){max_uvx, max_uvy};
    verts[(3 * 4) + 2].tex_coord = (vec2){min_uvx, max_uvy};
    verts[(3 * 4) + 3].tex_coord = (vec2){max_uvx, min_uvy};
    verts[(3 * 4) + 0].normal    = (vec3){1.0f, 0.0f, 0.0f};
    verts[(3 * 4) + 1].normal    = (vec3){1.0f, 0.0f, 0.0f};
    verts[(3 * 4) + 2].normal    = (vec3){1.0f, 0.0f, 0.0f};
    verts[(3 * 4) + 3].normal    = (vec3){1.0f, 0.0f, 0.0f};

    // Bottom face
    verts[(4 * 4) + 0].position  = (vec3){max_x, min_y, max_z};
    verts[(4 * 4) + 1].position  = (vec3){min_x, min_y, min_z};
    verts[(4 * 4) + 2].position  = (vec3){max_x, min_y, min_z};
    verts[(4 * 4) + 3].position  = (vec3){min_x, min_y, max_z};
    verts[(4 * 4) + 0].tex_coord = (vec2){min_uvx, min_uvy};
    verts[(4 * 4) + 1].tex_coord = (vec2){max_uvx, max_uvy};
    verts[(4 * 4) + 2].tex_coord = (vec2){min_uvx, max_uvy};
    verts[(4 * 4) + 3].tex_coord = (vec2){max_uvx, min_uvy};
    verts[(4 * 4) + 0].normal    = (vec3){0.0f, -1.0f, 0.0f};
    verts[(4 * 4) + 1].normal    = (vec3){0.0f, -1.0f, 0.0f};
    verts[(4 * 4) + 2].normal    = (vec3){0.0f, -1.0f, 0.0f};
    verts[(4 * 4) + 3].normal    = (vec3){0.0f, -1.0f, 0.0f};

    // Top face
    verts[(5 * 4) + 0].position  = (vec3){min_x, max_y, max_z};
    verts[(5 * 4) + 1].position  = (vec3){max_x, max_y, min_z};
    verts[(5 * 4) + 2].position  = (vec3){min_x, max_y, min_z};
    verts[(5 * 4) + 3].position  = (vec3){max_x, max_y, max_z};
    verts[(5 * 4) + 0].tex_coord = (vec2){min_uvx, min_uvy};
    verts[(5 * 4) + 1].tex_coord = (vec2){max_uvx, max_uvy};
    verts[(5 * 4) + 2].tex_coord = (vec2){min_uvx, max_uvy};
    verts[(5 * 4) + 3].tex_coord = (vec2){max_uvx, min_uvy};
    verts[(5 * 4) + 0].normal    = (vec3){0.0f, 1.0f, 0.0f};
    verts[(5 * 4) + 1].normal    = (vec3){0.0f, 1.0f, 0.0f};
    verts[(5 * 4) + 2].normal    = (vec3){0.0f, 1.0f, 0.0f};
    verts[(5 * 4) + 3].normal    = (vec3){0.0f, 1.0f, 0.0f};

    dcopy_memory(config.vertices, verts, sizeof(vertex_3D) * config.vertex_count);

    for (u32 i = 0; i < 6; ++i)
    {
        u32 v_offset                          = i * 4;
        u32 i_offset                          = i * 6;
        ((u32 *)config.indices)[i_offset + 0] = v_offset + 0;
        ((u32 *)config.indices)[i_offset + 1] = v_offset + 1;
        ((u32 *)config.indices)[i_offset + 2] = v_offset + 2;
        ((u32 *)config.indices)[i_offset + 3] = v_offset + 0;
        ((u32 *)config.indices)[i_offset + 4] = v_offset + 3;
        ((u32 *)config.indices)[i_offset + 5] = v_offset + 1;
    }
    return config;
}

bool geometry_system_create_default_geometry()
{

    u32              num_of_objects      = INVALID_ID;
    geometry_config *default_geo_configs = nullptr;

    dstring bin_file = "../assets/meshes/cube.obj.bin";

    bool result = file_exists(&bin_file);

    if (result)
    {
        result = geometry_system_parse_bin_file(&bin_file, &num_of_objects, &default_geo_configs);
        DASSERT(result);
    }
    else
    {
        geometry_system_parse_obj(geo_sys_state_ptr->arena, "../assets/meshes/cube.obj", &num_of_objects,
                                  &default_geo_configs);
        DASSERT(num_of_objects != INVALID_ID);
        default_geo_configs[0].material = material_system_get_default_material();
        geometry_system_write_configs_to_file(&bin_file, num_of_objects, default_geo_configs);
        // there is only one thats why
    }

    DASSERT(num_of_objects != INVALID_ID);

    for (u32 i = 0; i < num_of_objects; i++)
    {
        default_geo_configs[i].type       = GEO_TYPE_3D;
        geo_sys_state_ptr->default_geo_id = geometry_system_create_geometry(&default_geo_configs[i], false);
    }

    for (u32 i = 0; i < num_of_objects; i++)
    {
        destroy_geometry_config(&default_geo_configs[i]);
    }
    dfree(default_geo_configs, num_of_objects * sizeof(geometry_config), MEM_TAG_GEOMETRY);

    return true;
}

geometry *geometry_system_get_geometry(u64 id)
{
    geometry *geo = geo_sys_state_ptr->hashtable.find(id);

    if (!geo)
    {
        DERROR("There are no geometry associated with that id. Are you sure you haven't created it yet?? Returning "
               "default geometry");
        return geometry_system_get_default_geometry();
    }
    geo->reference_count++;
    return geo;
}

geometry *geometry_system_get_geometry_by_name(dstring geometry_name)
{
    const char *name = geometry_name.c_str();
    geometry   *geo  = geo_sys_state_ptr->hashtable.find(name);
    if (!geo)
    {
        DWARN("Geometry %s not loaded yet.Load it first by calling geometry_system_create_geometry. Returning "
              "default_geometry",
              name);
        return geometry_system_get_default_geometry();
    }
    geo->reference_count++;
    return geo;
}

geometry *geometry_system_get_default_geometry()
{

    u64       default_id = geo_sys_state_ptr->default_geo_id;
    geometry *geo        = geo_sys_state_ptr->hashtable.find(default_id);
    if (!geo)
    {
        DERROR("Default geometry is not loaded yet. How is this possible?? Make sure that you have initialzed the "
               "geometry system first.");
        return nullptr;
    }
    geo->reference_count++;
    return geo;
}

geometry *geometry_system_get_default_plane()
{
    geometry *geo = geo_sys_state_ptr->hashtable.find(DEFAULT_PLANE_HANDLE);
    if (!geo)
    {
        DERROR(
            "Default plane geometry is not loaded yet. How is this possible?? Make sure that you have initialzed the "
            "geometry system first.");
        return nullptr;
    }
    geo->reference_count++;
    return geo;
}

void geometry_system_copy_config(geometry_config *dst_config, const geometry_config *src_config)
{
    if (src_config == nullptr || dst_config == nullptr)
    {
        DERROR("Provided geometry_configs were nullptr");
        return;
    }
    if (src_config->vertex_count == INVALID_ID || src_config->index_count == INVALID_ID)
    {
        DERROR("Src geometry config has invalid vertex/index count.");
        return;
    }
    if (src_config->vertices == nullptr || src_config->indices == nullptr)
    {
        DERROR("Src geometry config has vertex_count of %d and index count of %d. But one of them is nullptr.",
               src_config->vertex_count, src_config->index_count);
        return;
    }
    if (dst_config->vertex_count != INVALID_ID || dst_config->index_count != INVALID_ID || dst_config->vertices ||
        dst_config->indices)
    {
        DERROR("Dst_config already has some data in it. Should destroy the data first before calling the function");
        return;
    }
    arena *arena = geo_sys_state_ptr->arena;

    dcopy_memory(dst_config->name.string, src_config->name.string, src_config->name.str_len);
    dst_config->name.str_len = src_config->name.str_len;

    dst_config->vertex_count = src_config->vertex_count;
    dst_config->vertices =
        static_cast<vertex_3D *>(dallocate(arena, sizeof(vertex_3D) * src_config->vertex_count, MEM_TAG_GEOMETRY));
    dcopy_memory(dst_config->vertices, src_config->vertices, src_config->vertex_count * sizeof(vertex_3D));

    dst_config->index_count = src_config->index_count;
    dst_config->indices = static_cast<u32 *>(dallocate(arena, sizeof(u32) * src_config->index_count, MEM_TAG_GEOMETRY));
    dcopy_memory(dst_config->indices, src_config->indices, src_config->index_count * sizeof(u32));

    if (src_config->material)
    {
        dst_config->material = material_system_acquire_from_name(&src_config->material->name);
    }

    return;
}

static const char ascii_chars[95] = {'X', 'r', 'F', 'm', 'S', '{', 'K',  '5', 'T', 'B', 'A', '7', '>', '\'', 'j', '/',
                                     'p', '3', '=', '^', 'W', '&', '|',  'd', '}', 'C', '@', 'h', 'n', 'G',  '0', 'U',
                                     '2', '!', 'Z', 'c', 'g', 'b', 'v',  'J', 'O', 'i', 's', 'N', 'L', '#',  '(', 'a',
                                     'x', '9', 'e', 'D', 't', '~', 'R',  'M', 'H', '6', 'E', '.', 'V', 'l',  '1', 'z',
                                     'w', 'f', 'q', 'I', '<', 'y', 'Q',  'P', 'Y', 'u', 'k', 'o', '8', '4',  '"', ',',
                                     '}', ']', '_', '+', '(', ':', '\\', '`', '*', ' ', ';', '-', '$', '[',  '?'};

void get_random_string(char *out_string)
{
    u32 index;
    for (u32 i = 0; i < MAX_KEY_LENGTH - 2; i++)
    {
        index         = drandom_in_range(0, 94);
        out_string[i] = ascii_chars[index];
    }
    out_string[MAX_KEY_LENGTH - 1] = '\0';

    return;
}
//
// this will allocate and write size back, assumes the caller will call free once the data is processed

void geometry_system_parse_obj(arena *arena, const char *obj_file_full_path, u32 *num_of_objects,
                               geometry_config **geo_configs)
{
    DTRACE("parsing file %s.", obj_file_full_path);
    dclock telemetry;
    clock_start(&telemetry);

    u64 buffer_mem_requirements = -1;
    file_open_and_read(obj_file_full_path, &buffer_mem_requirements, 0, 0);
    if (buffer_mem_requirements == INVALID_ID_64)
    {
        DERROR("Failed to get size requirements for %s", obj_file_full_path);
        return;
    }
    DASSERT(buffer_mem_requirements != INVALID_ID_64);

    char *buffer = static_cast<char *>(dallocate(arena, buffer_mem_requirements + 1, MEM_TAG_GEOMETRY));
    char *start  = buffer;

    file_open_and_read(obj_file_full_path, &buffer_mem_requirements, buffer, 0);

    // TODO: cleaup this piece of shit code
    u32 objects        = string_num_of_substring_occurence(buffer, "o ");
    u32 usemtl_objects = string_num_of_substring_occurence(buffer, "usemtl");

    bool usemtl_name = true;
    if (usemtl_objects < objects)
    {
        usemtl_name = false;
    }
    objects = DMAX(objects, usemtl_objects);

    *num_of_objects = objects;

    *geo_configs =
        static_cast<geometry_config *>(dallocate(arena, sizeof(geometry_config) * objects, MEM_TAG_GEOMETRY));

    clock_update(&telemetry);
    f64 names_proccesing_start_time = telemetry.time_elapsed;
    {
        char *object_name_ptr      = buffer;
        char *next_object_name_ptr = nullptr;
        char *object_usemtl_name   = nullptr;

        auto extract_name = [](char *dest, const char *src) -> u32 {
            u32 j = 0;
            while (*src != '\n' && *src != '\r')
            {
                if (*src == ' ')
                {
                    src++;
                    continue;
                }
                dest[j++] = *src;
                src++;
            }
            return j;
        };
        u32 j = 0;
        for (u32 i = 0; i < objects; i++)
        {
            object_name_ptr = strstr(object_name_ptr, "o ");
            if (object_name_ptr == nullptr)
            {
                break;
            }
            object_name_ptr        += 2;
            (*geo_configs)[i].type  = GEO_TYPE_3D;

            next_object_name_ptr = strstr(object_name_ptr, "o ");
            if (next_object_name_ptr == nullptr)
            {
                next_object_name_ptr = buffer + buffer_mem_requirements;
            }

            if (usemtl_name)
            {
                object_usemtl_name = object_name_ptr;
                while ((object_usemtl_name = strstr(object_usemtl_name, "usemtl")) && object_usemtl_name != nullptr &&
                       object_usemtl_name < next_object_name_ptr)
                {
                    dstring material_name  = {};
                    object_usemtl_name    += 6;
                    u32 size               = extract_name(material_name.string, object_usemtl_name);
                    get_random_string((*geo_configs)[j].name.string);
                    (*geo_configs)[j].name.str_len  = MAX_KEY_LENGTH - 1;
                    (*geo_configs)[j++].material    = material_system_acquire_from_name(&material_name);
                    object_usemtl_name             += size;
                }
            }
            else
            {
                get_random_string((*geo_configs)[j++].name.string);
                //(*geo_configs)[i].material = material_system_get_default_material();
            }
        }
    }

    clock_update(&telemetry);
    f64 names_proccesing_end_time = telemetry.time_elapsed - names_proccesing_start_time;
    DTRACE("names proccesing took %fs.\n", names_proccesing_end_time);

    char *vert_ptr            = buffer;
    char  vert_substring[3]   = "v ";
    u32   vert_substring_size = 2;

    char *vert_            = buffer;
    u32   vertex_count_obj = string_num_of_substring_occurence(vert_, vert_substring) + 1;

    vert_ptr  = const_cast<char *>(string_first_string_occurence(vert_ptr, vert_substring));
    vert_ptr += vert_substring_size;

    vec3 *vert_coords = static_cast<vec3 *>(dallocate(arena, sizeof(vec3) * vertex_count_obj, MEM_TAG_GEOMETRY));

    u32 vert_processed = 0;
    clock_update(&telemetry);
    f64 vert_proccesing_start_time = telemetry.time_elapsed;

    for (u32 i = 0; i < vertex_count_obj && vert_ptr != nullptr; i++)
    {
        char *a;
        vert_coords[i].x = strtof(vert_ptr, &a);
        char *b;
        vert_coords[i].y = strtof(a, &b);
        vert_coords[i].z = strtof(b, NULL);

        vert_ptr = strstr(vert_ptr, "v ");

        // if (vert_processed % 1000 == 0)
        //{
        //     DTRACE("Verts Processed: %d Remaining: %d", vert_processed,
        //     vertex_count_obj - vert_processed);
        // }
        vert_processed++;
        if (vert_ptr != nullptr)
        {
            vert_ptr += vert_substring_size;
        }
    }
    clock_update(&telemetry);
    f64 vert_proccesing_end_time = telemetry.time_elapsed;
    f64 total_vert_time          = vert_proccesing_end_time - vert_proccesing_start_time;
    DTRACE("%d Vertices processed in %fs.\n", vert_processed, total_vert_time);

    char vert_normal[3]             = "vn";
    u32  vert_normal_substring_size = 2;

    char *norm_ptr         = buffer;
    u32   normal_count_obj = string_num_of_substring_occurence(norm_ptr, "vn");

    vec3 *normals = static_cast<vec3 *>(dallocate(arena, sizeof(vec3) * normal_count_obj, MEM_TAG_GEOMETRY));

    norm_ptr  = const_cast<char *>(string_first_string_occurence(norm_ptr, vert_normal));
    norm_ptr += vert_normal_substring_size;

    u32 normal_processed = 0;
    clock_update(&telemetry);
    f64 norm_proccesing_start_time = telemetry.time_elapsed;
    for (u32 i = 0; i < normal_count_obj; i++)
    {
        char *a;
        normals[i].x = strtof(norm_ptr, &a);
        char *b;
        normals[i].y = strtof(a, &b);
        normals[i].z = strtof(b, NULL);

        norm_ptr = strstr(norm_ptr, "vn");
        // if (normal_processed % 1000 == 0)
        //{
        //     DTRACE("normals Processed: %d Remaining: %d", normal_processed,
        //     normal_count_obj - normal_processed);
        // }
        normal_processed++;
        if (norm_ptr == nullptr)
        {
            break;
        }
        norm_ptr += vert_normal_substring_size;
    }
    clock_update(&telemetry);
    f64 norm_proccesing_end_time = telemetry.time_elapsed;
    f64 total_norm_time          = norm_proccesing_end_time - norm_proccesing_start_time;

    DTRACE("%d normals processed in %fs.\n", normal_processed, total_norm_time);

    char vert_texture[3]             = "vt";
    u32  vert_texture_substring_size = 2;

    char *tex_ptr           = buffer;
    u32   texture_count_obj = string_num_of_substring_occurence(tex_ptr, "vt");

    vec2 *textures = static_cast<vec2 *>(dallocate(arena, sizeof(vec2) * texture_count_obj, MEM_TAG_GEOMETRY));

    tex_ptr  = const_cast<char *>(string_first_string_occurence(tex_ptr, vert_texture));
    tex_ptr += vert_texture_substring_size;

    u32 texture_processed = 0;
    clock_update(&telemetry);
    f64 textrue_proccesing_start_time = telemetry.time_elapsed;
    for (u32 i = 0; i < texture_count_obj; i++)
    {
        char *a;
        textures[i].x = strtof(tex_ptr, &a);
        textures[i].y = strtof(a, NULL);

        tex_ptr = strstr(tex_ptr, "vt");
        // if (texture_processed % 1000 == 0)
        //{
        //     DTRACE("textures Processed: %d Remaining: %d", texture_processed,
        //     texture_count_obj - texture_processed);
        // }
        texture_processed++;
        if (tex_ptr == nullptr)
        {
            break;
        }
        tex_ptr += vert_texture_substring_size;
    }

    clock_update(&telemetry);
    f64 textrue_proccesing_end_time = telemetry.time_elapsed;
    f64 total_texture_time          = textrue_proccesing_end_time - textrue_proccesing_start_time;
    DTRACE("%d Texture_coords processed in %fs.\n", texture_processed, total_texture_time);

    // INFO: Experimental get all the offsets
    u32 offsets_count = string_num_of_substring_occurence(buffer, "f ");
    // one f represents 3 vertices 3 nromals 3 texture coords which define a
    // triangle. So we need to have enough space for have number of 'f ' * 9
    // indicies.Furthermore, Each object will have its max offset and min offset for vertices, textures, and normals
    // and number of vertices for that object stored in the first 7 index of its
    // array (i.e) 0 , 1 and 2 .. 6 . From 7 till vertices count onwards are the actual
    // global offsets for the object.
    u32  offsets_size = (offsets_count * 9) + (7 * objects);
    u32 *offsets      = static_cast<u32 *>(dallocate(arena, sizeof(u32) * (offsets_size), MEM_TAG_GEOMETRY));

    char *object_ptr  = const_cast<char *>(string_first_string_occurence(buffer, "o "));
    object_ptr       += 2;

    const char *usemtl_substring      = "usemtl";
    u32         usemtl_substring_size = 6;

    const char *obj_substring      = "o ";
    u32         obj_substring_size = 3;

    const char *use_string      = nullptr;
    u32         use_string_size = INVALID_ID;
    if (usemtl_name)
    {
        object_ptr       = const_cast<char *>(string_first_string_occurence(buffer, usemtl_substring));
        object_ptr      += 6;
        use_string       = usemtl_substring;
        use_string_size  = usemtl_substring_size;
    }
    else
    {
        object_ptr       = const_cast<char *>(string_first_string_occurence(buffer, obj_substring));
        object_ptr      += obj_substring_size;
        use_string       = obj_substring;
        use_string_size  = obj_substring_size;
    }

    char *indices_ptr = object_ptr;

    object_ptr = const_cast<char *>(string_first_string_occurence(object_ptr, use_string));
    if (object_ptr == nullptr)
    {
        object_ptr = buffer + buffer_mem_requirements;
    }
    else
    {
        object_ptr += use_string_size;
    }

    // move the pointer to the next object's offsets
    u32   object_indices_offset = 0;
    char *found                 = indices_ptr;
    u32  *offset_ptr_start      = offsets;
    u32  *offset_ptr_walk       = &offset_ptr_start[7];

    u32 max_vert_for_obj      = 0;
    u32 min_vert_for_obj      = INVALID_ID;
    u32 max_texture_for_obj   = 0;
    u32 min_texture_for_obj   = INVALID_ID;
    u32 max_normal_for_obj    = 0;
    u32 min_normal_for_obj    = INVALID_ID;
    u32 offsets_count_for_obj = 0;

    u32 object_processed  = 0;
    u32 offsets_processed = 0;

    clock_update(&telemetry);
    f64  offsets_proccesing_start_time = telemetry.time_elapsed;
    bool process_final_line            = false;

    while (process_final_line || indices_ptr != NULL || found != NULL)
    {
        found = strstr(found, "f ");
        if (found == NULL || found > object_ptr)
        {

            offset_ptr_start[0] = min_vert_for_obj;
            offset_ptr_start[1] = max_vert_for_obj;
            offset_ptr_start[2] = min_texture_for_obj;
            offset_ptr_start[3] = max_texture_for_obj;
            offset_ptr_start[4] = min_normal_for_obj;
            offset_ptr_start[5] = max_normal_for_obj;
            offset_ptr_start[6] = offsets_count_for_obj;

            offsets_processed += offsets_count_for_obj;

            offset_ptr_start = offset_ptr_walk;
            offset_ptr_walk  = &offset_ptr_start[7];

            max_vert_for_obj      = 0;
            min_vert_for_obj      = INVALID_ID;
            max_texture_for_obj   = 0;
            min_texture_for_obj   = INVALID_ID;
            max_normal_for_obj    = 0;
            min_normal_for_obj    = INVALID_ID;
            offsets_count_for_obj = 0;

            object_ptr = const_cast<char *>(string_first_string_occurence(object_ptr, use_string));
            if (object_ptr == nullptr)
            {
                object_ptr = buffer + buffer_mem_requirements;
            }
            else
            {
                object_ptr += use_string_size;
            }
            object_processed++;
            if (found == NULL)
            {
                break;
            }
        }
        char *f = found;

        u32   j    = 0;
        u32   k    = 0;
        char *ptr2 = f + 2;
        char *a;

        while (k < 9 && *ptr2 != '\n')
        {
            offset_ptr_walk[k++] = strtol(ptr2, &a, 10);

            ptr2  = a;
            *ptr2 = ' ';
            a     = NULL;
        }

        for (u32 j = 0; j < 9; j += 3)
        {
            max_vert_for_obj    = DMAX(max_vert_for_obj, offset_ptr_walk[j]);
            min_vert_for_obj    = DMIN(min_vert_for_obj, offset_ptr_walk[j]);
            max_texture_for_obj = DMAX(max_texture_for_obj, offset_ptr_walk[j + 1]);
            min_texture_for_obj = DMIN(min_texture_for_obj, offset_ptr_walk[j + 1]);
            max_normal_for_obj  = DMAX(max_normal_for_obj, offset_ptr_walk[j + 2]);
            min_normal_for_obj  = DMIN(min_normal_for_obj, offset_ptr_walk[j + 2]);
        }

        offsets_count_for_obj += 9;
        offset_ptr_walk       += 9;

        if (found != NULL)
        {
            found += 2;
        }
        else
        {
            process_final_line = true;
        }
    }

    clock_update(&telemetry);
    f64 offsets_proccesing_end_time = telemetry.time_elapsed;
    f64 total_offsets_time          = offsets_proccesing_end_time - offsets_proccesing_start_time;
    DTRACE("%d offsets processed in %fs for %d objects .\n", offsets_processed, total_offsets_time, object_processed);

    clock_update(&telemetry);
    f64 object_proccesing_start_time = telemetry.time_elapsed;

    u32 size_till_now = 0;
    for (u32 object = 0; object < objects; object++)
    {
        u32 *offset_ptr_walk = &offsets[size_till_now + 7];
        u32  count           = offsets[(size_till_now) + 6];
        u32  vert_min        = offsets[size_till_now + 0];
        u32  vert_max        = offsets[size_till_now + 1];
        u32  tex_min         = offsets[size_till_now + 2];
        u32  norm_min        = offsets[size_till_now + 4];

        size_till_now            += (count + 7);
        u32 vertex_count_per_obj  = vert_max - vert_min + 1;

        (*geo_configs)[object].vertices =
            static_cast<vertex_3D *>(dallocate(arena, sizeof(vertex_3D) * (count / 3), MEM_TAG_GEOMETRY));
        (*geo_configs)[object].vertex_count = count / 3;
        (*geo_configs)[object].indices =
            static_cast<u32 *>(dallocate(arena, sizeof(u32) * (count / 3), MEM_TAG_GEOMETRY));
        (*geo_configs)[object].index_count = count / 3;

        vertex_3D *vertices = static_cast<vertex_3D *>((*geo_configs)[object].vertices);

        u32 index_ind = 0;
        for (u32 j = 0; j <= count - 3; j += 3)
        {
            u32 vert_ind = offset_ptr_walk[j] - 1;
            u32 tex_ind  = offset_ptr_walk[j + 1] - 1;
            u32 norm_ind = offset_ptr_walk[j + 2] - 1;

            vertices[index_ind].position              = vert_coords[vert_ind];
            vertices[index_ind].tex_coord             = textures[tex_ind];
            vertices[index_ind].normal                = normals[norm_ind];
            (*geo_configs)[object].indices[index_ind] = index_ind;
            index_ind++;
        }
        DASSERT(index_ind == (*geo_configs)[object].index_count);
    }

    clock_update(&telemetry);
    f64 object_proccesing_end_time = telemetry.time_elapsed;
    f64 total_object_time          = object_proccesing_end_time - object_proccesing_start_time;
    DTRACE("objects proccesing took %f", total_object_time);

    dfree(textures, sizeof(vec3) * texture_count_obj, MEM_TAG_UNKNOWN);
    dfree(normals, sizeof(vec3) * normal_count_obj, MEM_TAG_UNKNOWN);
    dfree(vert_coords, sizeof(vec3) * vertex_count_obj, MEM_TAG_UNKNOWN);
    dfree(buffer, buffer_mem_requirements, MEM_TAG_GEOMETRY);

    clock_update(&telemetry);
    f64 elapsed = telemetry.time_elapsed;

    DTRACE("Parse obj function took %fs : wtf so slow", elapsed);
}

void geometry_system_get_geometries_from_file(const char *obj_file_name, const char *mtl_file_name, geometry ***geos,
                                              u32 *geometry_count)
{
    DASSERT(obj_file_name);
    DASSERT(mtl_file_name);

    const char *file_prefix     = "../assets/meshes/";
    const char *file_mtl_prefix = "../assets/materials/";

    char obj_file_full_path[GEOMETRY_NAME_MAX_LENGTH];
    string_copy_format(obj_file_full_path, "%s%s", 0, file_prefix, obj_file_name);

    dstring file_mtl_full_path;
    string_copy_format(file_mtl_full_path.string, "%s%s", 0, file_mtl_prefix, mtl_file_name);
    material_system_parse_mtl_file(&file_mtl_full_path);

    u32              objects     = INVALID_ID;
    geometry_config *geo_configs = nullptr;

    dstring     bin_file_full_path;
    const char *suffix = ".bin";
    string_copy_format(bin_file_full_path.string, "%s%s", 0, obj_file_full_path, suffix);

    bool result = file_exists(&bin_file_full_path);

    arena *temp_arena = nullptr;

    if (result)
    {
        geometry_system_parse_bin_file(&bin_file_full_path, &objects, &geo_configs);
    }
    else
    {
        temp_arena = arena_get_arena();
        geometry_system_parse_obj(temp_arena, obj_file_full_path, &objects, &geo_configs);
        geometry_system_write_configs_to_file(&bin_file_full_path, objects, geo_configs);
    }

    DASSERT(objects != INVALID_ID);
    arena *arena = geo_sys_state_ptr->arena;
    *geos        = static_cast<geometry **>(dallocate(arena, sizeof(geometry *) * objects, MEM_TAG_GEOMETRY));

    // HACK:
    vec3 scale = {0.5, 0.5, 0.5};

    for (u32 i = 0; i < objects; i++)
    {

        scale_geometries(&geo_configs[i], scale);
        u64 id     = geometry_system_create_geometry(&geo_configs[i], false);
        (*geos)[i] = geometry_system_get_geometry(id);
    }
    *geometry_count = objects;

    if (temp_arena)
    {
        arena_free_arena(temp_arena);
        temp_arena = nullptr;
    }

    return;
}

bool destroy_geometry_config(geometry_config *config)
{
    // TODO: release material ?
    config->material     = nullptr;
    config->vertices     = nullptr;
    config->indices      = nullptr;
    config->vertex_count = INVALID_ID;
    config->index_count  = INVALID_ID;
    dzero_memory(config->name.string, GEOMETRY_NAME_MAX_LENGTH);
    return true;
}

bool geometry_system_write_configs_to_file(dstring *file_full_path, u32 geometry_config_count, geometry_config *configs)
{
    DASSERT(file_full_path);
    DASSERT(geometry_config_count);
    DASSERT(configs);

    std::fstream f;
    bool         result = file_open(file_full_path->c_str(), &f, true, true);
    DASSERT_MSG(result, "Failed to open file");

    char new_line = '\n';

    // INFO: file header
    file_write(&f, "geometry_config_count:", string_length("geometry_config_count:"));
    file_write(&f, reinterpret_cast<const char *>(&geometry_config_count), sizeof(u32));
    file_write(&f, reinterpret_cast<const char *>(&new_line), 1);

    u32 size = 0;
    for (u32 i = 0; i < geometry_config_count; i++)
    {
        file_write(&f, "geo_type:", string_length("geo_type:"));
        if (configs[i].type == GEO_TYPE_3D)
        {
            size = string_length("type_3D");
            file_write(&f, reinterpret_cast<const char *>(&size), sizeof(u32));
            file_write(&f, "type_3D", string_length("type_3D"));
            file_write(&f, reinterpret_cast<const char *>(&new_line), 1);
            size = 0;
        }
        else if (configs[i].type == GEO_TYPE_2D)
        {
            size = string_length("type_2D");
            file_write(&f, reinterpret_cast<const char *>(&size), sizeof(u32));
            file_write(&f, "type_2D", string_length("type_2D"));
            file_write(&f, reinterpret_cast<const char *>(&new_line), 1);
            size = 0;
        }
        else
        {
            DERROR("Unknown geometry type: %d", configs[i].type);
        }
        size = configs[i].name.str_len;
        if (size)
        {
            file_write(&f, "name:", string_length("name:"));
            file_write(&f, reinterpret_cast<const char *>(&size), sizeof(u32));
            file_write(&f, configs[i].name.c_str(), size);
            file_write(&f, reinterpret_cast<const char *>(&new_line), 1);
        }

        // NOTE: thi&s is the material name and not the material itself.
        if (configs[i].material)
        {
            size = configs[i].material->name.str_len;
            file_write(&f, "material:", string_length("material:"));
            file_write(&f, reinterpret_cast<const char *>(&size), sizeof(u32));
            file_write(&f, configs[i].material->name.c_str(), size);
            file_write(&f, reinterpret_cast<const char *>(&new_line), 1);
        }

        file_write(&f, "vertex_count:", string_length("vertex_count:"));
        file_write(&f, reinterpret_cast<const char *>(&configs[i].vertex_count), sizeof(u32));
        file_write(&f, reinterpret_cast<const char *>(&new_line), 1);

        file_write(&f, "vertices:", string_length("vertices:"));
        file_write(&f, reinterpret_cast<const char *>(configs[i].vertices),
                   configs[i].vertex_count * sizeof(vertex_3D));
        file_write(&f, reinterpret_cast<const char *>(&new_line), 1);

        file_write(&f, "index_count:", string_length("index_count:"));
        file_write(&f, reinterpret_cast<const char *>(&configs[i].index_count), sizeof(u32));
        file_write(&f, reinterpret_cast<const char *>(&new_line), 1);

        file_write(&f, "indices:", string_length("indices:"));
        file_write(&f, reinterpret_cast<const char *>(configs[i].indices), configs[i].index_count * sizeof(u32));
        file_write(&f, reinterpret_cast<const char *>(&new_line), 1);
    }
    file_close(&f);
    return true;
}
// NOTE: count and configs should be nullptr because the function will allocate it dynamically based on the bin file
// header
bool geometry_system_parse_bin_file(dstring *file_full_path, u32 *geometry_config_count, geometry_config **configs)
{
    DASSERT(file_full_path);

    u64 buff_size = INVALID_ID_64;
    file_open_and_read(file_full_path->c_str(), &buff_size, 0, 1);
    arena *arena = geo_sys_state_ptr->arena;

    char *buffer = static_cast<char *>(dallocate(arena, buff_size + 1, MEM_TAG_GEOMETRY));
    bool  result = file_open_and_read(file_full_path->c_str(), &buff_size, buffer, 1);
    DASSERT(result);

    buffer[buff_size] = EOF;
    const char *eof   = buffer + buff_size;

    auto go_to_colon = [](const char *line) -> const char * {
        u32 index = 0;
        while (line[index] != ':')
        {
            if (line[index] == '\n' && line[index] == '\r' && line[index] == '\0')
            {
                return nullptr;
            }
            index++;
        }
        return line + index + 1;
    };

    auto extract_identifier = [](const char *line, char *dst) {
        u32 index = 0;
        u32 j     = 0;

        while (line[index] != ':' && line[index] != '\0' && line[index] != '\n' && line[index] != '\r')
        {
            if (line[index] == ' ')
            {
                index++;
                continue;
            }
            dst[j++] = line[index++];
        }
        dst[j++] = '\0';
    };

    const char *ptr = buffer;
    dstring     identifier;
    u32         index = 0;

    while (ptr < eof && ptr != nullptr)
    {
        extract_identifier(ptr, identifier.string);
        ptr = go_to_colon(ptr);
        if (!ptr)
        {
            break;
        }

        if (string_compare(identifier.c_str(), "geometry_config_count"))
        {
            u32 config_count;
            dcopy_memory(&config_count, ptr, sizeof(u32));
            *geometry_config_count = config_count;
            (*configs)             = static_cast<geometry_config *>(
                dallocate(arena, sizeof(geometry_config) * config_count, MEM_TAG_GEOMETRY));
            ptr += sizeof(u32) + 1;
        }
        else if (string_compare(identifier.c_str(), "geo_type"))
        {
            u32 name_len;
            dcopy_memory(&name_len, ptr, sizeof(u32));
            ptr += sizeof(u32);
            dstring name;
            string_ncopy(name.string, ptr, name_len);

            if (string_compare(name.c_str(), "type_3D"))
            {
                (*configs)[index].type = GEO_TYPE_3D;
            }
            else if (string_compare(name.c_str(), "type_2D"))
            {
                (*configs)[index].type = GEO_TYPE_3D;
            }
            else
            {
                DERROR("Unknown geo_type_identifier %s", name.c_str());
                return false;
            }
            ptr += name_len + 1;
        }
        else if (string_compare(identifier.c_str(), "name"))
        {
            u32 name_len;
            dcopy_memory(&name_len, ptr, sizeof(u32));
            ptr           += sizeof(u32);
            dstring &name  = (*configs)[index].name;
            string_ncopy(name.string, ptr, name_len);
            ptr += name_len + 1;
        }
        else if (string_compare(identifier.c_str(), "material"))
        {
            u32 name_len;
            dcopy_memory(&name_len, ptr, sizeof(u32));
            ptr += sizeof(u32);

            dstring mat_name;
            string_ncopy(mat_name.string, ptr, name_len);
            mat_name.str_len  = name_len;
            ptr              += name_len + 1;

            (*configs)[index].material = material_system_acquire_from_name(&mat_name);
        }
        else if (string_compare(identifier.c_str(), "vertex_count"))
        {
            u32 vertex_count;
            dcopy_memory(&vertex_count, ptr, sizeof(u32));

            (*configs)[index].vertex_count = vertex_count;

            u32 size                    = sizeof(vertex_3D) * vertex_count;
            (*configs)[index].vertices  = static_cast<vertex_3D *>(dallocate(arena, size, MEM_TAG_GEOMETRY));
            ptr                        += sizeof(u32) + 1;
        }
        else if (string_compare(identifier.c_str(), "vertices"))
        {
            void *dst  = (*configs)[index].vertices;
            u32   size = sizeof(vertex_3D) * (*configs)[index].vertex_count;
            dcopy_memory(dst, ptr, size);
            ptr += size + 1;
        }
        else if (string_compare(identifier.c_str(), "index_count"))
        {
            u32 index_count;
            dcopy_memory(&index_count, ptr, sizeof(u32));

            (*configs)[index].index_count = index_count;

            u32 size                   = sizeof(u32) * index_count;
            (*configs)[index].indices  = static_cast<u32 *>(dallocate(arena, size, MEM_TAG_GEOMETRY));
            ptr                       += sizeof(u32) + 1;
        }
        else if (string_compare(identifier.c_str(), "indices"))
        {
            void *dst  = (*configs)[index].indices;
            u32   size = sizeof(u32) * (*configs)[index].index_count;
            dcopy_memory(dst, ptr, size);
            // flush the config
            {
                index++;
            }
            ptr += size + 1;
        }
    }

    dfree(buffer, buff_size + 1, MEM_TAG_GEOMETRY);

    return true;
}

static void calculate_tangents(geometry_config *config)
{
    DASSERT(config);

    u32 index_count = config->index_count;
    DASSERT(index_count);
    vertex_3D *vertices = static_cast<vertex_3D *>(config->vertices);
    for (u32 i = 0; i < index_count; i += 3)
    {
        u32 i0 = config->indices[i + 0];
        u32 i1 = config->indices[i + 1];
        u32 i2 = config->indices[i + 2];

        vec3 edge1 = vertices[i1].position - vertices[i0].position;
        vec3 edge2 = vertices[i2].position - vertices[i0].position;

        f32 deltaU1 = vertices[i1].tex_coord.x - vertices[i0].tex_coord.x;
        f32 deltaV1 = vertices[i1].tex_coord.y - vertices[i0].tex_coord.y;

        f32 deltaU2 = vertices[i2].tex_coord.x - vertices[i0].tex_coord.x;
        f32 deltaV2 = vertices[i2].tex_coord.y - vertices[i0].tex_coord.y;

        f32 dividend = (deltaU1 * deltaV2 - deltaU2 * deltaV1);
        f32 fc       = 1.0f / dividend;

        vec3 tangent =
            (vec3){(fc * (deltaV2 * edge1.x - deltaV1 * edge2.x)), (fc * (deltaV2 * edge1.y - deltaV1 * edge2.y)),
                   (fc * (deltaV2 * edge1.z - deltaV1 * edge2.z))};

        tangent = vec3_normalized(tangent);

        f32 sx = deltaU1, sy = deltaU2;
        f32 tx = deltaV1, ty = deltaV2;
        f32 handedness = ((tx * sy - ty * sx) < 0.0f) ? -1.0f : 1.0f;

        vec4 t4 = {tangent.x, tangent.y, tangent.z, handedness};

        vertices[i0].tangent = t4;
        vertices[i1].tangent = t4;
        vertices[i2].tangent = t4;
    }
}

bool geometry_system_generate_text_geometry(dstring *text, vec2 position, f32 font_size)
{
    DASSERT(text);
    u32              length = INVALID_ID;
    font_glyph_data *glyphs = texture_system_get_glyph_table(&length);
    DASSERT(glyphs);

    u32  strlen = text->str_len;
    u32  hash   = 0;
    vec2 pos    = position;

    vertex_2D q0;
    vertex_2D q1;
    vertex_2D q2;
    vertex_2D q3;

    vertex_2D *vertices = static_cast<vertex_2D *>(geo_sys_state_ptr->font_geometry_config.vertices);
    u32       *indices  = geo_sys_state_ptr->font_geometry_config.indices;

    // this is the ptr to the next starting block
    u32 vert_ind  = geo_sys_state_ptr->vertex_offset_ind;
    // this is the ptr to the next starting block
    u32 index_ind = geo_sys_state_ptr->font_geometry_config.index_count;

    u32 index_offset = geo_sys_state_ptr->index_offset_ind;

    for (u32 i = 0; i < strlen; i++)
    {
        char ch = (*text)[i];
        if (ch == '\0')
            break;
        hash = ch - 32;

        float u0 = glyphs[hash].x0 / (float)512;
        float v0 = glyphs[hash].y0 / (float)512;
        float u1 = glyphs[hash].x1 / (float)512;
        float v1 = glyphs[hash].y1 / (float)512;

        q0.tex_coord = {u0, v0}; // Top-left
        q1.tex_coord = {u1, v0}; // Top-right
        q2.tex_coord = {u0, v1}; // Bottom-left
        q3.tex_coord = {u1, v1}; // Bottom-right

        q0.position = {pos.x, pos.y};
        q1.position = {pos.x + font_size, pos.y};
        q2.position = {pos.x, pos.y + font_size};
        q3.position = {pos.x + font_size, pos.y + font_size};

        vertices[vert_ind++] = q0;
        vertices[vert_ind++] = q2;
        vertices[vert_ind++] = q1;

        vertices[vert_ind++] = q1;
        vertices[vert_ind++] = q2;
        vertices[vert_ind++] = q3;

        pos.x += font_size;

        indices[index_ind + 0] = index_offset + 0;
        indices[index_ind + 1] = index_offset + 1;
        indices[index_ind + 2] = index_offset + 2;
        indices[index_ind + 3] = index_offset + 3;
        indices[index_ind + 4] = index_offset + 4;
        indices[index_ind + 5] = index_offset + 5;

        // num of indices per quad i.e 2 triangles
        index_ind    += 6;
        // advacnce the index offset by 4 so that the next quad has the proper offset;
        index_offset += 6;
    }

    geo_sys_state_ptr->vertex_offset_ind                 += index_offset;
    geo_sys_state_ptr->index_offset_ind                  += index_offset;
    geo_sys_state_ptr->font_geometry_config.vertex_count += index_offset;
    geo_sys_state_ptr->font_geometry_config.index_count  += index_offset;

    return true;
}

u64 geometry_system_flush_text_geometries()
{
    geo_sys_state_ptr->font_geometry_config.type = GEO_TYPE_2D;

    u64 id = geo_sys_state_ptr->font_id;

    if (id == INVALID_ID_64)
    {
        id                         = geometry_system_create_geometry(&geo_sys_state_ptr->font_geometry_config, false);
        geo_sys_state_ptr->font_id = id;
    }
    else
    {
        geometry_system_update_geometry(&geo_sys_state_ptr->font_geometry_config, id);
    }
    geo_sys_state_ptr->index_offset_ind                  = 0;
    geo_sys_state_ptr->vertex_offset_ind                 = 0;
    geo_sys_state_ptr->font_geometry_config.vertex_count = 0;
    geo_sys_state_ptr->font_geometry_config.index_count  = 0;

    return id;
}
