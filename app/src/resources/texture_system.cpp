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

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "vendor/stb_image_write.h"

struct texture_system_state
{
    darray<dstring>     loaded_textures;
    dhashtable<texture> hashtable;
    arena              *arena;

    u32              glyphs_size;
    font_glyph_data *glyphs;
};

static texture_system_state *tex_sys_state_ptr;
bool                         texture_system_create_default_textures();

static bool texture_system_release_textures(dstring *texture_name);
// static bool texture_system_create_font_atlas();

bool texture_system_initialize(arena *system_arena, arena* resource_arena)
{
    DINFO("Initializing texture system...");
    DASSERT(system_arena);
    DASSERT(resource_arena);
    tex_sys_state_ptr = static_cast<texture_system_state *>(dallocate(system_arena, sizeof(texture_system_state), MEM_TAG_APPLICATION));
    DASSERT(tex_sys_state_ptr);

    tex_sys_state_ptr->hashtable.c_init(system_arena, MAX_TEXTURES_LOADED);
    tex_sys_state_ptr->loaded_textures.c_init(system_arena);
    tex_sys_state_ptr->arena = resource_arena;

    stbi_set_flip_vertically_on_load(true);
    texture_system_create_default_textures();
    dstring conf_file_base_name = "yokohama_3.conf";
    texture_system_load_cubemap(&conf_file_base_name);
    return true;
}

bool texture_system_shutdown()
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

bool texture_system_create_texture(dstring *texture_name, u32 tex_width, u32 tex_height, u32 tex_num_channels,
                                   image_format format, u8 *pixels)
{
    DASSERT(texture_name);
    DASSERT(pixels);
    DASSERT(format != IMG_FORMAT_UNKNOWN);

    texture texture{};
    texture.name         = *texture_name;
    texture.format       = format;
    texture.width        = tex_width;
    texture.height       = tex_height;
    texture.num_channels = tex_num_channels;

    bool result = create_texture(&texture, pixels);

    return true;
}

bool texture_system_write_texture(dstring *file_full_path, u32 tex_width, u32 tex_height, u32 tex_num_channels,
                                  image_format format, u8 *pixels)
{
    DASSERT(file_full_path);
    DASSERT(pixels);

    if (!stbi_write_png(file_full_path->c_str(), tex_width, tex_height, 1, pixels, tex_width))
    {
        DERROR("Failed to write PNG: %s.", file_full_path->c_str());
    }
    else
    {
        DDEBUG("Saved %s successfully.", file_full_path->c_str());
    }
    return true;
}

bool texture_system_create_texture(dstring *file_base_name, image_format format)
{
    DASSERT(file_base_name);
    DASSERT(format != IMG_FORMAT_UNKNOWN);

    texture texture{};
    texture.name = *file_base_name;

    char full_path_name[TEXTURE_NAME_MAX_LENGTH] = {0};

    const char *prefix = "../assets/textures/";

    string_copy_format(static_cast<char *>(full_path_name), "%s%s", 0, prefix, file_base_name->c_str());

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
    texture.format       = format;
    bool result          = create_texture(&texture, pixels);
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
        default_albedo_texture.format       = IMG_FORMAT_SRGB;

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
        default_normal_texture.format       = IMG_FORMAT_UNORM;

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
        texture_system_create_texture(&name, IMG_FORMAT_SRGB);
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
            extract_identifier(str, cubemap_faces[0]);
        }
        else if (string_compare(identifier.c_str(), "left"))
        {
            extract_identifier(str, cubemap_faces[1]);
        }
        else if (string_compare(identifier.c_str(), "top"))
        {
            extract_identifier(str, cubemap_faces[2]);
        }
        else if (string_compare(identifier.c_str(), "bottom"))
        {
            extract_identifier(str, cubemap_faces[3]);
        }
        else if (string_compare(identifier.c_str(), "front"))
        {
            extract_identifier(str, cubemap_faces[4]);
        }
        else if (string_compare(identifier.c_str(), "back"))
        {
            extract_identifier(str, cubemap_faces[5]);
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

    stbi_set_flip_vertically_on_load(false);
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
    stbi_set_flip_vertically_on_load(true);

    texture cubemap_texture;
    // HACK:
    cubemap_texture.name         = DEFAULT_CUBEMAP_TEXTURE_HANDLE;
    cubemap_texture.width        = prev_width;
    cubemap_texture.height       = prev_height;
    cubemap_texture.num_channels = 4;
    cubemap_texture.texure_size  = prev_width * prev_height * 4 * 6;
    cubemap_texture.format       = IMG_FORMAT_SRGB;

    arena *temp_arena = arena_get_arena();
    u8    *pixels     = static_cast<u8 *>(dallocate(temp_arena, cubemap_texture.texure_size, MEM_TAG_RENDERER));

    u32 tex_real_size = prev_width * prev_height * 4;
    u32 layer_size    = cubemap_texture.texure_size / 6;
    u8 *dest          = pixels;
    for (u32 i = 0; i < 6; i++)
    {
        dest = pixels + (i * tex_real_size);
        dcopy_memory(dest, cubemap_pixels[i], tex_real_size);
    }

    create_texture(&cubemap_texture, pixels);

    for (s32 i = 0; i < 6; i++)
    {
        stbi_image_free(cubemap_pixels[i]);
    }

    return true;
}

