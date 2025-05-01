#include "geometry_system.hpp"
#include "containers/dhashtable.hpp"
#include "renderer/vulkan/vulkan_backend.hpp"
#include "resources/material_system.hpp"
#include "resources/resource_types.hpp"

struct geometry_system_state
{
    darray<dstring>      loaded_geometry;
    dhashtable<geometry> hashtable;
};

static geometry_system_state *geo_sys_state_ptr;
bool                          geometry_system_create_default_geometry();

bool geometry_system_initialize(u64 *geometry_system_mem_requirements, void *state)
{
    *geometry_system_mem_requirements = sizeof(geometry_system_state);
    if (!state)
    {
        return true;
    }
    geo_sys_state_ptr = (geometry_system_state *)state;

    {
        geo_sys_state_ptr->hashtable.table    = dallocate(sizeof(geometry) * MAX_GEOMETRIES_LOADED, MEM_TAG_DHASHTABLE);
        geo_sys_state_ptr->hashtable.capacity = sizeof(geometry) * MAX_GEOMETRIES_LOADED;
        geo_sys_state_ptr->hashtable.max_length            = MAX_GEOMETRIES_LOADED;
        geo_sys_state_ptr->hashtable.element_size          = sizeof(geometry);
        geo_sys_state_ptr->hashtable.num_elements_in_table = 0;
    }
    {
        geo_sys_state_ptr->loaded_geometry = darray<dstring>();
    }
    geometry_system_create_default_geometry();

    return true;
}

bool geometry_system_shutdowm(void *state)
{
    u32       loaded_geometry_count = geo_sys_state_ptr->loaded_geometry.size();
    geometry *geo                   = nullptr;
    bool      result                = false;
    for (int i = 0; i < loaded_geometry_count; i++)
    {
        const char *geo_name = geo_sys_state_ptr->loaded_geometry[i].c_str();
        geo                  = geo_sys_state_ptr->hashtable.find(geo_name);
        result               = vulkan_destroy_geometry(geo);
        if (!result)
        {
            DERROR("Failed to destroy %s geometry", geo_name);
        }
    }
    geo_sys_state_ptr->hashtable.clear();
    geo_sys_state_ptr->hashtable.~dhashtable();
    geo_sys_state_ptr->loaded_geometry.~darray();
    geo_sys_state_ptr = 0;
    return true;
}

bool geometry_system_create_geometry(geometry_config *config)
{

    geometry geo{};
    u32      vertices_count = config->vertices->size();
    u32      indices_count  = config->indices->size();
    bool     result = vulkan_create_geometry(&geo, vertices_count, (vertex *)config->vertices->data, indices_count,
                                             (u32 *)config->indices->data);

    if (!result)
    {
        DERROR("Couldnt create geometry %s.", config->name);
        return false;
    }

    geo_sys_state_ptr->hashtable.insert(config->name, geo);
    geo_sys_state_ptr->loaded_geometry.push_back(config->name);
    return true;
}

