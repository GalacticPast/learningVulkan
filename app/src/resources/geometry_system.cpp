#include "defines.hpp"

#include "containers/dhashtable.hpp"

#include "core/dclock.hpp"
#include "core/dfile_system.hpp"
#include "core/dmemory.hpp"

#include "core/dstring.hpp"

#include "geometry_system.hpp"

#include "math/dmath.hpp"

#include "renderer/vulkan/vulkan_backend.hpp"
#include "resources/material_system.hpp"
#include "resources/resource_types.hpp"

#include <cstring>
#include <stdio.h>

struct geometry_system_state
{
    darray<dstring>      loaded_geometry;
    dhashtable<geometry> hashtable;
    u64                  default_geo_id;
};

static geometry_system_state *geo_sys_state_ptr;
bool                          geometry_system_create_default_geometry();

// this will allocate and write size back, assumes the caller will call free once the data is processed
static void geometry_system_parse_obj(const char *obj_file_full_path, u32 *num_of_objects,
                                      geometry_config **geo_configs);
static bool destroy_geometry_config(geometry_config *config);
static bool geometry_system_write_configs_to_file(dstring *file_full_path, u32 geometry_config_count,
                                                  geometry_config *configs);
static bool geometry_system_parse_bin_file(dstring *file_name, u32 *geometry_config_count, geometry_config **configs);
static void calculate_tangents(geometry_config *config);

