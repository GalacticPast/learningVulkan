#include "core/dclock.hpp"
#include "core/dfile_system.hpp"
#include "core/dmemory.hpp"

#include "core/dstring.hpp"
#include "defines.hpp"
#include "geometry_system.hpp"
#include "math/dmath.hpp"
#include "memory/linear_allocator.hpp"
#include "renderer/vulkan/vulkan_backend.hpp"
#include "resources/material_system.hpp"
#include "resources/resource_types.hpp"
#include <cstring>
#include <stdio.h>

struct geometry_system_state
{
    darray<dstring> loaded_geometry;
    u32             geometry_count;
    geometry        geometry_table[MAX_GEOMETRIES_LOADED];
};

static geometry_system_state *geo_sys_state_ptr;
bool                          geometry_system_create_default_geometry();

// this will allocate and write size back, assumes the caller will call free once the data is processed
void geometry_system_parse_obj(const char *obj_file_full_path, u32 *num_of_objects, geometry_config **geo_configs,
                               linear_allocator *allocator);
bool destroy_geometry_config(geometry_config *config);

geometry *_get_goemetry(const char *goemetry_name)
{
    geometry *out_geo = nullptr;

    for (u32 i = 0; i < MAX_MATERIALS_LOADED; i++)
    {
        const char *geo_name = (const char *)geo_sys_state_ptr->geometry_table[i].name.c_str();
        bool        result   = string_compare(goemetry_name, geo_name);
        if (result)
        {
            out_geo = &geo_sys_state_ptr->geometry_table[i];
        }
    }
    return out_geo;
}
u32 _get_geometry_empty_id()
{
    for (u32 i = 0; i < MAX_GEOMETRIES_LOADED; i++)
    {
        if (geo_sys_state_ptr->geometry_table[i].id == INVALID_ID)
        {
            return i;
        }
    }
    return INVALID_ID;
}

