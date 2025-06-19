#include "defines.hpp"

#include "containers/darray.hpp"
#include "containers/dhashtable.hpp"
#include "core/dfile_system.hpp"
#include "core/dmemory.hpp"
#include "core/dstring.hpp"
#include "core/logger.hpp"
#include "material_system.hpp"
#include "shader_system.hpp"

#include "renderer/vulkan/vulkan_backend.hpp"
#include "texture_system.hpp"

#include "resource_types.hpp"

#define STB_TRUETYPE_IMPLEMENTATION
#include "vendor/stb_truetype.h"

struct material_system_state
{
    darray<dstring>      loaded_materials;
    dhashtable<material> hashtable;
    arena               *arena;
};

static material_system_state *mat_sys_state_ptr;

bool        material_system_create_material(material_config *config);
bool        material_system_create_default_material();
static bool material_system_parse_configuration_file(dstring *conf_file_name, material_config *out_config);
static bool _write_glyph_atlas_to_file(dstring *file_base_name, font_glyph_data *glyph_data, u32 length);
static bool _parse_glyph_atlas_file(dstring *file_full_path, font_glyph_data **data, u32 *length);

bool material_system_initialize(arena *arena, u64 *material_system_mem_requirements, void *state)
{
    *material_system_mem_requirements = sizeof(material_system_state);

    if (!state)
    {
        return true;
    }
    DINFO("Initializing material system...");
    DASSERT(arena);

    mat_sys_state_ptr = static_cast<material_system_state *>(state);

    mat_sys_state_ptr->loaded_materials.reserve(arena);
    mat_sys_state_ptr->hashtable.c_init(arena, MAX_MATERIALS_LOADED);
    mat_sys_state_ptr->arena = arena;

    material_system_create_default_material();

    return true;
}
bool material_system_shutdown(void *state)
{
    u64 loaded_materials_count = MAX_MATERIALS_LOADED;

    mat_sys_state_ptr = nullptr;
    return true;
}

material *material_system_get_from_config_file(dstring *file_base_name)
{
    material_config base{};

    material_system_parse_configuration_file(file_base_name, &base);

    material *out_mat = nullptr;
    out_mat           = mat_sys_state_ptr->hashtable.find(base.mat_name.c_str());

    if (out_mat == nullptr)
    {
        DTRACE("Material:%s not loaded yet. Loading it...", file_base_name->c_str());
        material_system_create_material(&base);
    }

    out_mat = mat_sys_state_ptr->hashtable.find(base.mat_name.c_str());
    return out_mat;
};

material *material_system_acquire_from_config(material_config *config)
{

    material *out_mat = mat_sys_state_ptr->hashtable.find(config->mat_name.c_str());
    if (out_mat == nullptr)
    {
        DERROR("No Material by the name of %s. Maybe you havenet loaded it yet. Returning default Material",
               config->mat_name);
        out_mat = material_system_get_default_material();
    }
    return out_mat;
};

bool material_system_create_material(material_config *config)
{

    material mat{};
    mat.name            = config->mat_name;
    mat.id              = mat_sys_state_ptr->hashtable.size();
    mat.reference_count = 0;
    mat.map.diffuse     = texture_system_get_texture(config->albedo_map.c_str());
    mat.map.normal      = texture_system_get_texture(config->normal_map.c_str());
    mat.map.specular    = texture_system_get_texture(config->specular_map.c_str());
    mat.diffuse_color   = config->diffuse_color;

    bool result = vulkan_create_material(&mat, shader_system_get_default_material_shader_id());

    mat_sys_state_ptr->hashtable.insert(config->mat_name.c_str(), mat);
    mat_sys_state_ptr->loaded_materials.push_back(config->mat_name);

    DTRACE("Material %s created", config->mat_name);

    return true;
}