bool geometry_system_initialize(u64 *geometry_system_mem_requirements, void *state)
{
    *geometry_system_mem_requirements = sizeof(geometry_system_state);
    if (!state)
    {
        return true;
    }
    geo_sys_state_ptr = static_cast<geometry_system_state *>(state);

    geo_sys_state_ptr->hashtable.c_init(MAX_GEOMETRIES_LOADED);
    geo_sys_state_ptr->hashtable.is_non_resizable = true;
    geo_sys_state_ptr->loaded_geometry.c_init();

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

u64 geometry_system_create_geometry(geometry_config *config, bool use_name)
{
    calculate_tangents(config);
    geometry geo{};
    u32      tris_count    = config->vertex_count;
    u32      indices_count = config->index_count;
    bool     result        = vulkan_create_geometry(&geo, tris_count, config->vertices, indices_count, config->indices);
    geo.name               = config->name;
    geo.reference_count    = 0;
    geo.material           = config->material;
    // default model
    geo.ubo.model          = math::mat4();
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
    config.vertices     = static_cast<vertex *>(dallocate(sizeof(vertex) * config.vertex_count, MEM_TAG_GEOMETRY));
    config.index_count  = x_segment_count * y_segment_count * 6;
    config.indices      = static_cast<u32 *>(dallocate(sizeof(u32) * config.index_count, MEM_TAG_GEOMETRY));

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

            u32     v_offset = ((y * x_segment_count) + x) * 4;
            vertex *v0       = &(static_cast<vertex *>(config.vertices))[v_offset + 0];
            vertex *v1       = &(static_cast<vertex *>(config.vertices))[v_offset + 1];
            vertex *v2       = &(static_cast<vertex *>(config.vertices))[v_offset + 2];
            vertex *v3       = &(static_cast<vertex *>(config.vertices))[v_offset + 3];

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
    dstring file_name   = material_name;
    config.material     = material_system_acquire_from_config_file(&file_name);

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
        geometry_system_parse_obj(file_full_path.c_str(), &num_objects, &config);
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
        geometry_system_parse_obj("../assets/meshes/cube.obj", &num_of_objects, &default_geo_configs);
        DASSERT(num_of_objects != INVALID_ID);
        geometry_system_write_configs_to_file(&bin_file, num_of_objects, default_geo_configs);
    }

    DASSERT(num_of_objects != INVALID_ID);
    for (u32 i = 0; i < num_of_objects; i++)
    {
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

    dcopy_memory(dst_config->name.string, src_config->name.string, src_config->name.str_len);
    dst_config->name.str_len = src_config->name.str_len;

    dst_config->vertex_count = src_config->vertex_count;
    dst_config->vertices =
        static_cast<vertex *>(dallocate(sizeof(vertex) * src_config->vertex_count, MEM_TAG_GEOMETRY));
    dcopy_memory(dst_config->vertices, src_config->vertices, src_config->vertex_count * sizeof(vertex));

    dst_config->index_count = src_config->index_count;
    dst_config->indices     = static_cast<u32 *>(dallocate(sizeof(u32) * src_config->index_count, MEM_TAG_GEOMETRY));
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

void _geometry_system_parse_obj(const char *obj_file_full_path, u32 *num_of_objects, geometry_config **geo_configs)
{
    ZoneScoped;

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
    char *buffer = static_cast<char *>(dallocate(buffer_mem_requirements + 1, MEM_TAG_GEOMETRY));
    char *start  = buffer;

    file_open_and_read(obj_file_full_path, &buffer_mem_requirements, buffer, 0);

    u32 objects        = string_num_of_substring_occurence(buffer, "o ");
    u32 usemtl_objects = string_num_of_substring_occurence(buffer, "usemtl");

    bool usemtl_name = true;
    if (usemtl_objects < objects)
    {
        usemtl_name = false;
    }
    objects = DMAX(objects, usemtl_objects);

    *num_of_objects = objects;

    *geo_configs = static_cast<geometry_config *>(dallocate(sizeof(geometry_config) * objects, MEM_TAG_GEOMETRY));




}

void geometry_system_parse_obj(const char *obj_file_full_path, u32 *num_of_objects, geometry_config **geo_configs)
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
    char *buffer = static_cast<char *>(dallocate(buffer_mem_requirements + 1, MEM_TAG_GEOMETRY));
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

    *geo_configs = static_cast<geometry_config *>(dallocate(sizeof(geometry_config) * objects, MEM_TAG_GEOMETRY));

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
            object_name_ptr += 2;

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

    math::vec3 *vert_coords =
        static_cast<math::vec3 *>(dallocate(sizeof(math::vec3) * vertex_count_obj, MEM_TAG_GEOMETRY));

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

    math::vec3 *normals = static_cast<math::vec3 *>(dallocate(sizeof(math::vec3) * normal_count_obj, MEM_TAG_GEOMETRY));

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

    math::vec_2d *textures =
        static_cast<math::vec_2d *>(dallocate(sizeof(math::vec_2d) * texture_count_obj, MEM_TAG_GEOMETRY));

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
    u32 *offsets      = static_cast<u32 *>(dallocate(sizeof(u32) * (offsets_size), MEM_TAG_GEOMETRY));

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
            static_cast<vertex *>(dallocate(sizeof(vertex) * (count / 3), MEM_TAG_GEOMETRY));
        (*geo_configs)[object].vertex_count = count / 3;
        (*geo_configs)[object].indices     = static_cast<u32 *>(dallocate(sizeof(u32) * (count / 3), MEM_TAG_GEOMETRY));
        (*geo_configs)[object].index_count = count / 3;

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

    if (result)
    {
        geometry_system_parse_bin_file(&bin_file_full_path, &objects, &geo_configs);
    }
    else
    {
        geometry_system_parse_obj(obj_file_full_path, &objects, &geo_configs);
        geometry_system_write_configs_to_file(&bin_file_full_path, objects, geo_configs);
    }

    DASSERT(objects != INVALID_ID);
    *geos = static_cast<geometry **>(dallocate(sizeof(geometry *) * objects, MEM_TAG_GEOMETRY));

    // HACK:
    math::vec3 scale = {0.5, 0.5, 0.5};

    for (u32 i = 0; i < objects; i++)
    {

        scale_geometries(&geo_configs[i], scale);
        u64 id     = geometry_system_create_geometry(&geo_configs[i], false);
        (*geos)[i] = geometry_system_get_geometry(id);
    }
    *geometry_count = objects;

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
        file_write(&f, reinterpret_cast<const char *>(configs[i].vertices), configs[i].vertex_count * sizeof(vertex));
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

    char *buffer = static_cast<char *>(dallocate(buff_size + 1, MEM_TAG_GEOMETRY));
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
            (*configs) =
                static_cast<geometry_config *>(dallocate(sizeof(geometry_config) * config_count, MEM_TAG_GEOMETRY));
            ptr += sizeof(u32) + 1;
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

            u32 size                    = sizeof(vertex) * vertex_count;
            (*configs)[index].vertices  = static_cast<vertex *>(dallocate(size, MEM_TAG_GEOMETRY));
            ptr                        += sizeof(u32) + 1;
        }
        else if (string_compare(identifier.c_str(), "vertices"))
        {
            void *dst  = (*configs)[index].vertices;
            u32   size = sizeof(vertex) * (*configs)[index].vertex_count;
            dcopy_memory(dst, ptr, size);
            ptr += size + 1;
        }
        else if (string_compare(identifier.c_str(), "index_count"))
        {
            u32 index_count;
            dcopy_memory(&index_count, ptr, sizeof(u32));

            (*configs)[index].index_count = index_count;

            u32 size                   = sizeof(u32) * index_count;
            (*configs)[index].indices  = static_cast<u32 *>(dallocate(size, MEM_TAG_GEOMETRY));
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

    u32 size = config->index_count;
    DASSERT(size);
    for (u32 i = 0; i < size - 3; i += 3)
    {
        u32 ind_a = config->indices[i];
        u32 ind_b = config->indices[i + 1];
        u32 ind_c = config->indices[i + 2];

        vertex *a = &config->vertices[ind_a];
        vertex *b = &config->vertices[ind_b];
        vertex *c = &config->vertices[ind_c];

        // Because the winding is clockwise
        //  verify if this is correct
        math::vec3 edge0 = a->position - b->position;
        math::vec3 edge1 = c->position - b->position;

        math::vec_2d delta_uv1 = a->tex_coord - b->tex_coord;
        math::vec_2d delta_uv2 = c->tex_coord - b->tex_coord;

        float determinant = 1 / (delta_uv1.x * delta_uv2.y - delta_uv2.x * delta_uv1.y);

        math::vec4 tangent;
        tangent.x = determinant * (delta_uv2.y * edge0.x - delta_uv1.y * edge1.x);
        tangent.y = determinant * (delta_uv2.y * edge0.y - delta_uv1.y * edge1.y);
        tangent.z = determinant * (delta_uv2.y * edge0.z - delta_uv1.y * edge1.z);
        tangent.normalize();

        f32  sx         = delta_uv1.x;
        f32  sy         = delta_uv2.x;
        f32  tx         = delta_uv1.y;
        f32  ty         = delta_uv2.y;
        f32  handedness = ((tx * sy - ty * sx) < 0.0f) ? -1.0f : 1.0f;
        tangent.w = handedness;

        a->tangent = tangent;
        b->tangent = tangent;
        c->tangent = tangent;
    }
}
