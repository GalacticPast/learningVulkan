#include "font_system.hpp"

#include "core/dfile_system.hpp"

#include "core/dmemory.hpp"
#include "resources/resource_types.hpp"
#include "resources/shader_system.hpp"
#include "resources/texture_system.hpp"
#include "resources/material_system.hpp"

#define STB_TRUETYPE_IMPLEMENTATION
#include "vendor/stb_truetype.h"
struct font_system_state
{
    arena *arena;
    font_data system_font;
};

static font_system_state *font_sys_state_ptr;

static bool _write_glyph_atlas_to_file(dstring *file_base_name, font_glyph_data *glyph_data, u32 length);
static bool _parse_glyph_atlas_file(dstring *file_full_path, font_glyph_data **data, u32 *length);


bool font_system_initialize(arena *system_arena, arena* resource_arena)
{
    DASSERT(system_arena);
    DASSERT(resource_arena);

    font_sys_state_ptr = static_cast<font_system_state *>(dallocate(system_arena, sizeof(font_system_state), MEM_TAG_APPLICATION));
    font_sys_state_ptr->arena = resource_arena;

    dstring system_font = "SystemFont.UbuntuMono";

    font_system_load_font(&system_font, &font_sys_state_ptr->system_font);

    return true;
}

font_data *font_system_get_system_font()
{
    return &font_sys_state_ptr->system_font;
}

