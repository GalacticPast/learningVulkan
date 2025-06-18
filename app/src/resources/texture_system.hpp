#pragma once
#include "resource_types.hpp"

bool texture_system_initialize(arena *arena, u64 *texture_system_mem_requirements, void *state);
bool texture_system_shutdown(void *state);

bool     texture_system_create_texture(dstring *file_base_name);
texture *texture_system_get_texture(const char *texture_name);

//cube map configuration file name
bool     texture_system_load_cubemap(dstring *cube_map_conf);

font_glyph_data* texture_system_get_glyph_table(u32* length);
