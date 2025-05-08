#include "json_parser.hpp"
#include "core/dfile_system.hpp"
#include "core/dstring.hpp"
#include "core/logger.hpp"

void json_parse_string(const char *ptr, char *out_string)
{
    char ch        = '"';
    s32  occurence = string_first_char_occurence((const char *)ptr, ch);
    if (occurence == -1)
    {
        DERROR("there is no '\"/' for string %s", ptr);
        return;
    }
    ptr       += occurence + 1;
    occurence  = string_first_char_occurence((const char *)ptr, ch);
    if (occurence == -1)
    {
        DERROR("there is no '\"/' for string %s", ptr);
        return;
    }
    s32 var_length = occurence;
    string_ncopy(out_string, ptr, var_length);
};

void json_parse_material(dstring *file_base_path, material_config *out_material_config)
{
    u64 buffer_size_requirements = 0;
    file_open_and_read(file_base_path->c_str(), &buffer_size_requirements, 0, 0);
    if (buffer_size_requirements == 0)
    {
        DERROR("File: %s doesnt exist!!!", file_base_path->c_str());
    }
    char *buffer = (char *)dallocate((buffer_size_requirements + 1) * sizeof(char), MEM_TAG_RENDERER);
    file_open_and_read(file_base_path->c_str(), &buffer_size_requirements, buffer, 0);

    // INFO: this is horrible. I think :)
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
            json_parse_string(ptr, out_material_config->diffuse_tex_name);
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