bool material_system_create_default_material()
{
    {
        material default_mat{};
        default_mat.name            = DEFAULT_MATERIAL_HANDLE;
        default_mat.id              = 0;
        default_mat.reference_count = 0;
        default_mat.map.diffuse     = texture_system_get_texture(DEFAULT_ALBEDO_TEXTURE_HANDLE);
        default_mat.map.normal      = texture_system_get_texture(DEFAULT_NORMAL_TEXTURE_HANDLE);
        default_mat.map.specular    = texture_system_get_texture(DEFAULT_ALBEDO_TEXTURE_HANDLE);
        default_mat.diffuse_color   = {1.0f, 1.0f, 1.0f, 1.0f};

        bool result = vulkan_create_material(&default_mat, shader_system_get_default_material_shader_id());
        DASSERT(result);

        mat_sys_state_ptr->hashtable.insert(DEFAULT_MATERIAL_HANDLE, default_mat);
        mat_sys_state_ptr->loaded_materials.push_back(DEFAULT_MATERIAL_HANDLE);
    }
    {
        material default_light_mat{};
        default_light_mat.name            = DEFAULT_LIGHT_MATERIAL_HANDLE;
        default_light_mat.id              = 0;
        default_light_mat.reference_count = 0;
        default_light_mat.map.diffuse     = texture_system_get_texture(DEFAULT_ALBEDO_TEXTURE_HANDLE);
        default_light_mat.map.normal      = texture_system_get_texture(DEFAULT_NORMAL_TEXTURE_HANDLE);
        default_light_mat.map.specular    = texture_system_get_texture(DEFAULT_ALBEDO_TEXTURE_HANDLE);

        default_light_mat.diffuse_color = {1.0f, 1.0f, 1.0f, 1.0f};

        bool result = vulkan_create_material(&default_light_mat, shader_system_get_default_material_shader_id());
        DASSERT(result);

        mat_sys_state_ptr->hashtable.insert(DEFAULT_LIGHT_MATERIAL_HANDLE, default_light_mat);
        mat_sys_state_ptr->loaded_materials.push_back(DEFAULT_LIGHT_MATERIAL_HANDLE);
    }
    // HACK:
    // create skybox
    {
        material skybox{};
        skybox.name            = DEFAULT_CUBEMAP_TEXTURE_HANDLE;
        skybox.id              = 0;
        skybox.reference_count = 0;
        skybox.map.diffuse     = texture_system_get_texture(DEFAULT_CUBEMAP_TEXTURE_HANDLE);
        skybox.map.normal      = nullptr;
        skybox.map.specular    = nullptr;

        bool result = vulkan_create_cubemap(&skybox);
        DASSERT(result);

        mat_sys_state_ptr->hashtable.insert(DEFAULT_CUBEMAP_TEXTURE_HANDLE, skybox);
        mat_sys_state_ptr->loaded_materials.push_back(DEFAULT_CUBEMAP_TEXTURE_HANDLE);
    }
    return true;
}

material *material_system_get_default_material()
{
    material *default_mat = mat_sys_state_ptr->hashtable.find(DEFAULT_MATERIAL_HANDLE);
    default_mat->reference_count++;
    return default_mat;
}

bool material_system_release_materials(dstring *mat_name)
{
    const char *material_name = mat_name->c_str();

    bool found = mat_sys_state_ptr->hashtable.erase(material_name);

    if (!found)
    {
        DERROR("Couldn't find and release material %s. Have you made sure that you have loaded it correctly?",
               material_name);
        return false;
    }
    return true;
}
material *material_system_get_from_name(dstring *material_name)
{

    material *out_mat = mat_sys_state_ptr->hashtable.find(material_name->c_str());
    if (out_mat == nullptr)
    {
        DERROR("No Material by the name of %s. Maybe you havenet loaded it yet. Returning default Material",
               material_name->c_str());
        out_mat = material_system_get_default_material();
    }
    return out_mat;
}

