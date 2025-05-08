#include "core/dclock.hpp"
#include "core/dfile_system.hpp"
#include "core/dmemory.hpp"

#include "containers/dhashtable.hpp"
#include "core/dstring.hpp"
#include "defines.hpp"
#include "geometry_system.hpp"
#include "math/dmath.hpp"
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
void geometry_system_parse_obj(const char *obj_file_full_path, u32 *num_of_objects, geometry_config **geo_configs);
bool destroy_geometry_config(geometry_config *config);

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
        geo_sys_state_ptr->loaded_geometry.data =
            (u64 *)dallocate(sizeof(dstring) * MAX_GEOMETRIES_LOADED, MEM_TAG_DARRAY);
        geo_sys_state_ptr->loaded_geometry.element_size = sizeof(dstring);
        geo_sys_state_ptr->loaded_geometry.capacity     = sizeof(dstring) * MAX_TEXTURES_LOADED;
        geo_sys_state_ptr->loaded_geometry.length       = 1;
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

    u32 index                                      = geo_sys_state_ptr->loaded_geometry.size();
    geo_sys_state_ptr->loaded_geometry[index - 1]  = geo.name;
    geo_sys_state_ptr->loaded_geometry.length     += 1;

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

    geometry_system_parse_obj("../assets/meshes/cube.obj", &num_of_objects, &default_geo_configs);

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

        for (u32 j = 0; j < default_ind_size - 3; j += 3)
        {
            DTRACE("%d %d %d", default_geo_configs[i].indices[j], default_geo_configs[i].indices[j + 1],
                   default_geo_configs[i].indices[j + 2]);
        }

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

