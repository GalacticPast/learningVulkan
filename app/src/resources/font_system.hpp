#pragma once
#include "core/dstring.hpp"
#include "resources/resource_types.hpp"

bool font_system_initialize(arena *system_arena, arena* resource_arena);

// INFO: base name without any suffix .* just the base name of the font that you want to use. I will only support .ttf for now.
bool font_system_load_font(dstring *font_base_name, font_data* data);

font_data *font_system_get_system_font();