bool geometry_system_create_default_geometry()
{
    geometry_config default_config{};

    darray<vertex> verts(32);

    u32 width  = 1.0f;
    u32 height = 1.0f;
    u32 depth  = 1.0f;
    u32 tile_x = 1.0f;
    u32 tile_y = 1.0f;

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

    verts[(0 * 4) + 0].position  = (vec3){min_x, min_y, max_z};
    verts[(0 * 4) + 1].position  = (vec3){max_x, max_y, max_z};
    verts[(0 * 4) + 2].position  = (vec3){min_x, max_y, max_z};
    verts[(0 * 4) + 3].position  = (vec3){max_x, min_y, max_z};
    verts[(0 * 4) + 0].tex_coord = (vec2){min_uvx, min_uvy};
    verts[(0 * 4) + 1].tex_coord = (vec2){max_uvx, max_uvy};
    verts[(0 * 4) + 2].tex_coord = (vec2){min_uvx, max_uvy};
    verts[(0 * 4) + 3].tex_coord = (vec2){max_uvx, min_uvy};
    verts[(0 * 4) + 0].normal    = (vec3){0, 0, -1};
    verts[(0 * 4) + 1].normal    = (vec3){0, 0, -1};
    verts[(0 * 4) + 2].normal    = (vec3){0, 0, -1};
    verts[(0 * 4) + 3].normal    = (vec3){0, 0, -1};

    // Back face
    verts[(1 * 4) + 0].position  = (vec3){max_x, min_y, min_z};
    verts[(1 * 4) + 1].position  = (vec3){min_x, max_y, min_z};
    verts[(1 * 4) + 2].position  = (vec3){max_x, max_y, min_z};
    verts[(1 * 4) + 3].position  = (vec3){min_x, min_y, min_z};
    verts[(1 * 4) + 0].tex_coord = (vec2){min_uvx, min_uvy};
    verts[(1 * 4) + 1].tex_coord = (vec2){max_uvx, max_uvy};
    verts[(1 * 4) + 2].tex_coord = (vec2){min_uvx, max_uvy};
    verts[(1 * 4) + 3].tex_coord = (vec2){max_uvx, min_uvy};
    verts[(1 * 4) + 0].normal    = (vec3){0, 0, 1};
    verts[(1 * 4) + 1].normal    = (vec3){0, 0, 1};
    verts[(1 * 4) + 2].normal    = (vec3){0, 0, 1};
    verts[(1 * 4) + 3].normal    = (vec3){0, 0, 1};

    // Left
    verts[(2 * 4) + 0].position  = (vec3){min_x, min_y, min_z};
    verts[(2 * 4) + 1].position  = (vec3){min_x, max_y, max_z};
    verts[(2 * 4) + 2].position  = (vec3){min_x, max_y, min_z};
    verts[(2 * 4) + 3].position  = (vec3){min_x, min_y, max_z};
    verts[(2 * 4) + 0].tex_coord = (vec2){min_uvx, min_uvy};
    verts[(2 * 4) + 1].tex_coord = (vec2){max_uvx, max_uvy};
    verts[(2 * 4) + 2].tex_coord = (vec2){min_uvx, max_uvy};
    verts[(2 * 4) + 3].tex_coord = (vec2){max_uvx, min_uvy};
    verts[(2 * 4) + 0].normal    = (vec3){-1, 0, 0};
    verts[(2 * 4) + 1].normal    = (vec3){-1, 0, 0};
    verts[(2 * 4) + 2].normal    = (vec3){-1, 0, 0};
    verts[(2 * 4) + 3].normal    = (vec3){-1, 0, 0};

    // Right face
    verts[(3 * 4) + 0].position  = (vec3){max_x, min_y, max_z};
    verts[(3 * 4) + 1].position  = (vec3){max_x, max_y, min_z};
    verts[(3 * 4) + 2].position  = (vec3){max_x, max_y, max_z};
    verts[(3 * 4) + 3].position  = (vec3){max_x, min_y, min_z};
    verts[(3 * 4) + 0].tex_coord = (vec2){min_uvx, min_uvy};
    verts[(3 * 4) + 1].tex_coord = (vec2){max_uvx, max_uvy};
    verts[(3 * 4) + 2].tex_coord = (vec2){min_uvx, max_uvy};
    verts[(3 * 4) + 3].tex_coord = (vec2){max_uvx, min_uvy};
    verts[(3 * 4) + 0].normal    = (vec3){0, 1, 0};
    verts[(3 * 4) + 1].normal    = (vec3){0, 1, 0};
    verts[(3 * 4) + 2].normal    = (vec3){0, 1, 0};
    verts[(3 * 4) + 3].normal    = (vec3){0, 1, 0};

    // Bottom face
    verts[(4 * 4) + 0].position  = (vec3){max_x, min_y, max_z};
    verts[(4 * 4) + 1].position  = (vec3){min_x, min_y, min_z};
    verts[(4 * 4) + 2].position  = (vec3){max_x, min_y, min_z};
    verts[(4 * 4) + 3].position  = (vec3){min_x, min_y, max_z};
    verts[(4 * 4) + 0].tex_coord = (vec2){min_uvx, min_uvy};
    verts[(4 * 4) + 1].tex_coord = (vec2){max_uvx, max_uvy};
    verts[(4 * 4) + 2].tex_coord = (vec2){min_uvx, max_uvy};
    verts[(4 * 4) + 3].tex_coord = (vec2){max_uvx, min_uvy};
    verts[(4 * 4) + 0].normal    = (vec3){0, -1, 0};
    verts[(4 * 4) + 1].normal    = (vec3){0, -1, 0};
    verts[(4 * 4) + 2].normal    = (vec3){0, -1, 0};
    verts[(4 * 4) + 3].normal    = (vec3){0, -1, 0};

    // Top face
    verts[(5 * 4) + 0].position  = (vec3){min_x, max_y, max_z};
    verts[(5 * 4) + 1].position  = (vec3){max_x, max_y, min_z};
    verts[(5 * 4) + 2].position  = (vec3){min_x, max_y, min_z};
    verts[(5 * 4) + 3].position  = (vec3){max_x, max_y, max_z};
    verts[(5 * 4) + 0].tex_coord = (vec2){min_uvx, min_uvy};
    verts[(5 * 4) + 1].tex_coord = (vec2){max_uvx, max_uvy};
    verts[(5 * 4) + 2].tex_coord = (vec2){min_uvx, max_uvy};
    verts[(5 * 4) + 3].tex_coord = (vec2){max_uvx, min_uvy};
    verts[(5 * 4) + 0].normal    = (vec3){0, 1, 0};
    verts[(5 * 4) + 1].normal    = (vec3){0, 1, 0};
    verts[(5 * 4) + 2].normal    = (vec3){0, 1, 0};
    verts[(5 * 4) + 3].normal    = (vec3){0, 1, 0};

    darray<u32> indices(36);

    for (u32 i = 0; i < 6; ++i)
    {
        u32 v_offset          = i * 4;
        u32 i_offset          = i * 6;
        indices[i_offset + 0] = v_offset + 0;
        indices[i_offset + 1] = v_offset + 1;
        indices[i_offset + 2] = v_offset + 2;
        indices[i_offset + 3] = v_offset + 0;
        indices[i_offset + 4] = v_offset + 3;
        indices[i_offset + 5] = v_offset + 1;
    }

    const char *name = DEFAULT_GEOMETRY_HANDLE;
    u32         len  = strlen(name);
    dcopy_memory(default_config.name, (void *)name, len);

    default_config.indices  = &indices;
    default_config.vertices = &verts;
    default_config.material = material_system_get_default_material();

    geometry_system_create_geometry(&default_config);

    return true;
}

