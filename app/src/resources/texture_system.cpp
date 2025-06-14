#include "containers/dhashtable.hpp"
#include "core/dfile_system.hpp"
#include "core/dmemory.hpp"
#include "defines.hpp"
#include "memory/arenas.hpp"
#include "renderer/vulkan/vulkan_backend.hpp"
#include "resources/resource_types.hpp"
#include "texture_system.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include "vendor/stb_image.h"

struct texture_system_state
{
    darray<dstring>     loaded_textures;
    dhashtable<texture> hashtable;
    arena              *arena;
};

static texture_system_state *tex_sys_state_ptr;
bool                         texture_system_create_default_textures();

static bool texture_system_release_textures(dstring *texture_name);

bool texture_system_initialize(arena *arena, u64 *texture_system_mem_requirements, void *state)
{
    *texture_system_mem_requirements = sizeof(texture_system_state);
    if (!state)
    {
        return true;
    }
    DINFO("Initializing texture system...");
    DASSERT(arena);
    tex_sys_state_ptr = static_cast<texture_system_state *>(state);

    tex_sys_state_ptr->hashtable.c_init(arena, MAX_TEXTURES_LOADED);
    tex_sys_state_ptr->loaded_textures.c_init(arena);
    tex_sys_state_ptr->arena = arena;

    stbi_set_flip_vertically_on_load(true);
    texture_system_create_default_textures();
    dstring conf_file_base_name = "default_skybox.conf";
    texture_system_load_cubemap(&conf_file_base_name);
    return true;
}

bool texture_system_shutdown(void *state)
{
    u64 loaded_textures_count = tex_sys_state_ptr->loaded_textures.size();
    for (u64 i = 0; i < loaded_textures_count; i++)
    {
        dstring *tex_name = &tex_sys_state_ptr->loaded_textures[i];
        texture_system_release_textures(tex_name);
    }

    tex_sys_state_ptr->loaded_textures.~darray();
    tex_sys_state_ptr = nullptr;
    return true;
}

bool create_texture(texture *texture, u8 *pixels)
{
    bool result = vulkan_create_texture(texture, pixels);
    if (!result)
    {
        DERROR("Couldnt create default texture.");
        return false;
    }

    const char *conf_file_base_name = texture->name.c_str();

    tex_sys_state_ptr->hashtable.insert(conf_file_base_name, *texture);
    tex_sys_state_ptr->loaded_textures.push_back(texture->name);
    DDEBUG("Texture %s loaded in hastable.", conf_file_base_name);

    return true;
}

bool texture_system_create_texture(dstring *conf_file_base_name)
{
    texture texture{};
    texture.name = *conf_file_base_name;

    char full_path_name[TEXTURE_NAME_MAX_LENGTH] = {0};

    const char *prefix = "../assets/textures/";

    string_copy_format(static_cast<char *>(full_path_name), "%s%s", 0, prefix, conf_file_base_name->c_str());

    s32 tex_width    = -1;
    s32 tex_height   = -1;
    s32 tex_channels = -1;

    stbi_uc *pixels =
        stbi_load(static_cast<const char *>(full_path_name), &tex_width, &tex_height, &tex_channels, STBI_rgb_alpha);

    if (!pixels)
    {
        DERROR("Texture creation failed. Error opening file %s: %s", full_path_name, stbi_failure_reason());
        stbi__err(0, 0);
        return false;
    }
    DASSERT(tex_width != -1);
    DASSERT(tex_height != -1);
    DASSERT(tex_channels != -1);

    texture.width        = tex_width;
    texture.height       = tex_height;
    texture.num_channels = tex_channels;

    bool result = create_texture(&texture, pixels);
    stbi_image_free(pixels);

    return result;
}

