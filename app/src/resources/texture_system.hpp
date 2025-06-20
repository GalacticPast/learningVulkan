#pragma once
#include "resource_types.hpp"

bool texture_system_initialize(arena *system_arena, arena* resource_arena);
bool texture_system_shutdown();

bool texture_system_create_texture(dstring *texture_name, u32 tex_width, u32 tex_height, u32 tex_num_channels,
                                   image_format format, u8 *pixels);
// INFO: write a texure as jpg
bool texture_system_write_texture(dstring *texture_name, u32 tex_width, u32 tex_height, u32 tex_num_channels,
                                  image_format format, u8 *pixels);

bool     texture_system_create_texture(dstring *file_base_name, image_format format);
texture *texture_system_get_texture(const char *texture_name);

// cube map configuration file name
bool texture_system_load_cubemap(dstring *cube_map_conf);