// INFO: base name without any suffix like .ttf or .otf just the base name of the font that you want to use
bool material_system_load_font(dstring *font_base_name, font_glyph_data **data)
{
    arena *arena = mat_sys_state_ptr->arena;

    dstring prefix = "../assets/textures/";

    dstring file_full_path;
    string_copy_format(file_full_path.string, "%s%s%s", 0, prefix.c_str(), font_base_name->c_str(), ".png");

    // INFO: Only ascii characters for now
    stbtt_packedchar char_data[96]; // ASCII 32..126

    u32 atlas_width  = 512;
    u32 atlas_height = 512;

    bool verify_data = false;
    if (!file_exists(&file_full_path))
    {
        verify_data = true;
        file_full_path.clear();
        string_copy_format(file_full_path.string, "%s%s%s", 0, "../assets/fonts/", font_base_name->c_str(), ".ttf");

        std::fstream f;
        u64          font_buffer_size = INVALID_ID_64;

        bool result = file_open_and_read(file_full_path.c_str(), &font_buffer_size, nullptr, false);
        DASSERT(result);

        char *font_buffer = static_cast<char *>(dallocate(arena, font_buffer_size, MEM_TAG_UNKNOWN));
        result            = file_open_and_read(file_full_path.c_str(), &font_buffer_size, font_buffer, false);
        DASSERT(result);

        u32 font_atlas_size   = atlas_width * atlas_height;
        u8 *font_atlas_buffer = static_cast<u8 *>(dallocate(arena, font_atlas_size, MEM_TAG_UNKNOWN));

        stbtt_fontinfo font;
        result = stbtt_InitFont(&font, reinterpret_cast<u8 *>(font_buffer), 0);
        DASSERT(result);

        stbtt_pack_context packContext;
        if (!stbtt_PackBegin(&packContext, font_atlas_buffer, atlas_width, atlas_height, 0, 1, NULL))
        {
            DERROR("Failed to begin font packing.");
        }

        stbtt_PackSetOversampling(&packContext, 1, 1); // No oversampling

        stbtt_pack_range range;
        range.font_size                        = 32;
        range.first_unicode_codepoint_in_range = 32; // space
        range.num_chars                        = 96; // printable ASCII
        range.chardata_for_range               = char_data;

        if (!stbtt_PackFontRanges(&packContext, reinterpret_cast<const u8 *>(font_buffer), 0, &range, 1))
        {
            DERROR("Font range packing failed.");
        }

        stbtt_PackEnd(&packContext);

        _write_glyph_atlas_to_file(font_base_name, reinterpret_cast<font_glyph_data *>(char_data), 96);

        file_full_path.clear();
        string_copy_format(file_full_path.string, "%s%s%s", 0, "../assets/textures/", font_base_name->c_str(), ".png");
        texture_system_write_texture(&file_full_path, atlas_width, atlas_height, 4, IMG_FORMAT_UNORM,
                                     font_atlas_buffer);
    }
    else
    {
        file_full_path.clear();
        string_copy_format(file_full_path.string, "%s%s%s", 0,"../assets/fonts/", font_base_name->c_str(), ".font_data");
        u32 length = 0;
        _parse_glyph_atlas_file(&file_full_path, data, &length);
        DASSERT(length == 96);
    }

    material font_atlas{};
    font_atlas.name            = DEFAULT_FONT_ATLAS_TEXTURE_HANDLE;
    font_atlas.id              = 0;
    font_atlas.reference_count = 0;

    dstring base_name_plus_suffix;
    string_copy_format(base_name_plus_suffix.string, "%s%s", 0, font_base_name->c_str(), ".png");

    font_atlas.map.diffuse  = texture_system_get_texture(base_name_plus_suffix.c_str());
    font_atlas.map.normal   = nullptr;
    font_atlas.map.specular = nullptr;

    bool result = vulkan_create_material(&font_atlas, shader_system_get_default_ui_shader_id());
    DASSERT(result);

    mat_sys_state_ptr->hashtable.insert(DEFAULT_FONT_ATLAS_TEXTURE_HANDLE, font_atlas);
    mat_sys_state_ptr->loaded_materials.push_back(DEFAULT_FONT_ATLAS_TEXTURE_HANDLE);


    if(!*data)
    {
        *data = static_cast<font_glyph_data *>(dallocate(arena, sizeof(font_glyph_data) * 96, MEM_TAG_DARRAY));
        for (u32 i = 0; i < 96; i++)
        {
            void *ptr = &(*data)[i];
            dcopy_memory(ptr, &char_data[i], sizeof(stbtt_packedchar));
        }
    }

#ifdef DEBUG
    if (verify_data)
    {
        for (u32 i = 0; i < 96; i++)
        {
            DASSERT((*data)[i].x0 == char_data[i].x0);
            DASSERT((*data)[i].x1 == char_data[i].x1);
            DASSERT((*data)[i].y0 == char_data[i].y0);
            DASSERT((*data)[i].y1 == char_data[i].y1);
            DASSERT((*data)[i].xoff == char_data[i].xoff);
            DASSERT((*data)[i].yoff == char_data[i].yoff);
            DASSERT((*data)[i].xadvance == char_data[i].xadvance);
            DASSERT((*data)[i].xoff2 == char_data[i].xoff2);
            DASSERT((*data)[i].yoff2 == char_data[i].yoff2);
        }
    }
#endif

    return true;
}

