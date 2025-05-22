#include "texture_system.hpp"
#include "containers/dhashtable.hpp"
#include "core/dmemory.hpp"
#include "defines.hpp"
#include "renderer/vulkan/vulkan_backend.hpp"
#include "resources/resource_types.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include "vendor/stb_image.h"

struct texture_system_state
{
    darray<dstring>     loaded_textures;
    dhashtable<texture> hashtable;
};

static texture_system_state *tex_sys_state_ptr;
bool                         texture_system_create_default_textures();

static bool texture_system_release_textures(dstring *texture_name);

bool texture_system_initialize(u64 *texture_system_mem_requirements, void *state)
{
    *texture_system_mem_requirements = sizeof(texture_system_state);
    if (!state)
    {
        return true;
    }
    DINFO("Initializing texture system...");
    tex_sys_state_ptr = (texture_system_state *)state;

    tex_sys_state_ptr->hashtable.c_init(MAX_TEXTURES_LOADED);
    tex_sys_state_ptr->loaded_textures.c_init();

    stbi_set_flip_vertically_on_load(true);
    texture_system_create_default_textures();
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

    const char *file_base_name = texture->name.c_str();

    tex_sys_state_ptr->hashtable.insert(file_base_name, *texture);
    tex_sys_state_ptr->loaded_textures.push_back(texture->name);
    DDEBUG("Texture %s loaded in hastable.", file_base_name);

    return true;
}

bool texture_system_create_texture(dstring *file_base_name)
{
    texture texture{};
    texture.name = *file_base_name;

    char full_path_name[TEXTURE_NAME_MAX_LENGTH] = {0};

    const char *prefix = "../assets/textures/";

    string_copy_format((char *)full_path_name, "%s%s", 0, prefix, file_base_name->c_str());

    stbi_uc *pixels = stbi_load((const char *)full_path_name, &(int &)texture.width, &(int &)texture.height,
                                &(int &)texture.num_channels, STBI_rgb_alpha);

    if (!pixels)
    {
        DERROR("Texture creation failed. Error opening file %s: %s", full_path_name, stbi_failure_reason());
        stbi__err(0, 0);
        return false;
    }

    bool result = create_texture(&texture, pixels);
    stbi_image_free(pixels);

    return result;
}

bool texture_system_create_default_textures()
{
    {
        // INFO:black and white grids albedo texutre
        texture default_albedo_texture{};
        default_albedo_texture.name         = DEFAULT_ALBEDO_TEXTURE_HANDLE;
        default_albedo_texture.width        = 512;
        default_albedo_texture.height       = 512;
        default_albedo_texture.num_channels = 4;

        u32 tex_width    = default_albedo_texture.width;
        u32 tex_height   = default_albedo_texture.height;
        u32 tex_channels = default_albedo_texture.num_channels;

        u32 texture_size =
            default_albedo_texture.width * default_albedo_texture.height * default_albedo_texture.num_channels;

        u8 *pixels = (u8 *)dallocate(texture_size, MEM_TAG_UNKNOWN);

        for (u32 y = 0; y < tex_height; y++)
        {
            for (u32 x = 0; x < tex_width; x++)
            {
                u32 pixel_index = (y * tex_width + x) * tex_channels;
                u8  color       = ((x / 32 + y / 32) % 2) ? 255 : 0;

                pixels[pixel_index + 0] = color;
                pixels[pixel_index + 1] = color;
                pixels[pixel_index + 2] = color;
                pixels[pixel_index + 3] = 255;
            }
        }

        bool result = create_texture(&default_albedo_texture, pixels);
        DASSERT(result == true);
        dfree(pixels, texture_size, MEM_TAG_UNKNOWN);
    }

    {
        // INFO: fully opaque alpha texutre
        texture default_alpha_texture{};
        default_alpha_texture.name         = DEFAULT_ALPHA_TEXTURE_HANDLE;
        default_alpha_texture.width        = 512;
        default_alpha_texture.height       = 512;
        default_alpha_texture.num_channels = 4;

        u32 tex_width    = default_alpha_texture.width;
        u32 tex_height   = default_alpha_texture.height;
        u32 tex_channels = default_alpha_texture.num_channels;

        u32 texture_size =
            default_alpha_texture.width * default_alpha_texture.height * default_alpha_texture.num_channels;

        u8 *pixels = (u8 *)dallocate(texture_size, MEM_TAG_UNKNOWN);

        for (u32 y = 0; y < tex_height; y++)
        {
            for (u32 x = 0; x < tex_width; x++)
            {
                u32 pixel_index = (y * tex_width + x) * tex_channels;
                u8  color       = 255;

                pixels[pixel_index + 0] = color;
                pixels[pixel_index + 1] = color;
                pixels[pixel_index + 2] = color;
                pixels[pixel_index + 3] = 255;
            }
        }

        bool result = create_texture(&default_alpha_texture, pixels);
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
    // TODO: erase
    // tex_sys_state_ptr->hashtable->erase(texture_name);
    return true;
}
