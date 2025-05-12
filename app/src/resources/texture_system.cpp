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
    darray<dstring> loaded_textures;
    // dhashtable<texture> *hashtable;
    u32     loaded_textures_count;
    texture textures_table[MAX_TEXTURES_LOADED];
};

static texture_system_state *tex_sys_state_ptr;
bool                         texture_system_create_default_texture();

static bool texture_system_release_textures(dstring *texture_name);

texture *_get_texture(const char *texture_name)
{
    texture *out_tex = nullptr;

    for (u32 i = 0; i < MAX_TEXTURES_LOADED; i++)
    {
        const char *tex_name = (const char *)tex_sys_state_ptr->textures_table[i].name.c_str();
        bool        result   = string_compare(texture_name, tex_name);
        if (result)
        {
            out_tex = &tex_sys_state_ptr->textures_table[i];
            break;
        }
    }
    return out_tex;
}
u32 _get_texture_empty_id()
{
    for (u32 i = 0; i < MAX_GEOMETRIES_LOADED; i++)
    {
        if (tex_sys_state_ptr->textures_table[i].id == INVALID_ID)
        {
            return i;
        }
    }
    return INVALID_ID;
}

bool texture_system_initialize(u64 *texture_system_mem_requirements, void *state)
{
    *texture_system_mem_requirements = sizeof(texture_system_state);
    if (!state)
    {
        return true;
    }
    DINFO("Initializing texture system...");
    tex_sys_state_ptr = (texture_system_state *)state;

    {
        for (u32 i = 0; i < MAX_TEXTURES_LOADED; i++)
        {
            tex_sys_state_ptr->textures_table[i].id                   = INVALID_ID;
            tex_sys_state_ptr->textures_table[i].tex_width            = INVALID_ID;
            tex_sys_state_ptr->textures_table[i].tex_height           = INVALID_ID;
            tex_sys_state_ptr->textures_table[i].num_channels         = INVALID_ID;
            tex_sys_state_ptr->textures_table[i].vulkan_texture_state = nullptr;
        }
    }

    tex_sys_state_ptr->loaded_textures_count = 0;
    tex_sys_state_ptr->loaded_textures.c_init();

    stbi_set_flip_vertically_on_load(true);
    texture_system_create_default_texture();
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

    u32 tex_ind                                 = tex_sys_state_ptr->loaded_textures_count;
    tex_sys_state_ptr->textures_table[tex_ind]  = *texture;
    tex_sys_state_ptr->loaded_textures_count   += 1;

    DDEBUG("Texture %s loaded in hastable.", file_base_name);
    tex_sys_state_ptr->loaded_textures.push_back(texture->name);

    return true;
}

bool texture_system_create_texture(dstring *file_base_name)
{
    texture texture{};
    texture.name = *file_base_name;

    char full_path_name[TEXTURE_NAME_MAX_LENGTH] = {0};

    const char *prefix = "../assets/textures/";

    string_copy_format((char *)full_path_name, "%s%s", 0, prefix, file_base_name->c_str());

    stbi_uc *pixels = stbi_load((const char *)full_path_name, &(int &)texture.tex_width, &(int &)texture.tex_height,
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

bool texture_system_create_default_texture()
{
    texture default_texture{};
    default_texture.name         = DEFAULT_TEXTURE_HANDLE;
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
            u8  color       = ((x / 32 + y / 32) % 2) ? 255 : 0;

            pixels[pixel_index + 0] = color;
            pixels[pixel_index + 1] = color;
            pixels[pixel_index + 2] = color;
            pixels[pixel_index + 3] = 255;
        }
    }

    bool result = create_texture(&default_texture, pixels);

    return result;
}

texture *texture_system_get_default_texture()
{
    texture *texture = _get_texture(DEFAULT_TEXTURE_HANDLE);
    // TODO: increment the value for texture references

    return texture;
}
texture *texture_system_get_texture(const char *texture_name)
{
    if (texture_name == nullptr)
    {
        DWARN("Texuture name is nullptr retrunring default material");
        return texture_system_get_default_texture();
    }
    texture *texture = _get_texture(texture_name);
    // TODO: increment the value for texture references

    if (texture == nullptr)
    {
        DTRACE("Texture: %s not loaded in yet, loading it...", texture_name);
        dstring name = texture_name;
        texture_system_create_texture(&name);
        texture = _get_texture(texture_name);
    }

    return texture;
}

bool texture_system_release_textures(dstring *tex_name)
{
    const char *texture_name = tex_name->c_str();
    texture    *texture      = _get_texture(texture_name);
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
