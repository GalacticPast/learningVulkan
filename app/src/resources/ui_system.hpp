#pragma once

#include "math/dmath_types.hpp"
#include "memory/arenas.hpp"

bool ui_system_initialize(arena* system_arena, arena* resource_arena);

bool ui_create_button(vec2 position);
