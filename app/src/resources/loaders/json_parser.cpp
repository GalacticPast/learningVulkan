#include "core/dasserts.hpp"
#include "core/dfile_system.hpp"
#include "core/dmemory.hpp"
#include "core/dstring.hpp"
#include "core/logger.hpp"
#include "defines.hpp"
#include "json_parser.hpp"
#include "renderer/vulkan/vulkan_types.hpp"

void json_parse_string(const char *ptr, char *out_string)
{
    char ch        = '"';
    s32  occurence = string_first_char_occurence(ptr, ch);
    if (occurence == -1)
    {
        DERROR("json_pasrse_string: there is no '\"/' for string %s", ptr);
        return;
    }
    ptr       += occurence + 1;
    occurence  = string_first_char_occurence(ptr, ch);
    if (occurence == -1)
    {
        DERROR("Json_parse_string: there is no '\"/' for string %s", ptr);
        return;
    }
    s32 var_length = occurence;
    string_ncopy(out_string, ptr, var_length);
};

void json_deserialize_material(dstring *file_base_path, material_config *out_material_config)
{
    u64 buffer_size_requirements = 0;
    file_open_and_read(file_base_path->c_str(), &buffer_size_requirements, 0, 0);
    if (buffer_size_requirements == 0)
    {
        DFATAL("File: %s doesnt exist!!!", file_base_path->c_str());
        return;
    }
    char *buffer = static_cast<char *>(dallocate((buffer_size_requirements + 1) * sizeof(char), MEM_TAG_RENDERER));
    file_open_and_read(file_base_path->c_str(), &buffer_size_requirements, buffer, 0);

    // NOTE : this is horrible. I think :)
    u64   counter = buffer_size_requirements;
    char *ptr     = buffer;

    while (*ptr != '\0' || *ptr != '}')
    {
        // im making an assumption that the variable name length will never be greater than 256
        char var[256] = {'\0'};
        json_parse_string(ptr, var);

        if (string_compare(var, "name"))
        {
            ptr += string_first_char_occurence(ptr, ':');
            json_parse_string(ptr, out_material_config->mat_name);
        }
        else if (string_compare(var, "diffuse_color"))
        {
            ptr += string_first_char_occurence(ptr, ':');
            string_to_vec4(ptr, &out_material_config->diffuse_color, ']');
        }
        else if (string_compare(var, "diffuse_map_name"))
        {
            ptr += string_first_char_occurence(ptr, ':');
            json_parse_string(ptr, out_material_config->albedo_map);
        }
        u32 occurence = string_first_char_occurence(ptr, '\n');
        if (*ptr == '}')
        {
            break;
        }
        ptr += occurence + 1;
    }
    return;
}

enum shader_grammer
{
    STAGES,
    VERTEX,
    FRAGMENT,
    ATTRIBUTES,
    POSITION,
    NORMAL,
    TEX_COORD,
    UNIFORMS,
    PER_FRAME,
    PER_GROUP,
    PER_OBJECT,
    NAME,
    FILE_PATH,
    TYPE,
    BINDING,
    LOCATION,
    STAGE,
};

dstring grammer_to_string(shader_grammer grammer)
{
    switch (grammer)
    {
    case FILE_PATH: {
        return "filepath";
    }
    break;
    case STAGES: {
        return "stages";
    }
    break;
    case VERTEX: {
        return "vertex";
    }
    break;
    case FRAGMENT: {
        return "fragment";
    }
    break;
    case ATTRIBUTES: {
        return "attributes";
    }
    break;
    case POSITION: {
        return "position";
    }
    break;
    case NORMAL: {
        return "normal";
    }
    break;
    case TEX_COORD: {
        return "tex_coord";
    }
    break;
    case UNIFORMS: {
        return "uniforms";
    }
    break;
    case PER_FRAME: {
        return "per_frame";
    }
    break;
    case PER_GROUP: {
        return "per_group";
    }
    break;
    case PER_OBJECT: {
        return "per_object";
    }
    break;
    case NAME: {
        return "name";
    }
    break;
    case TYPE: {
        return "type";
    }
    break;
    case BINDING: {
        return "binding";
    }
    break;
    case LOCATION: {
        return "location";
    }
    break;
    case STAGE: {
        return "stage";
    }
    break;
    }
}
static void extract_file_path(char *line, char *out_file_path)
{
}