bool font_system_load_font(dstring *font_base_name, font_data* data)
{
    arena *arena = font_sys_state_ptr->arena;

    dstring prefix = "../assets/fonts/";

    dstring file_full_path;
    string_copy_format(file_full_path.string, "%s%s%s", 0, prefix.c_str(), font_base_name->c_str(), ".font_data");

    // INFO: Only ascii characters for now
    font_glyph_data glyphs[96]; // ASCII 32..126

    u32 atlas_width  = 256;
    u32 atlas_height = 256;

    data->atlas_height = atlas_height;
    data->atlas_width = atlas_width;

    bool verify_data = false;
    if (!file_exists(&file_full_path))
    {
        verify_data = true;
        file_full_path.clear();
        string_copy_format(file_full_path.string, "%s%s%s", 0, "../assets/fonts/", font_base_name->c_str(), ".ttf");

        std::fstream f;
        u64          font_buffer_size = INVALID_ID_64;

        bool result = file_open_and_read(file_full_path.c_str(), &font_buffer_size, nullptr, false);
        DASSERT(result == true);

        u8 *font_buffer = static_cast<u8 *>(dallocate(arena, font_buffer_size, MEM_TAG_UNKNOWN));
        result =
            file_open_and_read(file_full_path.c_str(), &font_buffer_size, reinterpret_cast<char *>(font_buffer), false);
        DASSERT(result == true);

        u32 font_atlas_size   = atlas_width * atlas_height;
        u8 *font_atlas_buffer = static_cast<u8 *>(dallocate(arena, font_atlas_size, MEM_TAG_UNKNOWN));

        bool use_sdf = false;

        if (use_sdf)
        {
            stbtt_fontinfo font;
            if (!stbtt_InitFont(&font, font_buffer, stbtt_GetFontOffsetForIndex(font_buffer, 0)))
            {
                DERROR("Failed to init font.");
                return false;
            }

            u32   char_count       = 94;
            u32   glyph_padding    = 6;
            float pixel_height     = 20;
            float scale            = stbtt_ScaleForPixelHeight(&font, pixel_height);
            float onedge_value     = 128;
            float pixel_dist_scale = 64.0f;

            s32 pen_x = 0, pen_y = 0, row_height = 0;
            for (u32 i = 0; i < char_count; ++i)
            {
                u32 c = 32 + i;

                s32 w    = 0;
                s32 h    = 0;
                s32 xoff = 0;
                s32 yoff = 0;

                unsigned char *sdf = stbtt_GetCodepointSDF(&font, scale, c, glyph_padding, onedge_value,
                                                           pixel_dist_scale, &w, &h, &xoff, &yoff);

                if (pen_x + w >= (s32)atlas_width)
                {
                    pen_x       = 0;
                    pen_y      += row_height + 1;
                    row_height  = 0;
                }

                if (pen_y + h >= (s32)atlas_height)
                {
                    DERROR("Atlas too small. Increase size.");
                    free(sdf);
                    break;
                }

                // Copy glyph SDF into atlas
                if (sdf)
                {
                    for (int y = 0; y < h; ++y)
                    {
                        dcopy_memory(font_atlas_buffer + (pen_y + y) * atlas_width + pen_x, sdf + y * w, w);
                    }

                    // Get advance
                    int adv, lsb;
                    stbtt_GetCodepointHMetrics(&font, c, &adv, &lsb);

                    glyphs[i].x0       = pen_x;
                    glyphs[i].y0       = pen_y;
                    glyphs[i].x1       = pen_x + w;
                    glyphs[i].y1       = pen_y + h;
                    // glyphs[i].w        = w;
                    // glyphs[i].h        = h;
                    glyphs[i].xoff     = (float)xoff;
                    glyphs[i].yoff     = (float)yoff;
                    glyphs[i].xadvance = scale * adv;

                    pen_x += w + 1;
                    if (h > row_height)
                        row_height = h;

                    free(sdf);
                }
            }
        }
        else
        {
            stbtt_pack_context packContext{};
            if (!stbtt_PackBegin(&packContext, font_atlas_buffer, atlas_width, atlas_height, 0, 1, NULL))
            {
                DERROR("Failed to begin font packing.");
            }

            stbtt_PackSetOversampling(&packContext, 1, 1); // No oversampling

            stbtt_pack_range range;
            range.array_of_unicode_codepoints      = nullptr;
            range.font_size                        = 20;
            range.first_unicode_codepoint_in_range = 32; // space
            range.num_chars                        = 96; // printable ASCII
            range.chardata_for_range               = reinterpret_cast<stbtt_packedchar *>(glyphs);

            STATIC_ASSERT(sizeof(stbtt_packedchar) == sizeof(font_glyph_data),
                          "These should be the same structs with same variables and sizes");
            if (!stbtt_PackFontRanges(&packContext, font_buffer, 0, &range, 1))
            {
                DERROR("Font range packing failed.");
            }

            stbtt_PackEnd(&packContext);
        }

        _write_glyph_atlas_to_file(font_base_name, glyphs, 96);

        file_full_path.clear();
        string_copy_format(file_full_path.string, "%s%s%s", 0, "../assets/textures/", font_base_name->c_str(), ".png");
        texture_system_write_texture(&file_full_path, atlas_width, atlas_height, 4, IMG_FORMAT_UNORM,
                                     font_atlas_buffer);
    }
    else
    {
        file_full_path.clear();
        string_copy_format(file_full_path.string, "%s%s%s", 0, "../assets/fonts/", font_base_name->c_str(),
                           ".font_data");
        u32 data_length = 0;
        _parse_glyph_atlas_file(&file_full_path, &data->glyphs, &data_length);
        data->glyph_table_length = data_length;
    }

    material_config font_atlas{};
    font_atlas.mat_name            = DEFAULT_FONT_ATLAS_TEXTURE_HANDLE;

    dstring base_name_plus_suffix;
    string_copy_format(base_name_plus_suffix.string, "%s%s", 0, font_base_name->c_str(), ".png");

    texture_system_create_texture(&base_name_plus_suffix, IMG_FORMAT_UNORM);

    font_atlas.albedo_map = base_name_plus_suffix;

    material_system_create_material(&font_atlas, shader_system_get_default_ui_shader_id());

    if (!data->glyphs)
    {
        data->glyphs = static_cast<font_glyph_data *>(dallocate(arena, sizeof(font_glyph_data) * 96, MEM_TAG_DARRAY));
        data->glyph_table_length = 96;

        font_glyph_data *glyphs_table = data->glyphs; // ASCII 32..126

        for (u32 i = 0; i < 96; i++)
        {
            dcopy_memory(&glyphs_table[i], &glyphs[i], sizeof(font_glyph_data));
        }
    }

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
    arena *arena = font_sys_state_ptr->arena;

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