bool _write_glyph_atlas_to_file(dstring *file_base_name, font_glyph_data *glyph_data, u32 length)
{
    dstring full_file_name;
    dstring prefix = "../assets/fonts/";
    dstring suffix = ".font_data";

    string_copy_format(full_file_name.string, "%s%s%s", 0, prefix.c_str(), file_base_name->c_str(), suffix.c_str());

    std::fstream f;
    bool         result = file_open(full_file_name.c_str(), &f, true, true);
    DASSERT_MSG(result, "Failed to open file");

    char new_line = '\n';

    font_glyph_data test{};

    // INFO: file header
    file_write(&f, "font_glyph_data_count:", string_length("font_glyph_data_count:"));
    file_write(&f, reinterpret_cast<const char *>(&length), sizeof(u32));
    file_write(&f, reinterpret_cast<const char *>(&new_line), 1);

    u32 size = sizeof(font_glyph_data);

    for (u32 i = 0; i < length; i++)
    {
        file_write(&f, "data:", string_length("data:"));
        file_write(&f, reinterpret_cast<const char *>(&glyph_data[i]), size);
        file_write(&f, reinterpret_cast<const char *>(&new_line), 1);
    }
    file_close(&f);
    return true;
}

// we might not need this now.
bool _parse_glyph_atlas_file(dstring *file_full_path, font_glyph_data **data, u32 *length)
{

    DASSERT(file_full_path);

    u64 buff_size = INVALID_ID_64;
    file_open_and_read(file_full_path->c_str(), &buff_size, 0, 1);
    arena *arena = mat_sys_state_ptr->arena;

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

    u32 size = 0;
    while (ptr < eof && ptr != nullptr)
    {
        extract_identifier(ptr, identifier.string);
        ptr = go_to_colon(ptr);
        if (!ptr)
        {
            break;
        }

        if (string_compare(identifier.c_str(), "data"))
        {
            font_glyph_data *glyph = &(*data)[index];
            dcopy_memory(glyph, ptr, size);
            ptr += size + 1;
            index++;
        }
        else if (string_compare(identifier.c_str(), "font_glyph_data_count"))
        {
            dcopy_memory(&size, ptr, sizeof(u32));
            (*data)  = static_cast<font_glyph_data *>(dallocate(arena, size * sizeof(font_glyph_data), MEM_TAG_DARRAY));
            *length  = size;
            ptr     += sizeof(u32) + 1;
            size     = 0;
            // small optimization I think. because this will only execute once
            size     = sizeof(font_glyph_data);
        }
        else
        {
            DERROR("Unknwon identifier %s", identifier.c_str());
            return false;
        }
    }
    return true;
}