bool texture_system_create_default_textures()
{
    arena *arena = tex_sys_state_ptr->arena;
    {
        // gray albedo texture
        texture default_albedo_texture{};
        default_albedo_texture.name         = DEFAULT_ALBEDO_TEXTURE_HANDLE;
        default_albedo_texture.width        = DEFAULT_TEXTURE_WIDTH;
        default_albedo_texture.height       = DEFAULT_TEXTURE_HEIGHT;
        default_albedo_texture.num_channels = 4;

        u32 tex_width    = default_albedo_texture.width;
        u32 tex_height   = default_albedo_texture.height;
        u32 tex_channels = default_albedo_texture.num_channels;

        u32 texture_size =
            default_albedo_texture.width * default_albedo_texture.height * default_albedo_texture.num_channels;

        u8 *pixels = static_cast<u8 *>(dallocate(arena, texture_size, MEM_TAG_UNKNOWN));

        for (u32 y = 0; y < tex_height; y++)
        {
            for (u32 x = 0; x < tex_width; x++)
            {
                u32 pixel_index = (y * tex_width + x) * tex_channels;

                pixels[pixel_index + 0] = 105;
                pixels[pixel_index + 1] = 105;
                pixels[pixel_index + 2] = 105;
                pixels[pixel_index + 3] = 255;
            }
        }

        bool result = create_texture(&default_albedo_texture, pixels);
        DASSERT(result == true);
        dfree(pixels, texture_size, MEM_TAG_UNKNOWN);
    }
    {
        texture default_normal_texture{};
        default_normal_texture.name         = DEFAULT_NORMAL_TEXTURE_HANDLE;
        default_normal_texture.width        = DEFAULT_TEXTURE_WIDTH;
        default_normal_texture.height       = DEFAULT_TEXTURE_HEIGHT;
        default_normal_texture.num_channels = 4;

        u32 tex_width    = default_normal_texture.width;
        u32 tex_height   = default_normal_texture.height;
        u32 tex_channels = default_normal_texture.num_channels;

        u32 texture_size =
            default_normal_texture.width * default_normal_texture.height * default_normal_texture.num_channels;

        u8 *pixels = static_cast<u8 *>(dallocate(arena, texture_size, MEM_TAG_UNKNOWN));

        for (u32 y = 0; y < tex_height; y++)
        {
            for (u32 x = 0; x < tex_width; x++)
            {
                u32 pixel_index = (y * tex_width + x) * tex_channels;

                pixels[pixel_index + 0] = 128;
                pixels[pixel_index + 1] = 128;
                pixels[pixel_index + 2] = 255;
                pixels[pixel_index + 3] = 255;
            }
        }

        bool result = create_texture(&default_normal_texture, pixels);
        DASSERT(result == true);
        dfree(pixels, texture_size, MEM_TAG_UNKNOWN);
    }

    return true;
}

texture *texture_system_get_texture(const char *texture_name)
{
    if (texture_name == nullptr)
    {
        DWARN("Texuture name is nullptr retrunring default albedo texture");
        return texture_system_get_texture(DEFAULT_ALBEDO_TEXTURE_HANDLE);
    }
    texture *texture = tex_sys_state_ptr->hashtable.find(texture_name);
    // TODO: increment the value for texture references

    if (texture == nullptr)
    {
        DTRACE("Texture: %s not loaded in yet, loading it...", texture_name);
        dstring name = texture_name;
        texture_system_create_texture(&name);
        texture = tex_sys_state_ptr->hashtable.find(texture_name);
    }

    return texture;
}

bool texture_system_release_textures(dstring *tex_name)
{
    const char *texture_name = tex_name->c_str();
    texture    *texture      = tex_sys_state_ptr->hashtable.find(texture_name);
    bool        result       = vulkan_destroy_texture(texture);
    if (!result)
    {
        DERROR("Couldn't release texture %s", texture_name);
        return false;
    }

    return true;
}