#define JSON_CHECK(a, b, msg)                                                                                          \
    {                                                                                                                  \
        if ((a) == INVALID_ID || (a) > (b))                                                                            \
        {                                                                                                              \
            DFATAL(msg);                                                                                               \
            return;                                                                                                    \
        }                                                                                                              \
    }

void json_deserialize_shader(dstring *file_base_path, struct vulkan_shader *out_vk_shader)
{

    // INFO: I think I should parse it stage by stage which will be much more easier to implement. I think. <-- Famous
    // last words
    char *buffer                   = nullptr;
    u64   buffer_size_requirements = INVALID_ID_64;

    file_open_and_read(file_base_path->c_str(), &buffer_size_requirements, 0, 0);
    DASSERT(buffer_size_requirements != INVALID_ID_64);

    buffer = static_cast<char *>(dallocate(sizeof(char) * buffer_size_requirements, MEM_TAG_UNKNOWN));

    file_open_and_read(file_base_path->c_str(), &buffer_size_requirements, buffer, 0);

    bool has_attributes = false;
    bool has_uniforms   = false;
    bool has_vertex     = false;
    bool has_fragment   = false;

    u32 stages_index = string_first_string_occurence(buffer, grammer_to_string(STAGES).c_str());
    if (stages_index == INVALID_ID)
    {
        DFATAL("There are no 'stages' specified in the %s shader configuration file.", file_base_path->c_str())
        return;
    }
    u32 attributes_index = string_first_string_occurence(buffer, grammer_to_string(ATTRIBUTES).c_str());
    if (attributes_index != INVALID_ID)
    {
        has_attributes = true;
    }
    else
    {
        attributes_index = buffer_size_requirements;
    }

    u32 uniforms_index = string_first_string_occurence(buffer, grammer_to_string(UNIFORMS).c_str());
    if (uniforms_index != INVALID_ID)
    {
        has_uniforms = true;
    }
    else
    {
        uniforms_index = buffer_size_requirements;
    }

    // stage
    {
        char *ptr       = buffer;
        ptr            += stages_index;
        u32 next_scope  = DMIN(attributes_index, uniforms_index);

        u32 end_of_scope = string_first_char_occurence(ptr, ']');
        JSON_CHECK(end_of_scope, next_scope, "End of scope identifier ']' is missing for the scope 'stages' .");

        u32 vertex_index = string_first_string_occurence(ptr, grammer_to_string(VERTEX).c_str());
        JSON_CHECK(vertex_index, end_of_scope,
                   "identifier 'vertex' is missing from scope 'stages'. Every shader should have a vertex stage");

        u32 fragment_index = string_first_string_occurence(ptr, grammer_to_string(FRAGMENT).c_str());
        JSON_CHECK(fragment_index, end_of_scope,
                   "identifier 'fragment' is missing from scope 'stages'. Every shader should have a fragment stage");

        // has_vertex = true;
        // has_fragment = true;

        // get vertex_file_path
        {
            char *vertex_ptr      = ptr + vertex_index;
            u32   file_path_index = string_first_string_occurence(vertex_ptr, grammer_to_string(FILE_PATH).c_str());
            JSON_CHECK(file_path_index, fragment_index,
                       "identifier 'file_path' is missing for local scope 'vertex' for global scope 'stages'.");
            vertex_ptr += file_path_index;
            extract_file_path(vertex_ptr, out_vk_shader->vertex_file_path.string);
        }
        // get fragment_file_path
        {
            char *fragment_ptr    = ptr + fragment_index;
            u32   file_path_index = string_first_string_occurence(fragment_ptr, grammer_to_string(FILE_PATH).c_str());
            JSON_CHECK(file_path_index, fragment_index,
                       "identifier 'file_path' is missing for local scope 'fragment' for global scope 'stages'.");
            fragment_ptr += file_path_index;
            extract_file_path(fragment_ptr, out_vk_shader->fragment_file_path.string);
        }
    }
    // attributes
    if (has_attributes)
    {
        char *ptr       = buffer;
        ptr            += attributes_index;
        u32 next_scope  = DMIN(attributes_index, uniforms_index);

        u32 end_of_scope = string_first_char_occurence(ptr, ']');
        JSON_CHECK(end_of_scope, next_scope, "End of scope identifier ']' is missing for the scope 'attributes' .");
    }
}
