#pragma once

#include "core/dstring.hpp"
#include "math/dmath_types.hpp"
#include "memory/arenas.hpp"

enum ui_element_type
{
    UI_UNKNOWN,

    /* PRIMITIVES */
    UI_BUTTON,
    UI_SLIDER,

    /* DYNAMICS */
    UI_WINDOW,
};

bool ui_system_initialize(arena *system_arena, arena *resource_arena);

bool ui_system_set_arena(arena *arena);

void ui_system_start_frame();
u64  ui_system_end_frame();

bool ui_window(u64 parent_id, u64 id, dstring label, u32 num_rows, u32 num_columns);

bool ui_button(u64 parent_id, u64 id, dstring *text, u32 row, u32 column);

// WARN: Do not mutate the passed value. If you do mutate it will cause the slider knob to be placed incorretly, which
// is undefined behaviour..
bool ui_slider(u64 parent_id, u64 id, s32 min, s32 max, s32 *value, u32 row, u32 column);

bool ui_text(u64 id, dstring *text, vec2 position, vec4 color);
