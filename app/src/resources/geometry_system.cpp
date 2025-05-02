#include "geometry_system.hpp"
#include "containers/dhashtable.hpp"
#include "core/dmemory.hpp"
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
    u32      vertices_count = config->vertex_count;
    u32      indices_count  = config->index_count;
    bool     result = vulkan_create_geometry(&geo, vertices_count, config->vertices, indices_count, config->indices);
    geo.name        = config->name;
    geo.reference_count = 0;
    geo.material        = config->material;

    if (!result)
    {
        DERROR("Couldnt create geometry %s.", config->name);
        return false;
    }

    geo_sys_state_ptr->hashtable.insert(config->name, geo);
    geo_sys_state_ptr->loaded_geometry.push_back(config->name);
    return true;
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

    geometry_config config;

    config.vertex_count = x_segment_count * y_segment_count * 4;
    config.vertices     = (vertex *)dallocate(sizeof(vertex) * config.vertex_count, MEM_TAG_RENDERER);
    config.index_count  = x_segment_count * y_segment_count * 6;
    config.indices      = (u32 *)dallocate(sizeof(u32) * config.index_count, MEM_TAG_RENDERER);

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
            f32 min_y   = (y * seg_height) - half_height;
            f32 max_x   = min_x + seg_width;
            f32 max_y   = min_y + seg_height;
            f32 min_uvx = (x / (f32)x_segment_count) * tile_x;
            f32 min_uvy = (y / (f32)y_segment_count) * tile_y;
            f32 max_uvx = ((x + 1) / (f32)x_segment_count) * tile_x;
            f32 max_uvy = ((y + 1) / (f32)y_segment_count) * tile_y;

            u32     v_offset = ((y * x_segment_count) + x) * 4;
            vertex *v0       = &((vertex *)config.vertices)[v_offset + 0];
            vertex *v1       = &((vertex *)config.vertices)[v_offset + 1];
            vertex *v2       = &((vertex *)config.vertices)[v_offset + 2];
            vertex *v3       = &((vertex *)config.vertices)[v_offset + 3];

            v0->position.x  = min_x;
            v0->position.y  = min_y;
            v0->tex_coord.x = min_uvx;
            v0->tex_coord.y = min_uvy;

            v1->position.x  = max_x;
            v1->position.y  = max_y;
            v1->tex_coord.x = max_uvx;
            v1->tex_coord.y = max_uvy;

            v2->position.x  = min_x;
            v2->position.y  = max_y;
            v2->tex_coord.x = min_uvx;
            v2->tex_coord.y = max_uvy;

            v3->position.x  = max_x;
            v3->position.y  = min_y;
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

    string_ncopy(config.name, name, GEOMETRY_NAME_MAX_LENGTH);
    config.material = material_system_get_default_material();

    return config;
}

