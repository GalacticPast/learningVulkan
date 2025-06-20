#pragma once

#include "defines.hpp"
#include "memory/arenas.hpp"
#include "resources/resource_types.hpp"

bool renderer_system_startup(arena* system_arena, arena* resource_arena, struct application_config *app_config);
void renderer_system_shutdown();
bool renderer_resize();

void renderer_draw_frame(struct render_data *data);

bool renderer_update_global_data(shader* shader, u32 offset, u32 size, void* data);
bool renderer_update_globals(shader* shader, darray<u32>& sizes);

bool renderer_start_frame();
bool renderer_end_frame();