// bool _texture_system_create_font_atlas() // sdf
//{
//     arena *arena = tex_sys_state_ptr->arena;
//
//     dstring prefix    = "../assets/fonts/";
//     dstring font_name = "SourceSansPro-Regular";
//     dstring suffix    = ".png";
//
//     dstring font_atlas_base_name = "SourceSansPro-Regular";
//
//     dstring file_full_path;
//     string_copy_format(file_full_path.string, "%s%s%s", 0, prefix.c_str(), font_atlas_base_name.c_str(),
//                        suffix.c_str());
//
//     bool            verify_data = false;
//     font_glyph_data glyphs[96]; // ASCII 32..126
//
//     if (!file_exists(&file_full_path))
//     {
//         verify_data = true;
//         file_full_path.clear();
//         string_copy_format(file_full_path.string, "%s%s%s", 0, prefix.c_str(), font_name.c_str(), ".ttf");
//
//         std::fstream f;
//         u64          font_buffer_size = INVALID_ID_64;
//
//         bool result = file_open_and_read(file_full_path.c_str(), &font_buffer_size, nullptr, false);
//         DASSERT(result);
//
//         u8 *font_buffer = static_cast<u8 *>(dallocate(arena, font_buffer_size, MEM_TAG_UNKNOWN));
//         result =
//             file_open_and_read(file_full_path.c_str(), &font_buffer_size, reinterpret_cast<char *>(font_buffer),
//             false);
//         DASSERT(result);
//
//         u32 atlas_width  = 512;
//         u32 atlas_height = 512;
//
//         u32 font_atlas_size   = atlas_width * atlas_height;
//         u8 *font_atlas_buffer = static_cast<u8 *>(dallocate(arena, font_atlas_size, MEM_TAG_UNKNOWN));
//
//         stbtt_fontinfo font;
//         if (!stbtt_InitFont(&font, font_buffer, stbtt_GetFontOffsetForIndex(font_buffer, 0)))
//         {
//             DERROR("Failed to init font.");
//             return false;
//         }
//
//         u32   char_count       = 96;
//         u32   glyph_padding    = 4;
//         float pixel_height     = 48.0f;
//         float scale            = stbtt_ScaleForPixelHeight(&font, pixel_height);
//         float onedge_value     = 128;
//         float pixel_dist_scale = 64.0f;
//
//         int pen_x = 0, pen_y = 0, row_height = 0;
//
//         for (u32 i = 0; i < char_count; ++i)
//         {
//             u32 c = 32 + i;
//
//             s32            w, h, xoff, yoff;
//             unsigned char *sdf = stbtt_GetCodepointSDF(&font, scale, c, glyph_padding, onedge_value,
//             pixel_dist_scale,
//                                                        &w, &h, &xoff, &yoff);
//
//             if (pen_x + w >= (s32)atlas_width)
//             {
//                 pen_x       = 0;
//                 pen_y      += row_height + 1;
//                 row_height  = 0;
//             }
//
//             if (pen_y + h >= (s32)atlas_height)
//             {
//                 DERROR("Atlas too small. Increase size.");
//                 free(sdf);
//                 break;
//             }
//
//             // Copy glyph SDF into atlas
//             for (int y = 0; y < h; ++y)
//             {
//                 dcopy_memory(font_atlas_buffer + (pen_y + y) * atlas_width + pen_x, sdf + y * w, w);
//             }
//
//             // Get advance
//             int adv, lsb;
//             stbtt_GetCodepointHMetrics(&font, c, &adv, &lsb);
//
//             glyphs[i].x0       = pen_x;
//             glyphs[i].y0       = pen_y;
//             glyphs[i].x1       = pen_x + w;
//             glyphs[i].y1       = pen_y + h;
//             // glyphs[i].w        = w;
//             // glyphs[i].h        = h;
//             glyphs[i].xoff     = (float)xoff;
//             glyphs[i].yoff     = (float)yoff;
//             glyphs[i].xadvance = scale * adv;
//
//             pen_x += w + 1;
//             if (h > row_height)
//                 row_height = h;
//
//             free(sdf);
//         }
//
//         _write_glyph_atlas_to_file(&font_name, glyphs, char_count);
//
//         file_full_path.clear();
//         string_copy_format(file_full_path.string, "%s%s%s", 0, prefix.c_str(), font_atlas_base_name.c_str(), ".png");
//
//         if (!stbi_write_png(file_full_path.c_str(), atlas_width, atlas_height, 1, font_atlas_buffer, atlas_width))
//         {
//             DERROR("Failed to write PNG: %s.", font_atlas_base_name.c_str());
//         }
//         else
//         {
//             DDEBUG("Saved %s successfully.", font_atlas_base_name.c_str());
//         }
//     }
//
//     s32 width        = INVALID_ID_S32;
//     s32 height       = INVALID_ID_S32;
//     s32 num_channels = INVALID_ID_S32;
//
//     stbi_set_flip_vertically_on_load(false);
//     stbi_uc *pixels = stbi_load(file_full_path.c_str(), &width, &height, &num_channels, STBI_rgb_alpha);
//     if (!pixels)
//     {
//         DERROR("Texture creation failed. Error opening file %s: %s", font_atlas_base_name.c_str(),
//                stbi_failure_reason());
//         stbi__err(0, 0);
//         return false;
//     }
//     stbi_set_flip_vertically_on_load(true);
//
//     texture font_atlas_texture;
//
//     font_atlas_texture.name         = DEFAULT_FONT_ATLAS_TEXTURE_HANDLE;
//     font_atlas_create_texture(&font_atlas_texture, pixels);
//     DASSERT(result);
//
//     stbi_image_free(pixels);
//
//     font_glyph_data *data = nullptr;
//     u32              size = 0;
//     file_full_path.clear();
//     file_full_path = "../assets/fonts/SourceSansPro-Regular.font_data";
//
//     _parse_glyph_atlas_file(&file_full_path, &data, &size);
//
//     tex_sys_state_ptr->glyphs      = data;
//     tex_sys_state_ptr->glyphs_size = size;
//
//     DASSERT(size == 96);
//
// #ifdef DEBUG
//     if (verify_data)
//     {
//         for (u32 i = 0; i < size; i++)
//         {
//             DASSERT(data[i].x0 == glyphs[i].x0);
//             DASSERT(data[i].x1 == glyphs[i].x1);
//             DASSERT(data[i].y0 == glyphs[i].y0);
//             DASSERT(data[i].y1 == glyphs[i].y1);
//             // DASSERT(data[i].w == glyphs[i].w);
//             // DASSERT(data[i].h == glyphs[i].h);
//             DASSERT(data[i].xoff == glyphs[i].xoff);
//             DASSERT(data[i].yoff == glyphs[i].yoff);
//             DASSERT(data[i].xadvance == glyphs[i].xadvance);
//             DASSERT(data[i].xoff2 == glyphs[i].xoff2);
//             DASSERT(data[i].yoff2 == glyphs[i].yoff2);
//         }
//     }
// #endif
//     return true;
// }
//
// bool texture_system_create_font_atlas()
//{
//     arena *arena = tex_sys_state_ptr->arena;
//
//     dstring prefix    = "../assets/fonts/";
//     dstring font_name = "SourceSansPro-Regular";
//     dstring suffix    = ".png";
//
//     dstring font_atlas_base_name = "SourceSansPro-Regular";
//
//     dstring file_full_path;
//     string_copy_format(file_full_path.string, "%s%s%s", 0, prefix.c_str(), font_atlas_base_name.c_str(),
//                        suffix.c_str());
//
//     bool             verify_data = false;
//     stbtt_packedchar char_data[96]; // ASCII 32..126
//
//     if (!file_exists(&file_full_path))
//     {
//         verify_data = true;
//         file_full_path.clear();
//         string_copy_format(file_full_path.string, "%s%s%s", 0, prefix.c_str(), font_name.c_str(), ".ttf");
//
//         std::fstream f;
//         u64          font_buffer_size = INVALID_ID_64;
//
//         bool result = file_open_and_read(file_full_path.c_str(), &font_buffer_size, nullptr, false);
//         DASSERT(result);
//
//         char *font_buffer = static_cast<char *>(dallocate(arena, font_buffer_size, MEM_TAG_UNKNOWN));
//         result            = file_open_and_read(file_full_path.c_str(), &font_buffer_size, font_buffer, false);
//         DASSERT(result);
//
//         u32 atlas_width  = 512;
//         u32 atlas_height = 512;
//
//         u32 font_atlas_size   = atlas_width * atlas_height;
//         u8 *font_atlas_buffer = static_cast<u8 *>(dallocate(arena, font_atlas_size, MEM_TAG_UNKNOWN));
//
//         stbtt_pack_context packContext;
//         if (!stbtt_PackBegin(&packContext, font_atlas_buffer, atlas_width, atlas_height, 0, 1, NULL))
//         {
//             DERROR("Failed to begin font packing.");
//         }
//
//         stbtt_PackSetOversampling(&packContext, 1, 1); // No oversampling
//
//         stbtt_pack_range range;
//         range.font_size                        = 32;
//         range.first_unicode_codepoint_in_range = 32; // space
//         range.num_chars                        = 96; // printable ASCII
//         range.chardata_for_range               = char_data;
//
//         if (!stbtt_PackFontRanges(&packContext, reinterpret_cast<const u8 *>(font_buffer), 0, &range, 1))
//         {
//             DERROR("Font range packing failed.");
//         }
//
//         stbtt_PackEnd(&packContext);
//
//         _write_glyph_atlas_to_file(&font_name, reinterpret_cast<font_glyph_data *>(char_data), 96);
//
//         file_full_path.clear();
//         string_copy_format(file_full_path.string, "%s%s%s", 0, prefix.c_str(), font_atlas_base_name.c_str(), ".png");
//
//         if (!stbi_write_png(file_full_path.c_str(), atlas_width, atlas_height, 1, font_atlas_buffer, atlas_width))
//         {
//             DERROR("Failed to write PNG: %s.", font_atlas_base_name.c_str());
//         }
//         else
//         {
//             DDEBUG("Saved %s successfully.", font_atlas_base_name.c_str());
//         }
//     }
//
//     s32 width        = INVALID_ID_S32;
//     s32 height       = INVALID_ID_S32;
//     s32 num_channels = INVALID_ID_S32;
//
//     stbi_set_flip_vertically_on_load(false);
//     stbi_uc *pixels = stbi_load(file_full_path.c_str(), &width, &height, &num_channels, STBI_rgb_alpha);
//     if (!pixels)
//     {
//         DERROR("Texture creation failed. Error opening file %s: %s", font_atlas_base_name.c_str(),
//                stbi_failure_reason());
//         stbi__err(0, 0);
//         return false;
//     }
//     stbi_set_flip_vertically_on_load(true);
//
//     texture font_atlas_texture;
//
//     font_atlas_texture.name         = DEFAULT_FONT_ATLAS_TEXTURE_HANDLE;
//     font_atlas_texture.width        = width;
//     font_atlas_texture.height       = height;
//     font_atlas_texture.format       = IMG_FORMAT_UNORM;
//     font_atlas_texture.num_channels = 4;
//
//     bool result = create_texture(&font_atlas_texture, pixels);
//     DASSERT(result);
//
//     stbi_image_free(pixels);
//
//     font_glyph_data *data = nullptr;
//     u32              size = 0;
//     file_full_path.clear();
//     file_full_path = "../assets/fonts/SourceSansPro-Regular.font_data";
//
//     _parse_glyph_atlas_file(&file_full_path, &data, &size);
//
//     tex_sys_state_ptr->glyphs      = data;
//     tex_sys_state_ptr->glyphs_size = size;
//
//     DASSERT(size == 96);
//
// #ifdef DEBUG
//     if (verify_data)
//     {
//         for (u32 i = 0; i < size; i++)
//         {
//             DASSERT(data[i].x0 == char_data[i].x0);
//             DASSERT(data[i].x1 == char_data[i].x1);
//             DASSERT(data[i].y0 == char_data[i].y0);
//             DASSERT(data[i].y1 == char_data[i].y1);
//             DASSERT(data[i].xoff == char_data[i].xoff);
//             DASSERT(data[i].yoff == char_data[i].yoff);
//             DASSERT(data[i].xadvance == char_data[i].xadvance);
//             DASSERT(data[i].xoff2 == char_data[i].xoff2);
//             DASSERT(data[i].yoff2 == char_data[i].yoff2);
//         }
//     }
// #endif
//     return true;
// }

// we might not need this now