bool geometry_system_create_default_geometry()
{
    geometry_config default_config{};

    default_config.vertex_count = 32;
    default_config.vertices     = (vertex *)dallocate(sizeof(vertex) * 32, MEM_TAG_RENDERER);

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

    default_config.vertices[(0 * 4) + 0].position  = (vec3){min_x, min_y, max_z};
    default_config.vertices[(0 * 4) + 1].position  = (vec3){max_x, max_y, max_z};
    default_config.vertices[(0 * 4) + 2].position  = (vec3){min_x, max_y, max_z};
    default_config.vertices[(0 * 4) + 3].position  = (vec3){max_x, min_y, max_z};
    default_config.vertices[(0 * 4) + 0].tex_coord = (vec2){min_uvx, min_uvy};
    default_config.vertices[(0 * 4) + 1].tex_coord = (vec2){max_uvx, max_uvy};
    default_config.vertices[(0 * 4) + 2].tex_coord = (vec2){min_uvx, max_uvy};
    default_config.vertices[(0 * 4) + 3].tex_coord = (vec2){max_uvx, min_uvy};
    default_config.vertices[(0 * 4) + 0].normal    = (vec3){0, 0, -1};
    default_config.vertices[(0 * 4) + 1].normal    = (vec3){0, 0, -1};
    default_config.vertices[(0 * 4) + 2].normal    = (vec3){0, 0, -1};
    default_config.vertices[(0 * 4) + 3].normal    = (vec3){0, 0, -1};

    // Back face
    default_config.vertices[(1 * 4) + 0].position  = (vec3){max_x, min_y, min_z};
    default_config.vertices[(1 * 4) + 1].position  = (vec3){min_x, max_y, min_z};
    default_config.vertices[(1 * 4) + 2].position  = (vec3){max_x, max_y, min_z};
    default_config.vertices[(1 * 4) + 3].position  = (vec3){min_x, min_y, min_z};
    default_config.vertices[(1 * 4) + 0].tex_coord = (vec2){min_uvx, min_uvy};
    default_config.vertices[(1 * 4) + 1].tex_coord = (vec2){max_uvx, max_uvy};
    default_config.vertices[(1 * 4) + 2].tex_coord = (vec2){min_uvx, max_uvy};
    default_config.vertices[(1 * 4) + 3].tex_coord = (vec2){max_uvx, min_uvy};
    default_config.vertices[(1 * 4) + 0].normal    = (vec3){0, 0, 1};
    default_config.vertices[(1 * 4) + 1].normal    = (vec3){0, 0, 1};
    default_config.vertices[(1 * 4) + 2].normal    = (vec3){0, 0, 1};
    default_config.vertices[(1 * 4) + 3].normal    = (vec3){0, 0, 1};

    // Left
    default_config.vertices[(2 * 4) + 0].position  = (vec3){min_x, min_y, min_z};
    default_config.vertices[(2 * 4) + 1].position  = (vec3){min_x, max_y, max_z};
    default_config.vertices[(2 * 4) + 2].position  = (vec3){min_x, max_y, min_z};
    default_config.vertices[(2 * 4) + 3].position  = (vec3){min_x, min_y, max_z};
    default_config.vertices[(2 * 4) + 0].tex_coord = (vec2){min_uvx, min_uvy};
    default_config.vertices[(2 * 4) + 1].tex_coord = (vec2){max_uvx, max_uvy};
    default_config.vertices[(2 * 4) + 2].tex_coord = (vec2){min_uvx, max_uvy};
    default_config.vertices[(2 * 4) + 3].tex_coord = (vec2){max_uvx, min_uvy};
    default_config.vertices[(2 * 4) + 0].normal    = (vec3){-1, 0, 0};
    default_config.vertices[(2 * 4) + 1].normal    = (vec3){-1, 0, 0};
    default_config.vertices[(2 * 4) + 2].normal    = (vec3){-1, 0, 0};
    default_config.vertices[(2 * 4) + 3].normal    = (vec3){-1, 0, 0};

    // Right face
    default_config.vertices[(3 * 4) + 0].position  = (vec3){max_x, min_y, max_z};
    default_config.vertices[(3 * 4) + 1].position  = (vec3){max_x, max_y, min_z};
    default_config.vertices[(3 * 4) + 2].position  = (vec3){max_x, max_y, max_z};
    default_config.vertices[(3 * 4) + 3].position  = (vec3){max_x, min_y, min_z};
    default_config.vertices[(3 * 4) + 0].tex_coord = (vec2){min_uvx, min_uvy};
    default_config.vertices[(3 * 4) + 1].tex_coord = (vec2){max_uvx, max_uvy};
    default_config.vertices[(3 * 4) + 2].tex_coord = (vec2){min_uvx, max_uvy};
    default_config.vertices[(3 * 4) + 3].tex_coord = (vec2){max_uvx, min_uvy};
    default_config.vertices[(3 * 4) + 0].normal    = (vec3){0, 1, 0};
    default_config.vertices[(3 * 4) + 1].normal    = (vec3){0, 1, 0};
    default_config.vertices[(3 * 4) + 2].normal    = (vec3){0, 1, 0};
    default_config.vertices[(3 * 4) + 3].normal    = (vec3){0, 1, 0};

    // Bottom face
    default_config.vertices[(4 * 4) + 0].position  = (vec3){max_x, min_y, max_z};
    default_config.vertices[(4 * 4) + 1].position  = (vec3){min_x, min_y, min_z};
    default_config.vertices[(4 * 4) + 2].position  = (vec3){max_x, min_y, min_z};
    default_config.vertices[(4 * 4) + 3].position  = (vec3){min_x, min_y, max_z};
    default_config.vertices[(4 * 4) + 0].tex_coord = (vec2){min_uvx, min_uvy};
    default_config.vertices[(4 * 4) + 1].tex_coord = (vec2){max_uvx, max_uvy};
    default_config.vertices[(4 * 4) + 2].tex_coord = (vec2){min_uvx, max_uvy};
    default_config.vertices[(4 * 4) + 3].tex_coord = (vec2){max_uvx, min_uvy};
    default_config.vertices[(4 * 4) + 0].normal    = (vec3){0, -1, 0};
    default_config.vertices[(4 * 4) + 1].normal    = (vec3){0, -1, 0};
    default_config.vertices[(4 * 4) + 2].normal    = (vec3){0, -1, 0};
    default_config.vertices[(4 * 4) + 3].normal    = (vec3){0, -1, 0};

    // Top face
    default_config.vertices[(5 * 4) + 0].position  = (vec3){min_x, max_y, max_z};
    default_config.vertices[(5 * 4) + 1].position  = (vec3){max_x, max_y, min_z};
    default_config.vertices[(5 * 4) + 2].position  = (vec3){min_x, max_y, min_z};
    default_config.vertices[(5 * 4) + 3].position  = (vec3){max_x, max_y, max_z};
    default_config.vertices[(5 * 4) + 0].tex_coord = (vec2){min_uvx, min_uvy};
    default_config.vertices[(5 * 4) + 1].tex_coord = (vec2){max_uvx, max_uvy};
    default_config.vertices[(5 * 4) + 2].tex_coord = (vec2){min_uvx, max_uvy};
    default_config.vertices[(5 * 4) + 3].tex_coord = (vec2){max_uvx, min_uvy};
    default_config.vertices[(5 * 4) + 0].normal    = (vec3){0, 1, 0};
    default_config.vertices[(5 * 4) + 1].normal    = (vec3){0, 1, 0};
    default_config.vertices[(5 * 4) + 2].normal    = (vec3){0, 1, 0};
    default_config.vertices[(5 * 4) + 3].normal    = (vec3){0, 1, 0};

    default_config.index_count = 36;
    default_config.indices     = (u32 *)dallocate(sizeof(u32) * 36, MEM_TAG_RENDERER);

    for (u32 i = 0; i < 6; ++i)
    {
        u32 v_offset                         = i * 4;
        u32 i_offset                         = i * 6;
        default_config.indices[i_offset + 0] = v_offset + 0;
        default_config.indices[i_offset + 1] = v_offset + 1;
        default_config.indices[i_offset + 2] = v_offset + 2;
        default_config.indices[i_offset + 3] = v_offset + 0;
        default_config.indices[i_offset + 4] = v_offset + 3;
        default_config.indices[i_offset + 5] = v_offset + 1;
    }

    const char *name = DEFAULT_GEOMETRY_HANDLE;
    u32         len  = strlen(name);
    dcopy_memory(default_config.name, (void *)name, len);

    default_config.material = material_system_get_default_material();

    geometry_system_create_geometry(&default_config);

    dfree(default_config.vertices, sizeof(vertex) * default_config.vertex_count, MEM_TAG_RENDERER);
    dfree(default_config.indices, sizeof(u32) * default_config.index_count, MEM_TAG_RENDERER);

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
        DTRACE("Geometry %s loaded.", config->name);
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