bool texture_system_load_cubemap(dstring *conf_file_base_name)
{
    DASSERT(conf_file_base_name);
    dstring full_file_path;
    dstring prefix = "../assets/textures/";
    string_copy_format(full_file_path.string, "%s%s", 0, prefix.c_str(), conf_file_base_name->c_str());

    dstring cubemap_faces[6];

    std::fstream f;
    file_open(full_file_path, &f, false, false);

    dstring line;
    dstring identifier;

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
        // skip the whitespace before an identifier
        while (line[index] == ' ')
        {
            index++;
        }
        while (line[index] != ' ' && line[index] != '\0' && line[index] != '\n' && line[index] != '\r')
        {
            dst[j++] = line[index++];
        }
        dst[j++]    = '\0';
        dst.str_len = j;
    };

    while (file_get_line(f, &line))
    {
        if (line[0] == '#' || line[0] == '\0' || line[0] == '\n' || line[0] == '\r')
        {
            continue;
        }
        extract_identifier(line.c_str(), identifier);
        const char *str = go_to_colon(line.c_str());
        DASSERT(str);

        if (string_compare(identifier.c_str(), "right"))
        {
            extract_identifier(str, cubemap_faces[3]);
        }
        else if (string_compare(identifier.c_str(), "left"))
        {
            extract_identifier(str, cubemap_faces[2]);
        }
        else if (string_compare(identifier.c_str(), "top"))
        {
            extract_identifier(str, cubemap_faces[5]);
        }
        else if (string_compare(identifier.c_str(), "bottom"))
        {
            extract_identifier(str, cubemap_faces[4]);
        }
        else if (string_compare(identifier.c_str(), "front"))
        {
            extract_identifier(str, cubemap_faces[1]);
        }
        else if (string_compare(identifier.c_str(), "back"))
        {
            extract_identifier(str, cubemap_faces[0]);
        }
        else
        {
            DERROR("Unidentified identifier%s in line %s", identifier.c_str(), line.c_str());
            return false;
        }
        identifier.clear();
        line.clear();
    }

    stbi_uc *cubemap_pixels[6] = {nullptr};
    s32      prev_width        = INVALID_ID_S32;
    s32      prev_height       = INVALID_ID_S32;
    s32      prev_channel      = INVALID_ID_S32;

    for (s32 i = 0; i < 6; i++)
    {
        full_file_path.clear();

        s32 width   = INVALID_ID_S32;
        s32 height  = INVALID_ID_S32;
        s32 channel = INVALID_ID_S32;

        string_copy_format(full_file_path.string, "%s%s", 0, prefix.c_str(), cubemap_faces[i].c_str());
        cubemap_pixels[i] = stbi_load(full_file_path.c_str(), &width, &height, &channel, STBI_rgb_alpha);
        if (!cubemap_pixels[i])
        {
            DERROR("Cubemap Texture creation failed. Error opening file %s: %s", full_file_path.c_str(),
                   stbi_failure_reason());
            stbi__err(0, 0);
            return false;
        }
        if (prev_width != INVALID_ID_S32 && prev_height != INVALID_ID_S32 && prev_channel != INVALID_ID_S32)
        {
            DASSERT(width == prev_width);
            DASSERT(height == prev_height);
            DASSERT(channel == prev_channel);
        }
        prev_width   = width;
        prev_height  = height;
        prev_channel = channel;
    }

    texture cubemap_texture;
    // HACK:
    cubemap_texture.name         = DEFAULT_CUBEMAP_TEXTURE_HANDLE;
    cubemap_texture.width        = prev_width;
    cubemap_texture.height       = prev_height;
    cubemap_texture.num_channels = 4;
    cubemap_texture.texure_size  = prev_width * prev_height * 4 * 6;

    arena* temp_arena = arena_get_arena();
    u8* pixels = static_cast<u8 *>(dallocate(temp_arena, cubemap_texture.texure_size, MEM_TAG_RENDERER));

    u32 tex_real_size = prev_width * prev_height * 4;
    u32 layer_size = cubemap_texture.texure_size / 6;
    u8* dest = pixels;
    for(u32 i = 0 ; i < 6 ; i++)
    {
        dest = pixels + (i * tex_real_size);
        dcopy_memory(dest, cubemap_pixels[i], tex_real_size);
    }

    create_texture(&cubemap_texture, pixels);


    return true;
}
