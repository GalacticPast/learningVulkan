#include "core/dfile_system.hpp"
#include "core/dmemory.hpp"

#include "containers/dhashtable.hpp"
#include "core/dstring.hpp"
#include "defines.hpp"
#include "geometry_system.hpp"
#include "renderer/vulkan/vulkan_backend.hpp"
#include "resources/material_system.hpp"
#include "resources/resource_types.hpp"
#include <stdio.h>

struct geometry_system_state
{
    darray<dstring>      loaded_geometry;
    dhashtable<geometry> hashtable;
};

static geometry_system_state *geo_sys_state_ptr;
bool                          geometry_system_create_default_geometry();

// this will allocate and write size back, assumes the caller will call free once the data is processed
void geometry_system_parse_obj(const char *obj_file_full_path, u32 *vertex_count, vertex **vertices, u32 *index_count,
                               u32 **indices);

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
    u32      tris_count    = config->vertex_count;
    u32      indices_count = config->index_count;
    bool     result        = vulkan_create_geometry(&geo, tris_count, config->vertices, indices_count, config->indices);
    geo.name               = config->name;
    geo.reference_count    = 0;
    geo.material           = config->material;

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
    dstring file_name = material_name;
    config.material   = material_system_acquire_from_file(&file_name);

    return config;
}

geometry_config geometry_system_generate_geometry_config(const char *file_name)
{
    const char *file_path_prefix = "../assets/meshes/";

    char file_full_path[GEOMETRY_NAME_MAX_LENGTH];
    string_copy_format(file_full_path, "%s%s", 0, file_path_prefix, file_name);

    geometry_config config{};
    geometry_system_parse_obj(file_full_path, &config.vertex_count, &config.vertices, &config.index_count,
                              &config.indices);

    string_ncopy(config.name, file_name, GEOMETRY_NAME_MAX_LENGTH);

    return config;
}