geometry *geometry_system_get_geometry(geometry_config *config)
{
    geometry *geo = geo_sys_state_ptr->hashtable.find(config->name);
    if (!geo)
    {
        DTRACE("Geometry %s not loaded yet. Loading it...", config->name);
        bool result = geometry_system_create_geometry(config);
        if (!result)
        {
            DWARN("Couldnt load %s geometry. returning default_geometry", config->name);
            return geometry_system_get_default_geometry();
        }
        DTRACE("Geometry %s loaded.");
    }
    geo = geo_sys_state_ptr->hashtable.find(config->name);
    geo->reference_count++;
    return geo;
}
geometry *geometry_system_get_geometry_by_name(dstring geometry_name)
{
    const char *name = geometry_name.c_str();
    geometry   *geo  = geo_sys_state_ptr->hashtable.find(name);
    if (!geo)
    {
        DWARN("Geometry %s not loaded yet.Load it first by calling geometry_system_get_geometry. Returning "
              "default_geometry",
              name);
        return geometry_system_get_default_geometry();
    }
    geo = geo_sys_state_ptr->hashtable.find(name);
    geo->reference_count++;
    return geo;
}

geometry *geometry_system_get_default_geometry()
{
    geometry *geo = geo_sys_state_ptr->hashtable.find(DEFAULT_GEOMETRY_HANDLE);
    if (!geo)
    {
        DERROR("Default geometry is not loaded yet. How is this possible?? Make sure that you have initialzed the "
               "geometry system first.");
        return nullptr;
    }
    geo->reference_count++;
    return geo;
}
