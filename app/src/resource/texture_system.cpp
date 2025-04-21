#include "texture_system.hpp"
#include "containers/dhashtable.hpp"
#include "renderer/vulkan/vulkan_backend.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include "vendor/stb_image.h"

#define MAX_TEXTURES_LOADED 1024
#define DEFAULT_TEXTURE_HANDLE "DEFAULT_TEXTURE"
#define TEXTURE_FILE_NAME_MAX_LENGTH 512

struct texture_system_state
{
    dhashtable<texture> hashtable;
};

static texture_system_state *tex_sys_state_ptr;
bool                         texture_system_create_default_texture();

bool texture_system_initialize(u64 *texture_system_mem_requirements, void *state)
{
    *texture_system_mem_requirements = sizeof(texture_system_state);
    if (!state)
    {
        return true;
    }
    DINFO("Initializing texture system...");
    tex_sys_state_ptr            = (texture_system_state *)state;
    tex_sys_state_ptr->hashtable = *new dhashtable<texture>(MAX_TEXTURES_LOADED);

    texture_system_create_default_texture();

    return true;
}

bool texture_system_shutdown(void *state)
{
    tex_sys_state_ptr->hashtable.~dhashtable();
    tex_sys_state_ptr = nullptr;
    return true;
}

bool texture_system_create_texture(dstring *file_base_name)
{
    texture texture{};
    texture.name = *file_base_name;

    char full_path_name[TEXTURE_FILE_NAME_MAX_LENGTH] = {};

    const char *prefix = "../assets/textures/";
    string_copy_format((char *)full_path_name, "%s%s", 0, prefix, file_base_name->c_str());

    stbi_uc *pixels = stbi_load((const char *)full_path_name, &(int &)texture.tex_width, &(int &)texture.tex_height,
                                &(int &)texture.num_channels, STBI_rgb_alpha);

    if (!pixels)
    {
        DERROR("Texture creation failed. Error opening file %s", full_path_name);
        return false;
    }

    texture.pixels = &pixels;

    bool result = vulkan_create_texture(&texture);
    if (!result)
    {
        DERROR("Couldnt create default texture.");
        return false;
    }
    tex_sys_state_ptr->hashtable.insert(texture.name.c_str(), texture);
    DDEBUG("Default texture loaded in hastable.");
    return true;
}

bool texture_system_create_default_texture()
{
    texture default_texture{};
    default_texture.name         = (char *)DEFAULT_TEXTURE_HANDLE;
    default_texture.tex_width    = 512;
    default_texture.tex_height   = 512;
    default_texture.num_channels = 4;

    u32 tex_width    = default_texture.tex_width;
    u32 tex_height   = default_texture.tex_height;
    u32 tex_channels = default_texture.num_channels;

    u32 texture_size = default_texture.tex_width * default_texture.tex_height * default_texture.num_channels;

    u8 *pixels = (u8 *)dallocate(texture_size, MEM_TAG_UNKNOWN);

    for (u32 y = 0; y < tex_height; y++)
    {
        for (u32 x = 0; x < tex_width; x++)
        {
            u32 pixel_index = (y * tex_width + x) * tex_channels;
            u8  color       = ((x / 256 + y / 256) % 2) ? 255 : 0;

            pixels[pixel_index + 0] = color;
            pixels[pixel_index + 1] = color;
            pixels[pixel_index + 2] = color;
            pixels[pixel_index + 3] = 255;
        }
    }

    default_texture.pixels = &pixels;

    bool result = vulkan_create_texture(&default_texture);
    if (!result)
    {
        DERROR("Couldnt create default texture.");
        return false;
    }
    tex_sys_state_ptr->hashtable.insert(default_texture.name.c_str(), default_texture);
    DDEBUG("Default texture loaded in hastable.");
    return true;
}

void texture_system_get_texture(const char *texture_name, texture *out_texture)
{
    texture texture = tex_sys_state_ptr->hashtable.find(texture_name);
    dcopy_memory(out_texture, &texture, sizeof(texture));
    return;
}