bool geometry_system_initialize(u64 *geometry_system_mem_requirements, void *state)
{
    *geometry_system_mem_requirements = sizeof(geometry_system_state);
    if (!state)
    {
        return true;
    }
    geo_sys_state_ptr = (geometry_system_state *)state;

    // geo_sys_state_ptr->hashtable = new dhashtable<geometry>(MAX_GEOMETRIES_LOADED);
    {
        geo_sys_state_ptr->geometry_count = 0;
        for (u32 i = 0; i < MAX_MATERIALS_LOADED; i++)
        {
            geo_sys_state_ptr->geometry_table[i].id = INVALID_ID;
        }
    }

    geo_sys_state_ptr->loaded_geometry.c_init(MAX_GEOMETRIES_LOADED);

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
        geo                  = _get_goemetry(geo_name);

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

bool geometry_system_create_geometry(geometry_config *config)
{
    if (config->name[0] == '\0')
    {
        DERROR("Provided geometry config doesnt have a valid geometry name .");
        return false;
    }
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
    geo_sys_state_ptr->geometry_table[geo_sys_state_ptr->geometry_count] = geo;

    geo_sys_state_ptr->loaded_geometry.push_back(geo.name);
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
    config.vertices     = (vertex *)dallocate(sizeof(vertex) * config.vertex_count, MEM_TAG_GEOMETRY);
    config.index_count  = x_segment_count * y_segment_count * 6;
    config.indices      = (u32 *)dallocate(sizeof(u32) * config.index_count, MEM_TAG_GEOMETRY);

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
    config.material   = material_system_acquire_from_config_file(&file_name);

    return config;
}

geometry_config geometry_system_generate_cube_config()
{
    const char *file_full_path = "../assets/meshes/cube.obj";
    const char *file_name      = "cube.obj";

    geometry_config config{};

    // u32     *cube_vert_object_size = nullptr;
    // vertex **cube_vert             = nullptr;
    // u32     *cube_ind_object_size  = nullptr;
    // u32    **cube_ind              = nullptr;

    // geometry_system_parse_obj(file_full_path, &cube_vert_object_size, &cube_vert, &cube_ind_object_size, &cube_ind);

    // u32 cube_vert_size = cube_vert[0].size();
    // u32 cube_ind_size  = cube_ind[0].size();

    // dcopy_memory(config.vertices, cube_vert.data, cube_vert_size);
    // dcopy_memory(config.indices, cube_ind.data, cube_ind_size);

    string_ncopy(config.name, file_name, GEOMETRY_NAME_MAX_LENGTH);

    return config;
}

bool geometry_system_create_default_geometry()
{

    u32              num_of_objects      = INVALID_ID;
    geometry_config *default_geo_configs = nullptr;
    linear_allocator allocator{};

    geometry_system_parse_obj("../assets/meshes/cube.obj", &num_of_objects, &default_geo_configs, &allocator);

    for (u32 i = 0; i < num_of_objects; i++)
    {

        for (u32 j = 0; j < num_of_objects; j++)
        {
            DTRACE("Object_Name: %s", default_geo_configs[i].name);
        }
        u32 default_vert_size = default_geo_configs[i].vertex_count;

        for (u32 j = 0; j < default_vert_size; j++)
        {
            DTRACE("position{ x: %f , y: %f, z: %f}, normals {x: %f, y: %f, z: %f}, texture{u: %f, v: %f}",
                   default_geo_configs[i].vertices[j].position.x, default_geo_configs[i].vertices[j].position.y,
                   default_geo_configs[i].vertices[j].position.z, default_geo_configs[i].vertices[j].normal.x,
                   default_geo_configs[i].vertices[j].normal.y, default_geo_configs[i].vertices[j].normal.z,
                   default_geo_configs[i].vertices[j].tex_coord.x, default_geo_configs[i].vertices[j].tex_coord.y);
        }
        u32 default_ind_size = default_geo_configs[i].index_count;

        for (u32 j = 0; j <= default_ind_size - 3; j += 3)
        {
            DTRACE("%d %d %d", default_geo_configs[i].indices[j], default_geo_configs[i].indices[j + 1],
                   default_geo_configs[i].indices[j + 2]);
        }
        string_ncopy(default_geo_configs[i].name, DEFAULT_GEOMETRY_HANDLE, sizeof(DEFAULT_GEOMETRY_HANDLE));
        geometry_system_create_geometry(&default_geo_configs[i]);
    }

    for (u32 i = 0; i < num_of_objects; i++)
    {
        destroy_geometry_config(&default_geo_configs[i]);
    }

    dfree(default_geo_configs, num_of_objects * sizeof(geometry_config), MEM_TAG_GEOMETRY);

    return true;
}

geometry *geometry_system_get_geometry(geometry_config *config)
{
    geometry *geo = _get_goemetry(config->name);
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
    geo->reference_count++;
    return geo;
}
geometry *geometry_system_get_geometry_by_name(dstring geometry_name)
{
    const char *name = geometry_name.c_str();
    geometry   *geo  = _get_goemetry(name);
    if (!geo)
    {
        DWARN("Geometry %s not loaded yet.Load it first by calling geometry_system_get_geometry. Returning "
              "default_geometry",
              name);
        return geometry_system_get_default_geometry();
    }
    geo->reference_count++;
    return geo;
}

geometry *geometry_system_get_default_geometry()
{
    geometry *geo = _get_goemetry(DEFAULT_GEOMETRY_HANDLE);
    if (!geo)
    {
        DERROR("Default geometry is not loaded yet. How is this possible?? Make sure that you have initialzed the "
               "geometry system first.");
        return nullptr;
    }
    geo->reference_count++;
    return geo;
}

// this will allocate and write size back, assumes the caller will call free once the data is processed
void geometry_system_parse_obj(const char *obj_file_full_path, u32 *num_of_objects, geometry_config **geo_configs,
                               linear_allocator *allocator)
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
    char *buffer = (char *)dallocate(buffer_mem_requirements + 1, MEM_TAG_GEOMETRY);
    file_open_and_read(obj_file_full_path, &buffer_mem_requirements, buffer, 0);

    // TODO: cleaup this piece of shit code
    u32 objects     = string_num_of_substring_occurence(buffer, "o ");
    *num_of_objects = objects;
    *geo_configs    = (geometry_config *)dallocate(sizeof(geometry_config) * objects, MEM_TAG_GEOMETRY);

    clock_update(&telemetry);
    f64 names_proccesing_start_time = telemetry.time_elapsed;
    {
        char *object_name_ptr      = buffer;
        char *next_object_name_ptr = nullptr;
        char *object_usemtl_name   = nullptr;

        for (u32 i = 0; i < objects; i++)
        {
            object_name_ptr = strstr(object_name_ptr, "o ");
            if (object_name_ptr == nullptr)
            {
                break;
            }
            auto extract_name = [](char *dest, const char *src) {
                u32 j = 0;
                while (*src != '\n')
                {
                    if (*src == ' ')
                    {
                        src++;
                        continue;
                    }
                    dest[j++] = *src;
                    src++;
                }
            };
            object_name_ptr += 2;
            extract_name((*geo_configs)[i].name, object_name_ptr);

            next_object_name_ptr = strstr(object_name_ptr, "o ");
            if (next_object_name_ptr == nullptr)
            {
                next_object_name_ptr = buffer + buffer_mem_requirements;
            }

            object_usemtl_name = strstr(object_name_ptr, "usemtl");
            if (object_usemtl_name != nullptr && object_usemtl_name < next_object_name_ptr)
            {
                dstring material_name  = {};
                object_usemtl_name    += 6;
                extract_name(material_name.string, object_usemtl_name);
                (*geo_configs)[i].material = material_system_acquire_from_name(&material_name);
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

    u32 v     = string_first_string_occurence(vert_ptr, vert_substring);
    vert_ptr += v + vert_substring_size;

    math::vec3 *vert_coords = (math::vec3 *)dallocate(sizeof(math::vec3) * vertex_count_obj, MEM_TAG_GEOMETRY);

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
    DTRACE("Vert proccesing took %fs.\n", total_vert_time);

    char vert_normal[3]             = "vn";
    u32  vert_normal_substring_size = 2;

    char *norm_ptr         = buffer;
    u32   normal_count_obj = string_num_of_substring_occurence(norm_ptr, "vn");

    math::vec3 *normals = (math::vec3 *)dallocate(sizeof(math::vec3) * normal_count_obj, MEM_TAG_GEOMETRY);

    u32 vn    = string_first_string_occurence(norm_ptr, vert_normal);
    norm_ptr += vn + vert_normal_substring_size;

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
    DTRACE("norm proccesing took %fs.\n", total_norm_time);

    char vert_texture[3]             = "vt";
    u32  vert_texture_substring_size = 2;

    char *tex_ptr           = buffer;
    u32   texture_count_obj = string_num_of_substring_occurence(tex_ptr, "vt");

    math::vec_2d *textures = (math::vec_2d *)dallocate(sizeof(math::vec_2d) * texture_count_obj, MEM_TAG_GEOMETRY);

    u32 vt   = string_first_string_occurence(tex_ptr, vert_texture);
    tex_ptr += vt + vert_texture_substring_size;

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
    DTRACE("norm proccesing took %fs.\n", total_texture_time);

    // INFO: Experimental get all the offsets
    u32 offsets_count = string_num_of_substring_occurence(buffer, "f ");
    // one f represents 3 vertices 3 nromals 3 texture coords which define a
    // triangle. So we need to have enough space for have number of 'f ' * 9
    // indicies.Furthermore, Each object will have its max offset and min offset for vertices, textures, and normals
    // and number of vertices for that object stored in the first 7 index of its
    // array (i.e) 0 , 1 and 2 .. 6 . From 7 till vertices count onwards are the actual
    // global offsets for the object.
    u32  offsets_size = (offsets_count * 9) + (7 * objects);
    u32 *offsets      = (u32 *)dallocate(sizeof(u32) * (offsets_size), MEM_TAG_GEOMETRY);

    u32   object_index = string_first_string_occurence(buffer, "o ");
    char *object_ptr   = buffer + object_index + 2;

    char *indices_ptr = object_ptr;

    s32 object_next_index = string_first_string_occurence(object_ptr, "o ");
    if (object_next_index == EOF)
    {
        object_ptr = buffer + buffer_mem_requirements;
    }
    else
    {
        object_ptr += object_next_index + 2;
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

    u32 object_processed = 0;

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

            offset_ptr_start = offset_ptr_walk;
            offset_ptr_walk  = &offset_ptr_start[7];

            max_vert_for_obj      = 0;
            min_vert_for_obj      = INVALID_ID;
            max_texture_for_obj   = 0;
            min_texture_for_obj   = INVALID_ID;
            max_normal_for_obj    = 0;
            min_normal_for_obj    = INVALID_ID;
            offsets_count_for_obj = 0;

            s32 object_next_index = string_first_string_occurence(object_ptr, "o ");
            if (object_next_index == EOF)
            {
                object_ptr = buffer + buffer_mem_requirements;
            }
            else
            {
                object_ptr += object_next_index + 2;
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
    DTRACE("offsets proccesing took %fs.\n", total_offsets_time);

    clock_update(&telemetry);
    f64 object_proccesing_start_time = telemetry.time_elapsed;

    linear_allocator_create(allocator, GIGA(1));

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

        (*geo_configs)[object].vertices = (vertex *)linear_allocator_allocate(allocator, sizeof(vertex) * (count / 3));
        (*geo_configs)[object].vertex_count = count / 3;
        (*geo_configs)[object].indices      = (u32 *)linear_allocator_allocate(allocator, sizeof(u32) * (count / 3));
        (*geo_configs)[object].index_count  = count / 3;

        u32 index_ind = 0;
        for (u32 j = 0; j <= count - 3; j += 3)
        {
            u32 vert_ind = offset_ptr_walk[j] - 1;
            u32 tex_ind  = offset_ptr_walk[j + 1] - 1;
            u32 norm_ind = offset_ptr_walk[j + 2] - 1;

            (*geo_configs)[object].vertices[index_ind].position  = vert_coords[vert_ind];
            (*geo_configs)[object].vertices[index_ind].tex_coord = textures[tex_ind];
            (*geo_configs)[object].vertices[index_ind].normal    = normals[norm_ind];
            (*geo_configs)[object].indices[index_ind]            = index_ind;
            index_ind++;
        }
        DASSERT(index_ind == (*geo_configs)[object].index_count);
    }

    const char *material_name      = "usemtl";
    u32         material_name_size = 6;

    clock_update(&telemetry);
    f64 object_proccesing_end_time = telemetry.time_elapsed;
    f64 total_object_time          = object_proccesing_end_time - object_proccesing_start_time;
    DTRACE("objects proccesing took %f", total_object_time);

    dfree(textures, sizeof(math::vec3) * texture_count_obj, MEM_TAG_UNKNOWN);
    dfree(normals, sizeof(math::vec3) * normal_count_obj, MEM_TAG_UNKNOWN);
    dfree(vert_coords, sizeof(math::vec3) * vertex_count_obj, MEM_TAG_UNKNOWN);
    dfree(buffer, buffer_mem_requirements, MEM_TAG_GEOMETRY);

    clock_update(&telemetry);
    f64 elapsed = telemetry.time_elapsed;

    DTRACE("Parse obj function took %fs : wtf so slow", elapsed);
}

void geometry_system_get_sponza_geometries(geometry ***sponza_geos, u32 *sponza_geometry_count)
{
    const char *file_name       = "sponza.obj";
    const char *file_prefix     = "../assets/meshes/";
    const char *file_mtl_name   = "sponza.mtl";
    const char *file_mtl_prefix = "../assets/materials/";

    char file_full_path[GEOMETRY_NAME_MAX_LENGTH];
    string_copy_format(file_full_path, "%s%s", 0, file_prefix, file_name);

    char file_mtl_full_path[GEOMETRY_NAME_MAX_LENGTH];
    string_copy_format(file_mtl_full_path, "%s%s", 0, file_mtl_prefix, file_mtl_name);

    u32              sponza_objects     = INVALID_ID;
    geometry_config *sponza_geo_configs = nullptr;

    linear_allocator sponza_allocator{};

    material_system_parse_mtl_file((const char *)file_mtl_full_path);
    geometry_system_parse_obj(file_full_path, &sponza_objects, &sponza_geo_configs, &sponza_allocator);

    // for (u32 i = 0; i < sponza_objects; i++)
    //{
    //     u32 vertex_count = sponza_geo_configs[i].vertex_count;
    //     u32 index_count  = sponza_geo_configs[i].index_count;
    //     DTRACE("%s", sponza_geo_configs[i].name);
    //     for (u32 j = 0; j < vertex_count; j++)
    //     {
    //         DTRACE("position{ x: %f , y: %f, z: %f}, normals {x: %f, y: %f, z: %f}, texture{u: %f, v: %f}",
    //                sponza_geo_configs[i].vertices[j].position.x, sponza_geo_configs[i].vertices[j].position.y,
    //                sponza_geo_configs[i].vertices[j].position.z, sponza_geo_configs[i].vertices[j].normal.x,
    //                sponza_geo_configs[i].vertices[j].normal.y, sponza_geo_configs[i].vertices[j].normal.z,
    //                sponza_geo_configs[i].vertices[j].tex_coord.x, sponza_geo_configs[i].vertices[j].tex_coord.y);
    //     }
    //     for (u32 j = 0; j <= index_count - 3; j += 3)
    //     {
    //         DTRACE("%d %d %d", sponza_geo_configs[i].indices[j], sponza_geo_configs[i].indices[j + 1],
    //                sponza_geo_configs[i].indices[j + 2]);
    //     }
    //     DTRACE("-------------------------------------------------------------------------------------------------------"
    //            "--------------------------------------------");
    // }

    *sponza_geos     = (geometry **)dallocate(sizeof(geometry *) * sponza_objects, MEM_TAG_GEOMETRY);
    math::vec3 scale = {0.5, 0.5, 0.5};

    for (u32 i = 0; i < sponza_objects; i++)
    {

        scale_geometries(&sponza_geo_configs[i], scale);
        geometry_system_create_geometry(&sponza_geo_configs[i]);
        dstring geo       = sponza_geo_configs[i].name;
        (*sponza_geos)[i] = geometry_system_get_geometry_by_name(geo);
    }
    *sponza_geometry_count = sponza_objects;

    // for (u32 i = 0; i < sponza_objects; i++)
    //{
    //     for (u32 j = 0; j < sponza_objects; j++)
    //     {
    //         if (i == j)
    //             continue;
    //         if ((*sponza_geos)[i] == (*sponza_geos)[j])
    //         {
    //             DERROR("ith_ind %d and jth_ind:%d point to the same address %s", i, j,
    //             (*sponza_geos)[i]->name.c_str());
    //         }
    //     }
    // }

    linear_allocator_destroy(&sponza_allocator);

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
    dzero_memory(config->name, GEOMETRY_NAME_MAX_LENGTH);
    return true;
}