// this will allocate and write size back, assumes the caller will call free once the data is processed
void geometry_system_parse_obj(const char *obj_file_full_path, u32 *num_of_objects, geometry_config **geo_configs)
{
    dclock telemetry;
    clock_start(&telemetry);

    u64 buffer_mem_requirements = INVALID_ID_64;
    file_open_and_read(obj_file_full_path, &buffer_mem_requirements, 0, 0);
    if (buffer_mem_requirements == INVALID_ID_64)
    {
        DERROR("Failed to get size requirements for %s", obj_file_full_path);
        return;
    }

    char *buffer = (char *)dallocate(buffer_mem_requirements + 1, MEM_TAG_GEOMETRY);
    file_open_and_read(obj_file_full_path, &buffer_mem_requirements, buffer, 0);

    // TODO: cleaup this piece of shit code
    char *ptr                 = buffer;
    char  vert_substring[3]   = "v ";
    u32   vert_substring_size = 3;

    u64   vertex_first_occurence = string_first_string_occurence(ptr, vert_substring);
    char *vert_                  = ptr + vertex_first_occurence;

    u32 vertex_count_obj = string_num_of_substring_occurence(vert_, vert_substring);

    ptr    = buffer;
    u32 v  = string_first_string_occurence(ptr, vert_substring);
    ptr   += v + vert_substring_size - 1;

    vec3 *vert_coords = (vec3 *)dallocate(sizeof(vec3) * (vertex_count_obj + 1), MEM_TAG_UNKNOWN);

    u32 vert_processed = 0;
    for (u32 i = 0; i < vertex_count_obj; i++)
    {
        char *a;
        vert_coords[i].x = strtof(ptr, &a);
        char *b;
        vert_coords[i].y = strtof(a, &b);
        vert_coords[i].z = strtof(b, NULL);

        ptr = strstr(ptr, "v ");
        vert_processed++;
        if (vert_processed % 1000 == 0)
        {
            DTRACE("Verts Processed: %d Remaining: %d", vert_processed, vertex_count_obj - vert_processed);
        }
        if (ptr == nullptr)
        {
            break;
        }
        ptr += vert_substring_size - 1;
    }

    ptr                                 = buffer;
    u64   vertex_normal_first_occurence = string_first_string_occurence(ptr, "vn");
    char *normal                        = ptr + vertex_normal_first_occurence;

    u32   normal_count_obj = string_num_of_substring_occurence(normal, "vn");
    vec3 *normals          = (vec3 *)dallocate(sizeof(vec3) * (normal_count_obj + 1), MEM_TAG_UNKNOWN);

    ptr                             = buffer;
    char vert_normal[3]             = "vn";
    u32  vert_normal_substring_size = 3;

    u32 vn  = string_first_string_occurence(ptr, vert_normal);
    ptr    += vn + vert_normal_substring_size - 1;

    u32 normal_processed = 0;
    for (u32 i = 0; i < normal_count_obj; i++)
    {
        char *a;
        normals[i].x = strtof(ptr, &a);
        char *b;
        normals[i].y = strtof(a, &b);
        normals[i].z = strtof(b, NULL);

        ptr = strstr(ptr, "vn");
        if (normal_processed % 1000 == 0)
        {
            DTRACE("normals Processed: %d Remaining: %d", normal_processed, normal_count_obj - normal_processed);
        }
        normal_processed++;
        if (ptr == nullptr)
        {
            break;
        }
        ptr += vert_normal_substring_size - 1;
    }

    ptr                                  = buffer;
    u64   vertex_texture_first_occurence = string_first_string_occurence(ptr, "vt");
    char *texture                        = ptr + vertex_texture_first_occurence;

    u32   texture_count_obj = string_num_of_substring_occurence(texture, "vt");
    vec2 *textures          = (vec2 *)dallocate(sizeof(vec2) * (texture_count_obj + 1), MEM_TAG_UNKNOWN);

    ptr                              = buffer;
    char vert_texture[3]             = "vt";
    u32  vert_texture_substring_size = 3;

    u32 vt  = string_first_string_occurence(ptr, vert_texture);
    ptr    += vt + vert_texture_substring_size - 1;

    u32 texture_processed = 0;
    for (u32 i = 0; i < texture_count_obj; i++)
    {
        char *a;
        textures[i].x = strtof(ptr, &a);
        textures[i].y = strtof(a, NULL);

        ptr = strstr(ptr, "vt");
        if (texture_processed % 1000 == 0)
        {
            DTRACE("textures Processed: %d Remaining: %d", texture_processed, texture_count_obj - texture_processed);
        }
        texture_processed++;
        if (ptr == nullptr)
        {
            break;
        }
        ptr += vert_texture_substring_size - 1;
    }

    u32  objects = string_num_of_substring_occurence(buffer, "o ");
    char temp    = ' ';

    *num_of_objects = objects;
    *geo_configs    = (geometry_config *)dallocate(sizeof(geometry_config) * (objects + 1), MEM_TAG_GEOMETRY);

    char *object_ptr              = buffer;
    u32   object_first_occurence  = string_first_string_occurence(object_ptr, "o ");
    object_ptr                   += object_first_occurence + 1;

    const char *material_name      = "usemtl";
    u32         material_name_size = 6;

    for (u32 object = 0; object < objects; object++)
    {
        // get object name
        u32 new_line = string_first_char_occurence(object_ptr, '\n');
        if (new_line != INVALID_ID)
        {
            string_ncopy((*geo_configs)[object].name, object_ptr, new_line);
            (*geo_configs)[object].name[new_line] = '\0';
        }
        // parse object
        object_first_occurence = string_first_string_occurence(object_ptr, "o ");
        if (object_first_occurence != INVALID_ID)
        {
            temp                               = object_ptr[object_first_occurence];
            object_ptr[object_first_occurence] = '\0';
        }

        // get material name for object
        char *material_name_ptr = object_ptr;
        u32   material_name_ind = string_first_string_occurence(material_name_ptr, material_name);
        if (material_name_ind != INVALID_ID)
        {
            material_name_ptr        += material_name_ind + material_name_size + 1;
            u32     usemtl_name_size  = string_first_char_occurence(material_name_ptr, '\n');
            dstring usemtl_name;
            dcopy_memory(usemtl_name.string, material_name_ptr, usemtl_name_size);
            usemtl_name.str_len = usemtl_name_size;

            (*geo_configs)[object].material = material_system_acquire_from_name(&usemtl_name);
        }
        if ((*geo_configs)[object].material == nullptr)
        {
            (*geo_configs)[object].material = material_system_get_default_material();
        }

        //

        u64 tris_count          = string_num_of_substring_occurence(object_ptr, "f ");
        u32 max_vert_for_obj    = 0;
        u32 min_vert_for_obj    = INVALID_ID;
        u32 max_texture_for_obj = 0;
        u32 min_texture_for_obj = INVALID_ID;
        u32 max_normal_for_obj  = 0;
        u32 min_normal_for_obj  = INVALID_ID;

        char *ptr  = object_ptr;
        u32   f    = string_first_string_occurence(ptr, "f ");
        ptr       += f + 2;

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

            char *ptr2 = ptr;
            char *a;
            for (u32 k = 0; k < 9; k++)
            {
                offsets[k] = strtol(ptr2, &a, 10);
                ptr2       = a;
                a          = NULL;
            }

            for (u32 k = 0; k < 9; k += 3)
            {
                max_vert_for_obj    = DMAX(max_vert_for_obj, offsets[k]);
                min_vert_for_obj    = DMIN(min_vert_for_obj, offsets[k]);
                max_texture_for_obj = DMAX(max_texture_for_obj, offsets[k + 1]);
                min_texture_for_obj = DMIN(min_texture_for_obj, offsets[k + 1]);
                max_normal_for_obj  = DMAX(max_normal_for_obj, offsets[k + 2]);
                min_normal_for_obj  = DMIN(min_normal_for_obj, offsets[k + 2]);
            }

            f    = string_first_string_occurence(ptr, "f ");
            ptr += f + 2;
        }

        vertex **vert_arr = (vertex **)(&((*geo_configs)[object].vertices));

        u32 obj_vertices_count = max_vert_for_obj - min_vert_for_obj + 1;
        *vert_arr              = (vertex *)dallocate(sizeof(vertex) * obj_vertices_count, MEM_TAG_GEOMETRY);

        u32 **ind_arr            = (u32 **)(&((*geo_configs)[object].indices));
        u32   obj_indicies_count = tris_count * 3;
        *ind_arr                 = (u32 *)dallocate(sizeof(u32) * (obj_indicies_count), MEM_TAG_GEOMETRY);

        ptr  = object_ptr;
        f    = string_first_string_occurence(ptr, "f ");
        ptr += f + 2;

        for (u32 i = 0; i < tris_count; i++)
        {
            u32 offsets[9] = {0};

            char *ptr2 = ptr;
            char *a;
            for (u32 k = 0; k < 9; k++)
            {
                offsets[k] = strtol(ptr2, &a, 10);
                ptr2       = a;
                a          = NULL;
            }

            u32 *ind_dum_ptr = ((*ind_arr)) + (3 * i);

            for (u32 k = 0; k < 9; k += 3)
            {
                DASSERT(offsets[k] - min_vert_for_obj <= obj_vertices_count);
                vertex *dum = &((*vert_arr)[offsets[k] - min_vert_for_obj]);

                dum->position  = vert_coords[offsets[k] - 1];
                dum->tex_coord = textures[offsets[k + 1] - 1];
                dum->normal    = normals[offsets[k + 2] - 1];
            }

            ind_dum_ptr[0] = offsets[0] - min_vert_for_obj;
            ind_dum_ptr[1] = offsets[3] - min_vert_for_obj;
            ind_dum_ptr[2] = offsets[6] - min_vert_for_obj;

            f    = string_first_string_occurence(ptr, "f ");
            ptr += f + 1;
        }

        (*geo_configs)[object].vertex_count = obj_vertices_count;
        (*geo_configs)[object].index_count  = tris_count * 3;

        if (object_first_occurence != INVALID_ID)
        {
            object_ptr[object_first_occurence]  = temp;
            temp                                = ' ';
            object_ptr                         += object_first_occurence + 1;
        }
    }
    dfree(textures, sizeof(vec2) * (texture_count_obj + 1), MEM_TAG_UNKNOWN);
    dfree(normals, sizeof(vec3) * (normal_count_obj + 1), MEM_TAG_UNKNOWN);
    dfree(vert_coords, sizeof(vec3) * (vertex_count_obj + 1), MEM_TAG_UNKNOWN);
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

    material_system_parse_mtl_file((const char *)file_mtl_full_path);
    geometry_system_parse_obj(file_full_path, &sponza_objects, &sponza_geo_configs);

    *sponza_geos = (geometry **)dallocate(sizeof(geometry *) * sponza_objects, MEM_TAG_GEOMETRY);
    vec3 scale   = {0.5, 0.5, 0.5};

    for (u32 i = 0; i < sponza_objects; i++)
    {

        scale_geometries(&sponza_geo_configs[i], scale);
        geometry_system_create_geometry(&sponza_geo_configs[i]);
        dstring geo       = sponza_geo_configs[i].name;
        (*sponza_geos)[i] = geometry_system_get_geometry_by_name(geo);
    }
    *sponza_geometry_count = sponza_objects;

    return;
}

bool destroy_geometry_config(geometry_config *config)
{
    dfree(config->vertices, sizeof(vertex) * config->vertex_count, MEM_TAG_GEOMETRY);
    dfree(config->indices, sizeof(u32) * config->index_count, MEM_TAG_GEOMETRY);
    // TODO: release material ?
    config->material     = nullptr;
    config->vertices     = nullptr;
    config->indices      = nullptr;
    config->vertex_count = INVALID_ID;
    config->index_count  = INVALID_ID;
    dzero_memory(config->name, GEOMETRY_NAME_MAX_LENGTH);
    return true;
}