bool geometry_system_create_default_geometry()
{
    geometry_config default_config{};

    default_config.vertex_count = 32;

    u32     vertex_count = INVALID_ID;
    vertex *vertices     = nullptr;
    u32     index_count  = INVALID_ID;
    u32    *indices      = 0;

    geometry_system_parse_obj("../assets/meshes/cube.obj", &vertex_count, &vertices, &index_count, &indices);

    default_config.vertices     = vertices;
    default_config.vertex_count = vertex_count;
    default_config.index_count  = index_count;
    default_config.indices      = indices;

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

u32 get_number_of_occurences_of_substring(const char *string, const char *substring)
{
    u32 ans = -1;

    u32   f   = 0;
    char *ptr = (char *)string;
    while (f != -1)
    {
        f    = string_first_string_occurence(ptr, substring);
        ptr += f + 1;
        ans++;
    }
    return ans;
}

// this will allocate and write size back, assumes the caller will call free once the data is processed
void geometry_system_parse_obj(const char *obj_file_full_path, u32 *vertex_count, vertex **vertices, u32 *index_count,
                               u32 **indices)
{
    u64 buffer_mem_requirements = INVALID_ID_64;
    file_open_and_read(obj_file_full_path, &buffer_mem_requirements, 0, 0);
    if (buffer_mem_requirements == INVALID_ID_64)
    {
        DERROR("Failed to get size requirements for %s", obj_file_full_path);
        return;
    }
    char *buffer = (char *)dallocate(buffer_mem_requirements, MEM_TAG_RENDERER);
    file_open_and_read(obj_file_full_path, &buffer_mem_requirements, buffer, 0);

    // TODO: cleaup this piece of shit code
    u64   vertex_first_occurence = string_first_char_occurence(buffer, 'v');
    char *vertex                 = buffer + vertex_first_occurence;

    u32 vertex_count_obj = get_number_of_occurences_of_substring(vertex, "v ");

    char *ptr  = buffer;
    u32   v    = string_first_char_occurence(ptr, 'v');
    ptr       += v + 1;

    vec3 *vert_coords = (vec3 *)dallocate(sizeof(vec3) * vertex_count_obj, MEM_TAG_UNKNOWN);

    for (u32 i = 0; i < vertex_count_obj; i++)
    {
        sscanf_s(ptr, "%f %f %f", &vert_coords[i].x, &vert_coords[i].y, &vert_coords[i].z);
        v    = string_first_char_occurence(ptr, 'v');
        ptr += v + 1;
    }

    u64   vertex_normal_first_occurence = string_first_string_occurence((const char *)buffer, "vn");
    char *normal                        = buffer + vertex_normal_first_occurence;

    u32   normal_count_obj = get_number_of_occurences_of_substring(normal, "vn");
    vec3 *normals          = (vec3 *)dallocate(sizeof(vec3) * normal_count_obj, MEM_TAG_UNKNOWN);

    ptr     = buffer;
    u32 vn  = string_first_string_occurence(ptr, "vn");
    ptr    += vn + 2;

    for (u32 i = 0; i < normal_count_obj; i++)
    {
        sscanf_s(ptr, "%f %f %f", &normals[i].x, &normals[i].y, &normals[i].z);
        vn   = string_first_string_occurence(ptr, "vn");
        ptr += vn + 2;
    }

    u64   vertex_texture_first_occurence = string_first_string_occurence(buffer, "vt");
    char *texture                        = buffer + vertex_texture_first_occurence;

    u64 texture_first_occurence = string_first_string_occurence((const char *)buffer, "vn");

    u32   texture_count_obj = get_number_of_occurences_of_substring(texture, "vt");
    vec2 *textures          = (vec2 *)dallocate(sizeof(vec2) * texture_count_obj, MEM_TAG_UNKNOWN);

    ptr     = buffer;
    u32 vt  = string_first_string_occurence(ptr, "vt");
    ptr    += vt + 2;

    for (u32 i = 0; i < texture_count_obj; i++)
    {
        sscanf_s(ptr, "%f %f", &textures[i].x, &textures[i].y);
        vt   = string_first_string_occurence(ptr, "vt");
        ptr += vt + 2;
    }

    u64 tris_count = get_number_of_occurences_of_substring(buffer, "f");

    *vertices = (struct vertex *)dallocate(tris_count * 3 * sizeof(struct vertex), MEM_TAG_RENDERER);
    *indices  = (u32 *)dallocate(sizeof(u32) * tris_count * 3, MEM_TAG_RENDERER);

    ptr    = buffer;
    u32 f  = string_first_char_occurence(ptr, 'f');
    ptr   += f + 1;

    for (u32 i = 0; i < tris_count; i++)
    {
        u32 offsets[9] = {0};
        u32 j          = 0;
        while (ptr[j] != '\n')
        {
            if (ptr[j] == '/')
            {
                ptr[j] = ' ';
            }
            j++;
        }
        sscanf_s(ptr, "%d %d %d %d %d %d %d %d %d", &offsets[0], &offsets[1], &offsets[2], &offsets[3], &offsets[4],
                 &offsets[5], &offsets[6], &offsets[7], &offsets[8]);

        u32 *ind_dum_ptr = (*indices) + (3 * i);

        for (u32 k = 0; k < 9; k += 3)
        {
            struct vertex *dum = (*vertices) + (offsets[k] - 1);

            dum->position  = vert_coords[offsets[k] - 1];
            dum->normal    = normals[offsets[k + 1] - 1];
            dum->tex_coord = textures[offsets[k + 2] - 1];
        }

        ind_dum_ptr[0] = offsets[0] - 1;
        ind_dum_ptr[1] = offsets[3] - 1;
        ind_dum_ptr[2] = offsets[6] - 1;

        f    = string_first_char_occurence(ptr, 'f');
        ptr += f + 1;
    }

    *vertex_count = tris_count * 3;
    *index_count  = tris_count * 3;

    dfree(textures, sizeof(vec2) * texture_count_obj, MEM_TAG_UNKNOWN);
    dfree(normals, sizeof(vec3) * normal_count_obj, MEM_TAG_UNKNOWN);
    dfree(vert_coords, sizeof(vec3) * vertex_count_obj, MEM_TAG_UNKNOWN);
}
