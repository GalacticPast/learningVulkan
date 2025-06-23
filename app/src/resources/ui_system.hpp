#pragma once

#include "core/dstring.hpp"
#include "math/dmath_types.hpp"
#include "memory/arenas.hpp"

bool ui_system_initialize(arena *system_arena, arena *resource_arena);

u64 ui_system_flush_geometries();

bool ui_button(u64 parent_id, u64 id, vec2 position);
bool ui_text(u64 id, dstring *text, vec2 position, vec4 color);