bool material_system_parse_mtl_file(dstring *mtl_file_name)
{
    DASSERT(mtl_file_name);

    material_config *configs = nullptr;

    dstring prefix = "../assets/materials/";
    dstring full_file_path;
    string_copy_format(full_file_path.string, "%s%s", 0, prefix, mtl_file_name->c_str());

    arena *arena = mat_sys_state_ptr->arena;

    char *file                         = nullptr;
    u64   file_buffer_mem_requirements = INVALID_ID_64;

    file_open_and_read(full_file_path.c_str(), &file_buffer_mem_requirements, 0, 0);
    file = static_cast<char *>(dallocate(arena, file_buffer_mem_requirements + 1, MEM_TAG_RENDERER));
    file_open_and_read(full_file_path.c_str(), &file_buffer_mem_requirements, file, 0);

    DASSERT(file_buffer_mem_requirements != INVALID_ID_64);

    u32 real_size = string_length(file);

    const char *eof = file + file_buffer_mem_requirements;

    s32 num_materials = string_num_of_substring_occurence(file, "newmtl");
    if (num_materials <= 0)
    {
        DERROR("No newmtl defined in %s file", mtl_file_name);
        return false;
    }
    configs =
        static_cast<material_config *>(dallocate(arena, sizeof(material_config) * (num_materials), MEM_TAG_RENDERER));

    auto go_to_next_line = [](const char *ptr) -> const char * {
        while (*ptr != '\n' && *ptr != '\r')
        {
            ptr++;
        }
        return ptr + 1;
    };
    auto extract_identifier = [](const char *line, dstring &dst) {
        u32 index = 0;
        u32 j     = 0;

        while (line[index] != ' ' && line[index] != '\0' && line[index] != '\n' && line[index] != '\r')
        {
            dst[j++] = line[index++];
        }
        dst[j++]    = '\0';
        dst.str_len = j;
    };

    const char *ptr = file;
    dstring     identifier;

    s32 index = -1;

    while (ptr < eof && ptr != nullptr)
    {
        if (*ptr == '#' || *ptr == '\n' || *ptr == '\r')
        {
            ptr = go_to_next_line(ptr);
            continue;
        }
        extract_identifier(ptr, identifier);

        if (string_compare(identifier.c_str(), "newmtl"))
        {
            index++;
            ptr += identifier.str_len;
            extract_identifier(ptr, configs[index].mat_name);
        }
        else if (string_compare(identifier.c_str(), "map_Kd"))
        {
            ptr += identifier.str_len;
            dstring full_path;
            extract_identifier(ptr, full_path);
            u32 size = full_path.str_len - 1;

            u32     i = 0;
            dstring stack;
            while (size--)
            {
                if (full_path[size] == '/')
                {
                    break;
                }
                stack[i++] = full_path[size];
            }
            stack.str_len = i;

            u32 j = 0;
            for (; i > 0; i--)
            {
                configs[index].albedo_map[j++] = stack[i - 1];
            }
            configs[index].albedo_map.str_len = j;
        }
        else if (string_compare(identifier.c_str(), "map_d"))
        {
            ptr += identifier.str_len;
            dstring full_path;
            extract_identifier(ptr, full_path);
            u32 size = full_path.str_len - 1;

            u32     i = 0;
            dstring stack;
            while (size--)
            {
                if (full_path[size] == '/')
                {
                    break;
                }
                stack[i++] = full_path[size];
            }
            stack.str_len = i;

            u32 j = 0;
            for (; i > 0; i--)
            {
                configs[index].alpha_map[j++] = stack[i - 1];
            }
            configs[index].alpha_map.str_len = j;
        }
        ptr = go_to_next_line(ptr);
    }

    for (s32 i = 0; i < num_materials; i++)
    {
        material_system_create_material(&configs[i]);
    }

    return true;
}

static bool material_system_parse_configuration_file(dstring *conf_file_name, material_config *out_config)
{
    DASSERT(conf_file_name);
    DASSERT(out_config);

    dstring     conf_full_path;
    const char *prefix = "../assets/materials/";
    string_copy_format(conf_full_path.string, "%s%s", 0, prefix, conf_file_name->c_str());

    std::fstream file;
    bool         result = file_open(conf_full_path, &file, false, false);
    DASSERT(result);

    auto go_to_colon = [](const char *line) -> const char * {
        u32 index = 0;
        while (line[index] != ':')
        {
            if (line[index] == '\n' || line[index] == '\0' || line[index] == '\r')
            {
                return nullptr;
            }
            index++;
        }
        return line + index + 1;
    };

    auto extract_identifier = [](const char *line, dstring &dst) {
        u32 index = 0;
        u32 j     = 0;

        while (line[index] != ':' && line[index] != '\0')
        {
            if (line[index] == ' ')
            {
                index++;
                continue;
            }
            dst[j++] = line[index++];
        }
        dst[j++]    = '\0';
        dst.str_len = j;
    };

    dstring line;
    while (file_get_line(file, &line))
    {
        // INFO: if comment skip line
        if (line[0] == '#' || line[0] == '\0' || line[0] == '\n' || line[0] == '\r')
        {
            continue;
        }
        dstring identifier;
        extract_identifier(line.c_str(), identifier);

        const char *str = go_to_colon(line.c_str());
        DASSERT(str);
        dstring string = str;

        if (string_compare(identifier.c_str(), "name"))
        {
            extract_identifier(str, out_config->mat_name);
        }
        else if (string_compare(identifier.c_str(), "diffuse_color"))
        {
            string_to_vec4(string.c_str(), &out_config->diffuse_color);
        }
        else if (string_compare(identifier.c_str(), "albedo_map"))
        {
            extract_identifier(str, out_config->albedo_map);
        }
        else if (string_compare(identifier.c_str(), "normal_map"))
        {
            extract_identifier(str, out_config->normal_map);
        }
        else if (string_compare(identifier.c_str(), "specular_map"))
        {
            extract_identifier(str, out_config->specular_map);
        }
        else
        {
            DERROR("Unidentified identifier%s in line %s", identifier.c_str(), line.c_str());
            return false;
        }
    }

    return true;
}
